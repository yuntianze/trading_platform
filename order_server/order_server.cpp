#include "order_server.h"
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "logger.h"
#include "futures_order.pb.h"
#include "role.pb.h"

const char* LOGFILE = "./log/order_server.log";

OrderServer::OrderServer() 
    : running_(false),
      reload_config_(false),
      kafka_manager_(KafkaManager::instance()),
      order_processor_() {
}

OrderServer::~OrderServer() {
    LOG(INFO, "OrderServer destroyed");
}

OrderServer& OrderServer::instance() {
    static OrderServer s_inst;
    return s_inst;
}

int OrderServer::init(ServerStartModel model) {
    if (init_daemon(model) != 0) {
        return -1;
    }

    // Set up signal handlers
    signal(SIGINT, OrderServer::signal_handler);
    signal(SIGTERM, OrderServer::signal_handler);
    signal(SIGUSR1, OrderServer::signal_handler);
    signal(SIGUSR2, OrderServer::signal_handler);

    // Initialize KafkaManager with Oracle Cloud Streaming settings
    if (!kafka_manager_.init(
        "cell-1.streaming.ca-toronto-1.oci.oraclecloud.com:9092",
        "stanjiang2010/stanjiang2010@gmail.com/ocid1.streampool.oc1.ca-toronto-1.amaaaaaauz54kbqapjf3estamgf42ivwojfaktgruwh6frqw2acpodjuxlaq",
        "WIe46t6kj<Z[]cN+Y3ug")) {
        LOG(ERROR, "Failed to initialize Kafka manager");
        return -1;
    }

    // Start consuming from the new orders topic
    if (!kafka_manager_.start_consuming({GATEWAY_TO_ORDER_TOPIC}, ORDER_KAFKA_CONSUMER_GROUP_ID, 
        [this](const google::protobuf::Message& message) {
            this->handle_kafka_message(message);
        })) {
        LOG(ERROR, "Failed to start consuming Kafka messages");
        return -1;
    }

    if (order_processor_.init() != 0) {
        LOG(ERROR, "Failed to initialize order processor");
        return -1;
    }

    LOG(INFO, "OrderServer initialized successfully");
    return 0;
}

void OrderServer::run() {
    LOG(INFO, "Starting order server main loop");
    running_ = true;
    while (running_) {
        // Process run flag
        process_run_flag();

        // Process incoming Kafka messages
        kafka_manager_.process_messages();
        
        // Process pending orders
        order_processor_.process_orders();
        
        // Small sleep to prevent CPU hogging
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    LOG(INFO, "Order server main loop ended");
}

void OrderServer::handle_kafka_message(const google::protobuf::Message& message) {
    if (const auto* login_req = dynamic_cast<const cspkg::AccountLoginReq*>(&message)) {
        handle_login_request(*login_req);
    } else if (const auto* order = dynamic_cast<const cs_proto::FuturesOrder*>(&message)) {
        handle_futures_order(*order);
    } else {
        LOG(ERROR, "Received unknown message type");
    }
}

void OrderServer::handle_login_request(const cspkg::AccountLoginReq& login_req) {
    // Use OrderProcessor to validate login
    cspkg::AccountLoginRes login_res = order_processor_.validate_login(login_req);

    // Send login response back to gateway_server
    if (kafka_manager_.produce(ORDER_TO_GATEWAY_TOPIC, login_res, login_req.account())) {
        LOG(INFO, "Sent AccountLoginRes to Kafka for account {}", login_req.account());
    } else {
        LOG(ERROR, "Failed to send AccountLoginRes to Kafka for account {}", login_req.account());
    }

    if (login_res.result() == 0) {  // Login successful
        // Allocate user object or perform other necessary operations
        order_processor_.allocate_user_object(login_req.account());
    }
}

void OrderServer::handle_futures_order(const cs_proto::FuturesOrder& order) {
    // Process the order using OrderProcessor
    cs_proto::OrderResponse response = order_processor_.process_new_order(order);
    
    // Send the response back to gateway_server via Kafka
    if (kafka_manager_.produce(ORDER_TO_GATEWAY_TOPIC, response, order.client_id())) {
        LOG(INFO, "Sent response to Kafka for client {}", order.client_id());
    } else {
        LOG(ERROR, "Failed to send response to Kafka for client {}", order.client_id());
    }
}

void OrderServer::process_run_flag() {
    if (reload_config_) {
        LOG(INFO, "Reloading configuration...");
        // Implement config reloading logic here
        // For example:
        // reload_configuration();
        reload_config_ = false;
        LOG(INFO, "Configuration reloaded");
    }
}

void OrderServer::reload_config() {
    LOG(INFO, "Reload configuration requested");
    reload_config_ = true;
}

void OrderServer::stop() {
    LOG(INFO, "Stopping order server...");
    running_ = false;
    kafka_manager_.stop_consuming();
}

void OrderServer::signal_handler(int signum) {
    OrderServer& server = OrderServer::instance();
    switch (signum) {
        case SIGINT:
        case SIGTERM:
        case SIGUSR2:
            server.stop();
            break;
        case SIGUSR1:
            server.reload_config();
            break;
        default:
            break;
    }
}

int OrderServer::init_daemon(ServerStartModel model) {
    // Check if another instance is running
    const char* lockFilePath = "./order_server.lock";
    int lock_fd = open(lockFilePath, O_RDWR | O_CREAT, 0640);
    if (lock_fd < 0) {
        LOG(ERROR, "Open lock file failed: {}", strerror(errno));
        return -1;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        LOG(ERROR, "Lock file failed, another instance is running.");
        close(lock_fd);
        return -1;
    }

    // Daemonize if requested
    if (model != SERVER_START_DAEMON) {
        return 0; // Not daemonizing
    }

    // Fork child process
    pid_t pid = fork();
    if (pid < 0) {
        LOG(ERROR, "Fork failed: {}", strerror(errno));
        close(lock_fd);
        return -1;
    } else if (pid > 0) {
        // Parent process exits
        _exit(EXIT_SUCCESS);
    }

    // Child process continues

    // Create new session
    if (setsid() < 0) {
        LOG(ERROR, "Setsid failed: {}", strerror(errno));
        return -1;
    }

    // Change working directory
    if (chdir("/") < 0) {
        LOG(ERROR, "Chdir failed: {}", strerror(errno));
        return -1;
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect standard file descriptors to /dev/null
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_WRONLY); // stderr

    // Ignore SIGHUP signal
    signal(SIGHUP, SIG_IGN);

    // Fork again to avoid becoming a session leader
    pid = fork();
    if (pid < 0) {
        LOG(ERROR, "Fork failed: {}", strerror(errno));
        return -1;
    } else if (pid > 0) {
        // First child process exits
        _exit(EXIT_SUCCESS);
    }

    // Grandchild process continues

    // Reset file creation mask
    umask(0);

    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    Logger::init(LOGFILE);

    ServerStartModel model = SERVER_START_NODAEMON;
    OrderServer& server = OrderServer::instance();
    
    if (server.init(model) != 0) {
        LOG(ERROR, "Failed to initialize Order server");
        return -1;
    }

    LOG(INFO, "Order server started successfully");
    printf("Order server started successfully\n");

    server.run();

    return 0;
}