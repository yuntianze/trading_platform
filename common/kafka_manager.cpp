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
bool KafkaManager::init(const std::string& brokers, const std::string& username, const std::string& password) {
    brokers_ = brokers;
    username_ = username;
    password_ = password;
    
    // Initialize Kafka producer
    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    if (conf->set("bootstrap.servers", brokers_, errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("security.protocol", "SASL_SSL", errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("sasl.mechanism", "PLAIN", errstr) != RdKafka::Conf::CONF_OK) {
        Logger::log(ERROR, "Failed to set Kafka configuration: {}", errstr);
        return false;
    }

    std::string jaas_config = "org.apache.kafka.common.security.plain.PlainLoginModule required username=\"" 
                              + username_ + "\" password=\"" + password_ + "\";";
    if (conf->set("sasl.jaas.config", jaas_config, errstr) != RdKafka::Conf::CONF_OK) {
        Logger::log(ERROR, "Failed to set JAAS config: {}", errstr);
        return false;
    }

    if (conf->set("dr_cb", &delivery_cb_, errstr) != RdKafka::Conf::CONF_OK) {
        Logger::log(ERROR, "Failed to set delivery report callback: {}", errstr);
        return false;
    }

    producer_.reset(RdKafka::Producer::create(conf, errstr));
    if (!producer_) {
        Logger::log(ERROR, "Failed to create Kafka producer: {}", errstr);
        return false;
    }

    delete conf;  // Producer has taken ownership of conf

    Logger::log(INFO, "KafkaManager initialized successfully");
    return true;
}

// Produce a protobuf message to a topic
bool KafkaManager::produce(const std::string& topic, const google::protobuf::Message& message, int client_id) {
    if (!producer_) {
        Logger::log(ERROR, "Producer not initialized");
        return false;
    }

    // Add client ID to the message
    google::protobuf::Message* mutable_message = message.New();
    mutable_message->CopyFrom(message);
    mutable_message->GetReflection()->SetInt32(mutable_message, 
        mutable_message->GetDescriptor()->FindFieldByName("client_id"), client_id);

    std::string serialized_message;
    if (!mutable_message->SerializeToString(&serialized_message)) {
        Logger::log(ERROR, "Failed to serialize protobuf message");
        delete mutable_message;
        return false;
    }

    RdKafka::ErrorCode err = producer_->produce(
        topic,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(serialized_message.c_str()),
        serialized_message.size(),
        nullptr,  // No key
        0,        // No key length
        0,        // Use current timestamp
        nullptr   // No message headers
    );

    delete mutable_message;

    if (err != RdKafka::ERR_NO_ERROR) {
        Logger::log(ERROR, "Failed to produce message: {}", RdKafka::err2str(err));
        return false;
    }

    producer_->poll(0);  // Trigger delivery report callbacks
    return true;
}

// Start consuming messages from topics
bool KafkaManager::start_consuming(const std::vector<std::string>& topics, const std::string& group_id, MessageCallback callback) {
    if (consumer_) {
        Logger::log(ERROR, "Consumer already running");
        return false;
    }

    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    if (conf->set("bootstrap.servers", brokers_, errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("group.id", group_id, errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("auto.offset.reset", "earliest", errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("security.protocol", "SASL_SSL", errstr) != RdKafka::Conf::CONF_OK ||
        conf->set("sasl.mechanism", "PLAIN", errstr) != RdKafka::Conf::CONF_OK) {
        Logger::log(ERROR, "Failed to set Kafka consumer configuration: {}", errstr);
        delete conf;
        return false;
    }

    std::string jaas_config = "org.apache.kafka.common.security.plain.PlainLoginModule required username=\"" 
                              + username_ + "\" password=\"" + password_ + "\";";
    if (conf->set("sasl.jaas.config", jaas_config, errstr) != RdKafka::Conf::CONF_OK) {
        Logger::log(ERROR, "Failed to set JAAS config for consumer: {}", errstr);
        delete conf;
        return false;
    }

    consumer_.reset(RdKafka::KafkaConsumer::create(conf, errstr));
    if (!consumer_) {
        Logger::log(ERROR, "Failed to create Kafka consumer: {}", errstr);
        delete conf;
        return false;
    }

    delete conf;  // Consumer has taken ownership of conf

    RdKafka::ErrorCode err = consumer_->subscribe(topics);
    if (err) {
        Logger::log(ERROR, "Failed to subscribe to topics: {}", RdKafka::err2str(err));
        consumer_.reset();
        return false;
    }

    running_ = true;
    consumer_thread_ = std::make_unique<std::thread>(&KafkaManager::consume_loop, this, callback);

    Logger::log(INFO, "Started consuming from topics");
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
    Logger::log(INFO, "Stopped consuming messages");
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
                break;

            case RdKafka::ERR_NO_ERROR:
                {
                    std::string payload(static_cast<const char*>(msg->payload()), msg->len());
                    auto protobuf_message = deserialize_message(payload);
                    if (protobuf_message) {
                        callback(*protobuf_message);
                    } else {
                        Logger::log(ERROR, "Failed to deserialize message");
                    }
                }
                break;

            case RdKafka::ERR__PARTITION_EOF:
                // Reached end of partition, not an error
                break;

            default:
                Logger::log(ERROR, "Consume error: {}", msg->errstr());
                break;
        }
    }
}

// Delivery report callback
void KafkaManager::DeliveryReportCb::dr_cb(RdKafka::Message& message) {
    if (message.err()) {
        Logger::log(ERROR, "Message delivery failed: {}", message.errstr());
    } else {
        Logger::log(INFO, "Message delivered to topic {} [{}] at offset {}",
                    message.topic_name(), message.partition(), message.offset());
    }
}

// Helper function to deserialize protobuf message
std::unique_ptr<google::protobuf::Message> KafkaManager::deserialize_message(const std::string& payload) {
    // Here we assume that the first few bytes of the payload contain the message type
    // You might want to implement a more robust message type identification system
    if (payload.substr(0, 11) == "FuturesOrder") {
        auto message = std::make_unique<cs_proto::FuturesOrder>();
        if (message->ParseFromString(payload.substr(11))) {
            return message;
        }
    }
    // Add more message types as needed

    return nullptr;
}