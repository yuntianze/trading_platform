/*************************************************************************
 * @file    kafka_manager.h
 * @brief   KafkaManager class declaration for handling Kafka operations with protobuf support
 * @author  stanjiang
 * @date    2024-08-01
 * @copyright
***/

#ifndef _COMMON_KAFKA_MANAGER_H_
#define _COMMON_KAFKA_MANAGER_H_

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <librdkafka/rdkafkacpp.h>
#include <google/protobuf/message.h>
#include "futures_order.pb.h"
#include "logger.h"

class KafkaManager {
public:
    // Callback function type for message consumption
    using MessageCallback = std::function<void(const google::protobuf::Message&)>;

    // Singleton instance
    static KafkaManager& instance();

    // Initialize Kafka manager with Oracle Cloud Streaming settings
    bool init(const std::string& bootstrap_servers,
              const std::string& username,
              const std::string& password);

    // Produce a protobuf message to a topic with additional metadata
    bool produce(const std::string& topic, const google::protobuf::Message& message, int client_id);

    // Start consuming messages from topics
    bool start_consuming(const std::vector<std::string>& topics, const std::string& group_id, MessageCallback callback);

    // Stop consuming messages
    void stop_consuming();

    // Flush all produced messages
    void flush(int timeout_ms);

    // Process incoming Kafka messages
    void process_messages();

private:
    KafkaManager();
    ~KafkaManager();
    KafkaManager(const KafkaManager&) = delete;
    KafkaManager& operator=(const KafkaManager&) = delete;

    // Kafka configuration
    std::string bootstrap_servers_;
    std::string username_;
    std::string password_;

    // Kafka producer
    std::unique_ptr<RdKafka::Producer> producer_;

    // Kafka consumer
    std::unique_ptr<RdKafka::KafkaConsumer> consumer_;

    // Flag to control consumption loop
    bool running_;

    // Delivery report callback
    class DeliveryReportCb : public RdKafka::DeliveryReportCb {
    public:
        void dr_cb(RdKafka::Message& message) override;
    };

    DeliveryReportCb delivery_cb_;

    // Consumer polling thread
    std::unique_ptr<std::thread> consumer_thread_;

    // Consumer thread function
    void consume_loop(MessageCallback callback);

    // Helper function to deserialize protobuf message
    std::unique_ptr<google::protobuf::Message> deserialize_message(const std::string& payload);
};

#endif // _COMMON_KAFKA_MANAGER_H_