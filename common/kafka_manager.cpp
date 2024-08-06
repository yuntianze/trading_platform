#include "kafka_manager.h"
#include <thread>
#include <chrono>

using namespace cs_proto;

// Singleton instance
KafkaManager& KafkaManager::instance() {
    static KafkaManager instance;
    return instance;
}

KafkaManager::KafkaManager() : running_(false) {}

KafkaManager::~KafkaManager() {
    stop_consuming();
    if (producer_) {
        producer_->flush(1000);  // Flush with 1s timeout before destroying
    }
}

// Initialize Kafka manager with Oracle Cloud Streaming settings
bool KafkaManager::init(const std::string& bootstrap_servers,
                        const std::string& username,
                        const std::string& password) {
    bootstrap_servers_ = bootstrap_servers;
    username_ = username;
    password_ = password;
    
    // Initialize Kafka producer
    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    // Set Kafka configuration
    if (conf->set("bootstrap.servers", bootstrap_servers_, errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("security.protocol", "SASL_SSL", errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("sasl.mechanism", "PLAIN", errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("sasl.username", username_, errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("sasl.password", password_, errstr) != RdKafka::Conf::CONF_OK) {
        LOG(ERROR, "Failed to set Kafka configuration: {}", errstr);
        delete conf;
        return false;
    }

    // Set delivery report callback
    if (conf->set("dr_cb", &delivery_cb_, errstr) != RdKafka::Conf::CONF_OK) {
        LOG(ERROR, "Failed to set delivery report callback: {}", errstr);
        delete conf;
        return false;
    }

    // Create Kafka producer
    producer_.reset(RdKafka::Producer::create(conf, errstr));
    if (!producer_) {
        LOG(ERROR, "Failed to create Kafka producer: {}", errstr);
        delete conf;
        return false;
    }

    delete conf;  // Producer has taken ownership of conf

    LOG(INFO, "KafkaManager initialized successfully");
    return true;
}

// Produce a protobuf message to a topic
bool KafkaManager::produce(const std::string& topic, const google::protobuf::Message& message, int client_id) {
    if (!producer_) {
        LOG(ERROR, "Producer not initialized");
        return false;
    }

    LOG(DEBUG, "Producing message of type: {}, client_id: {}", message.GetTypeName(), client_id);

    // Create a mutable copy of the message
    std::unique_ptr<google::protobuf::Message> mutable_message(message.New());
    mutable_message->CopyFrom(message);

    // Check if the message has a client_id field and set it if present
    const google::protobuf::FieldDescriptor* client_id_field = 
        mutable_message->GetDescriptor()->FindFieldByName("client_id");
    if (client_id_field) {
        mutable_message->GetReflection()->SetInt32(mutable_message.get(), client_id_field, client_id);
    } else {
        LOG(ERROR, "Message type does not have a client_id field. Client ID: {} will not be set.", client_id);
        return false;
    }

    // Serialize the protobuf message with type information
    std::string type_name = message.GetTypeName();
    std::string serialized_content = mutable_message->SerializeAsString();
    
    // Combine type name, null separator, and serialized content
    std::string serialized_message;
    serialized_message.reserve(type_name.length() + 1 + serialized_content.length());
    serialized_message.append(type_name);
    serialized_message.push_back('\0');
    serialized_message.append(serialized_content);

    // Log the serialized message details for debugging
    LOG(DEBUG, "Serialized message: type='{}', content_length={}, total_length={}",
        type_name, serialized_content.length(), serialized_message.length());

    // Produce the message to Kafka
    RdKafka::ErrorCode err = producer_->produce(
        topic,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(serialized_message.data()),
        serialized_message.size(),
        nullptr,  // No key
        0,        // No key length
        0,        // Use current timestamp
        nullptr   // No message headers
    );

    if (err != RdKafka::ERR_NO_ERROR) {
        LOG(ERROR, "Failed to produce message: {}", RdKafka::err2str(err));
        return false;
    }

    producer_->poll(0);  // Trigger delivery report callbacks
    return true;
}

// Start consuming messages from topics
bool KafkaManager::start_consuming(const std::vector<std::string>& topics, const std::string& group_id, MessageCallback callback) {
    if (consumer_) {
        LOG(ERROR, "Consumer already running");
        return false;
    }

    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    // Set Kafka consumer configuration
    if (conf->set("bootstrap.servers", bootstrap_servers_, errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("group.id", group_id, errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("auto.offset.reset", "earliest", errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("security.protocol", "SASL_SSL", errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("sasl.mechanism", "PLAIN", errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("sasl.username", username_, errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("sasl.password", password_, errstr) != RdKafka::Conf::CONF_OK) {
        LOG(ERROR, "Failed to set Kafka consumer configuration: {}", errstr);
        delete conf;
        return false;
    }

    // Create Kafka consumer
    consumer_.reset(RdKafka::KafkaConsumer::create(conf, errstr));
    if (!consumer_) {
        LOG(ERROR, "Failed to create Kafka consumer: {}", errstr);
        delete conf;
        return false;
    }

    delete conf;  // Consumer has taken ownership of conf

    // Subscribe to topics
    RdKafka::ErrorCode err = consumer_->subscribe(topics);
    if (err) {
        LOG(ERROR, "Failed to subscribe to topics: {}", RdKafka::err2str(err));
        consumer_.reset();
        return false;
    }

    running_ = true;
    consumer_thread_ = std::make_unique<std::thread>(&KafkaManager::consume_loop, this, callback);

    LOG(INFO, "Started consuming from topics");
    return true;
}

// Stop consuming messages
void KafkaManager::stop_consuming() {
    running_ = false;
    if (consumer_thread_ && consumer_thread_->joinable()) {
        consumer_thread_->join();
    }
    if (consumer_) {
        consumer_->close();
        consumer_.reset();
    }
    LOG(INFO, "Stopped consuming messages");
}

// Flush all produced messages
void KafkaManager::flush(int timeout_ms) {
    if (producer_) {
        producer_->flush(timeout_ms);
    }
}

// Process incoming Kafka messages
void KafkaManager::process_messages() {
    // This method is now empty as message processing is handled in the consumer thread
}

// Consumer thread function
void KafkaManager::consume_loop(MessageCallback callback) {
    while (running_) {
        std::unique_ptr<RdKafka::Message> msg(consumer_->consume(100));  // 100ms timeout

        switch (msg->err()) {
            case RdKafka::ERR__TIMED_OUT:
                // No message received within timeout, this is normal
                // LOG(DEBUG, "Consume timed out");
                break;

            case RdKafka::ERR_NO_ERROR:
                {
                    if (msg->len() == 0) {
                        LOG(DEBUG, "Received empty message");
                        break;
                    }
                    std::string payload(static_cast<const char*>(msg->payload()), msg->len());
                    LOG(DEBUG, "Received message with length: {}", msg->len());
                    auto protobuf_message = deserialize_message(payload);
                    if (protobuf_message) {
                        callback(*protobuf_message);
                    } else {
                        LOG(ERROR, "Failed to deserialize message");
                    }
                }
                break;

            case RdKafka::ERR__PARTITION_EOF:
                // Reached end of partition, not an error
                LOG(DEBUG, "Reached end of partition");
                break;

            default:
                LOG(ERROR, "Consume error: {}", msg->errstr());
                break;
        }
    }
}

// Delivery report callback
void KafkaManager::DeliveryReportCb::dr_cb(RdKafka::Message& message) {
    if (message.err()) {
        LOG(ERROR, "Message delivery failed: {}", message.errstr());
    } else {
        LOG(INFO, "Message delivered to topic {} [{}] at offset {}",
                    message.topic_name(), message.partition(), message.offset());
    }
}

// Helper function to deserialize protobuf message
std::unique_ptr<google::protobuf::Message> KafkaManager::deserialize_message(const std::string& payload) {
    LOG(DEBUG, "Attempting to deserialize message of length: {}", payload.length());

    // Log the raw payload for debugging
    LOG(DEBUG, "Raw payload: length={}, content={}", payload.length(), 
        payload.substr(0, std::min(static_cast<size_t>(100), payload.length())));

    // Find the null terminator that separates the message type from the content
    size_t null_terminator = payload.find('\0');
    if (null_terminator == std::string::npos) {
        LOG(ERROR, "Invalid message format: no null terminator found");
        return nullptr;
    }

    // Extract message type and content
    std::string message_type(payload.data(), null_terminator);
    std::string message_content(payload.data() + null_terminator + 1, 
                                payload.length() - null_terminator - 1);

    LOG(DEBUG, "Message type: '{}', Content length: {}", message_type, message_content.length());

    // Create the appropriate message object based on the type
    std::unique_ptr<google::protobuf::Message> message;
    if (message_type == "cspkg.AccountLoginReq") {
        message = std::make_unique<cspkg::AccountLoginReq>();
    } else if (message_type == "cspkg.AccountLoginRes") {
        message = std::make_unique<cspkg::AccountLoginRes>();
    } else if (message_type == "cs_proto.FuturesOrder") {
        message = std::make_unique<cs_proto::FuturesOrder>();
    } else if (message_type == "cs_proto.OrderResponse") {
        message = std::make_unique<cs_proto::OrderResponse>();
    } else {
        LOG(ERROR, "Unknown message type: '{}'", message_type);
        return nullptr;
    }

    // Parse the message content
    if (!message->ParseFromString(message_content)) {
        LOG(ERROR, "Failed to parse {} message", message_type);
        return nullptr;
    }

    LOG(INFO, "Successfully deserialized {} message", message_type);
    return message;
}