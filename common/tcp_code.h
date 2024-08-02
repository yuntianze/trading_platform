/********************************************************************
 * @file    tcp_code.h
 * @brief   Encoding and decoding of CS and SS communication packages using protobuf
 * @author  stanjiang
 * @date    2024-07-17
 * @copyright
*/

#ifndef _TRADING_PLATFORM_COMMON_TCP_CODE_H_
#define _TRADING_PLATFORM_COMMON_TCP_CODE_H_

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>
#include <arpa/inet.h>

/********************Proto transmission format description*****************************/
// Total package length + protobuf message name length + message name + protobuf data
// Note: message name is used as the message command word
// Note: No need to define additional cmdid and ver
/*************************************************************************************/

class TcpCode {
public:
    TcpCode() {}
    ~TcpCode() {}

    // Encode protobuf message
    static std::string encode(const google::protobuf::Message& message);

    // Decode protobuf message
    static google::protobuf::Message* decode(const std::string& buf);

    // Create message based on protobuf message typename
    static google::protobuf::Message* create_message(const std::string& type_name);

    // Convert the first four bytes of the message stream to int data in host byte order
    static int convert_int32(const char* buf);

private:
};

#endif  // _TRADING_PLATFORM_COMMON_TCP_CODE_H_