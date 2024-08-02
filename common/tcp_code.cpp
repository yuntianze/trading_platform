#include "tcp_code.h"
#include <algorithm>
#include "tcp_comm.h"
#include "logger.h"
#include "role.pb.h"
#include "futures_order.pb.h"

std::string TcpCode::encode(const google::protobuf::Message& message)
{
    std::string result;
    result.resize(PKGHEAD_FIELD_SIZE);  // Reserve the first four bytes for the message header

    // Add message type length and specific content of message type
    const std::string& type_name = message.GetTypeName();
    int name_len = static_cast<int>(type_name.size()+1);
    int be32 = ::htonl(name_len);
    result.append(reinterpret_cast<char*>(&be32), sizeof(be32));
    result.append(type_name.c_str(), name_len);

    // Append protobuf message protocol data to result
    bool succeed = message.AppendToString(&result);
    if (succeed) {
        // Calculate the total length of the message body and add it to the top of the message header
        int len = ::htonl(result.size());
        std::copy(reinterpret_cast<char*>(&len), reinterpret_cast<char*>(&len) + sizeof(len), result.begin());
        LOG(INFO, "Encoded message successfully, name={0:s}", type_name);
    } else {
        LOG(ERROR, "Failed to encode message, name={0:s}", type_name);
        result.clear();
    }

    return result;
}

google::protobuf::Message* TcpCode::decode(const std::string& buf) {
    google::protobuf::Message* result = NULL;
    int len = static_cast<int>(buf.size());  // Total length of the message package
    LOG(INFO, "Decoding message info, pkglen={0:d}", len);

    if (len >= 2*PKGHEAD_FIELD_SIZE) {
        int name_len = convert_int32(buf.c_str()+PKGHEAD_FIELD_SIZE);
        LOG(INFO, "Decoding message info, namelen={0:d}", name_len);

        if (name_len >= 2 && name_len <= len - 2*PKGHEAD_FIELD_SIZE) {
            std::string type_name(buf.begin() + 2*PKGHEAD_FIELD_SIZE, buf.begin() + 2*PKGHEAD_FIELD_SIZE + name_len-1);
            google::protobuf::Message* message = create_message(type_name);
            if (message != NULL) {
                const char* data = buf.c_str() + 2*PKGHEAD_FIELD_SIZE + name_len;
                int data_len = len - name_len - 2*PKGHEAD_FIELD_SIZE;
                if (message->ParseFromArray(data, data_len)) {
                    result = message;
                    LOG(INFO, "Decoded message successfully, name={0:s}", type_name);
                } else {
                    // Failed to parse protobuf message
                    LOG(ERROR, "Failed to decode message, name={0:s}", type_name);
                    delete message;
                }
            } else {
                // Failed to create protobuf message
                LOG(ERROR, "Failed to create message, name={0:s}", type_name);
            }
        } else {
            // Invalid message type length
            LOG(ERROR, "Failed to decode message, invalid namelen={0:d}", name_len);
        }
    }

    return result;
}

google::protobuf::Message* TcpCode::create_message(const std::string& type_name) {
    if (type_name == "cspkg.AccountLoginReq") {
        return new cspkg::AccountLoginReq();
    } else if (type_name == "cspkg.AccountLoginRes") {
        return new cspkg::AccountLoginRes();
    } else if (type_name == "cs_proto.FuturesOrder") {
        return new cs_proto::FuturesOrder();
    } else if (type_name == "cs_proto.OrderResponse") {
        return new cs_proto::OrderResponse();
    }
    LOG(ERROR, "Unknown message type: {}", type_name);
    return nullptr;
}

int TcpCode::convert_int32(const char* buf) {
    int be32 = 0;
    ::memmove(&be32, buf, sizeof(be32));
    return ::ntohl(be32);
}