#include "order_server.h"
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "logger.h"

const char* LOGFILE = "./log/order_server.log";

OrderServer::OrderServer() 
    : loop_(nullptr), 
      run_flag_(RUN_INIT),
      kafka_manager_(KafkaManager::instance()),
      order_processor_() {
}

OrderServer::~OrderServer() {
    if (loop_) {
        uv_loop_close(loop_);
        free(loop_);
    }
    Logger::log(INFO, "OrderServer destroyed");
}

OrderServer& OrderServer::instance() {
    static OrderServer s_inst;
    return s_inst;
}

int OrderServer::init(ServerStartModel model) {
    if (init_daemon(model) != 0) {
        return -1;
    }

    signal(SIGUSR1, OrderServer::sigusr1_handle);
    signal(SIGUSR2, OrderServer::sigusr2_handle);

    loop_ = (uv_loop_t*)malloc(sizeof(uv_loop_t));
    if (!loop_ || uv_loop_init(loop_) != 0) {
        Logger::log(ERROR, "Failed to initialize uv loop");
        return -1;
    }

    // Initialize KafkaManager with Oracle Cloud Streaming settings
    if (!kafka_manager_.init(
        "cell-1.streaming.ca-toronto-1.oci.oraclecloud.com:9092",
        "stanjiang2010/stanjiang2010@gmail.com/ocid1.streampool.oc1.ca-toronto-1.amaaaaaauz54kbqapjf3estamgf42ivwojfaktgruwh6frqw2acpodjuxlaq",
        "AUTH_TOKEN")) {
        Logger::log(ERROR, "Failed to initialize Kafka manager");
        return -1;
    }

    // Start consuming from the new orders topic
    if (!kafka_manager_.start_consuming({"new_orders_topic"}, "order_server_consumer_group", 
        [this](const google::protobuf::Message& message) {
            const cs_proto::FuturesOrder& order = dynamic_cast<const cs_proto::FuturesOrder&>(message);
            this->order_processor_.process_new_order(order);
        })) {
        Logger::log(ERROR, "Failed to start consuming Kafka messages");
        return -1;
    }

    if (order_processor_.init() != 0) {
        Logger::log(ERROR, "Failed to initialize order processor");
        return -1;
    }

    uv_async_init(loop_, &async_handle_, on_async);
    async_handle_.data = this;

    uv_timer_init(loop_, &check_timer_);
    check_timer_.data = this;
    uv_timer_start(&check_timer_, on_timer, 100, 100);

    Logger::log(INFO, "OrderServer initialized successfully");
    return 0;
}

void OrderServer::run() {
    Logger::log(INFO, "Starting order server main loop");
    uv_run(loop_, UV_RUN_DEFAULT);
    Logger::log(INFO, "Order server main loop ended");
}

void OrderServer::reload_config() {
    run_flag_ = RELOAD_CFG;
    uv_async_send(&async_handle_);
}

void OrderServer::stop() {
    run_flag_ = SERVER_EXIT;
    uv_async_send(&async_handle_);
}

void OrderServer::on_async(uv_async_t* handle) {
    OrderServer* server = static_cast<OrderServer*>(handle->data);
    server->process_run_flag();
}

void OrderServer::on_timer(uv_timer_t* handle) {
    OrderServer* server = static_cast<OrderServer*>(handle->data);
    server->perform_periodic_checks();
}

void OrderServer::process_run_flag() {
    switch (run_flag_) {
        case RELOAD_CFG:
            Logger::log(INFO, "Reloading configuration...");
            // Implement config reloading logic here
            run_flag_ = RUN_INIT;
            break;
        case SERVER_EXIT:
            Logger::log(INFO, "Exiting server...");
            uv_timer_stop(&check_timer_);
            kafka_manager_.stop_consuming();
            uv_stop(loop_);
            break;
        default:
            break;
    }
}

void OrderServer::perform_periodic_checks() {
    order_processor_.process_orders();
}

void OrderServer::sigusr1_handle(int sigval) {
    (void)sigval;
    OrderServer::instance().reload_config();
}

void OrderServer::sigusr2_handle(int sigval) {
    (void)sigval;
    OrderServer::instance().stop();
}

int OrderServer::init_daemon(ServerStartModel model) {
    // Check if another instance is running
    const char* lockFilePath = "./order_server.lock";
    int lock_fd = open(lockFilePath, O_RDWR | O_CREAT, 0640);
    if (lock_fd < 0) {
        Logger::log(ERROR, "Open lock file failed: {}", strerror(errno));
        return -1;
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        Logger::log(ERROR, "Lock file failed, another instance is running.");
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
        Logger::log(ERROR, "Fork failed: {}", strerror(errno));
        close(lock_fd);
        return -1;
    } else if (pid > 0) {
        // Parent process exits
        _exit(EXIT_SUCCESS);
    }

    // Child process continues

    // Create new session
    if (setsid() < 0) {
        Logger::log(ERROR, "Setsid failed: {}", strerror(errno));
        return -1;
    }

    // Change working directory
    if (chdir("/") < 0) {
        Logger::log(ERROR, "Chdir failed: {}", strerror(errno));
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
        Logger::log(ERROR, "Fork failed: {}", strerror(errno));
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
        Logger::log(ERROR, "Failed to initialize Order server");
        return -1;
    }

    Logger::log(INFO, "Order server started successfully");
    printf("Order server started successfully\n");

    server.run();

    return 0;
}

