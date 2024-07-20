# Real-Time Securities Trading Distributed Backend Service

This project is a C++ implementation of a real-time securities trading distributed backend service. It consists of four independent services, utilizing Kafka for inter-service communication and Protobuf for message serialization. Additionally, a client application is provided for testing purposes.

## Project Structure

### Services

1. **Gateway Server**
    - Accepts client TCP socket requests.
    - Parses and validates incoming orders.
    - Sends orders to the `Order Server` via Kafka.

2. **Order Server**
    - Processes buy and sell order requests.
    - Validates orders and performs necessary pre-matching logic.
    - Forwards orders to the `Match Server` via Kafka.

3. **Match Server**
    - Matches buy and sell orders.
    - Executes the matching algorithm to find compatible orders.
    - Sends match results to the `Clearing Server` via Kafka.

4. **Clearing Server**
    - Processes matched orders.
    - Updates order status and performs necessary accounting operations.
    - Ensures that the final state of orders is consistent and accurate.

### Communication

- **Kafka**: Used for inter-service communication, ensuring reliable and efficient message passing.
- **Protobuf**: Used for message serialization between services, ensuring a compact and efficient data format.

### Logging

- **spdlog**: Used for logging within all services, providing `INFO`, `DEBUG`, and `ERROR` log levels. The logging configuration is encapsulated in a common `logger.h` file.

### Common Directory

- Contains shared headers and implementation files used across different services.
- Includes the `logger.h` file for unified logging.

### Protobuf Messages

- Located in the `proto/include/cs_proto` directory.
- Defines messages for order requests, match results, and other necessary communication structures.

## Client Application

- A testing client that simulates real-world usage of the trading system.
- Sends test orders to the `Gateway Server`.
- Verifies the proper functioning of the entire order processing pipeline.

## Directory Structure

```
project_root/
├── common/
│   ├── common_file1.h
│   ├── common_file1.cpp
│   ├── common_file2.h
│   └── common_file2.cpp
├── proto/
│   └── include/
│       └── cs_proto/
│           ├── cs_proto.pb.h
│           └── cs_proto.pb.cpp
├── client/
│   ├── CMakeLists.txt
│   ├── tcp_client.h
│   ├── tcp_client.cpp
│   └── main.cpp
└── services/
    ├── gateway/
    │   ├── gateway_server.h
    │   ├── gateway_server.cpp
    │   └── main.cpp
    ├── order/
    │   ├── order_server.h
    │   ├── order_server.cpp
    │   └── main.cpp
    ├── match/
    │   ├── match_server.h
    │   ├── match_server.cpp
    │   └── main.cpp
    └── clearing/
        ├── clearing_server.h
        ├── clearing_server.cpp
        └── main.cpp
```

## Dependencies

- **Kafka**: Message broker for inter-service communication.
- **Protobuf**: Google's protocol buffers for message serialization.
- **spdlog**: Fast C++ logging library.

## Building and Running

### Prerequisites

- CMake
- Protobuf compiler
- Kafka
- spdlog

### Steps

1. **Clone the repository**

    ```bash
    git clone <repository-url>
    cd project_root
    ```

2. **Build the project**

    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

3. **Run the services**

    Start each service in a separate terminal or deploy them using a service orchestration tool like Kubernetes.

    ```bash
    ./gateway_server
    ./order_server
    ./match_server
    ./clearing_server
    ```

4. **Run the client application**

    ```bash
    ./TcpClient
    ```

## Logging Configuration

The logging configuration is centralized in the `common/logger.h` file. It supports `INFO`, `DEBUG`, and `ERROR` log levels and uses the format:

```
[%Y-%m-%d %H:%M:%S.%e] %l: %v
```

## Testing

The client application can be used to test the end-to-end functionality of the trading system. It sends test orders to the `Gateway Server` and verifies the processing and matching of these orders through the entire pipeline.

## Future Enhancements

- Implement security measures for data transmission.
- Add more detailed logging and monitoring.
- Implement scaling strategies for high load scenarios.
