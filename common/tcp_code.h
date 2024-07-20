/********************************************************************
 * @file    tcp_code.h
 * @brief   应用protobuf对CS、SS通信包进行加解码
 * @author  stanjiang
 * @date    2024-07-17
 * @copyright
*/
#ifndef _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_CODE_H_
#define _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_CODE_H_

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>
#include <arpa/inet.h>

/********************proto传输格式说明*****************************/
// 包总长+protobuf  message name长度+message name+protobuf data
// 注:用message name作为消息命令字
// 注:无需自定义额外的cmdid和ver
/****************************************************************/

class TcpCode {
 public:
    TcpCode() {}
    ~TcpCode() {}

    /***
     *  @brief   protobuf消息打包
     *  @param   message: 待打包的protobuf message
     *  @return   打包后的string串
     ***/
    static std::string encode(const google::protobuf::Message& message);

    /***
     *  @brief   protobuf消息解包
     *  @param   buf: 待解包的消息流
     *  @return   解包后的protobuf message
     ***/
    static google::protobuf::Message* decode(const std::string& buf);

    /***
     *  @brief   根据protobuf message typename创建message
     *  @param   type_name: protobuf message typename
     *  @return   protobuf message
     ***/
    static google::protobuf::Message* create_message(const std::string& type_name);

    /***
     *  @brief   将消息流前四个字节转换为本地字节序的int数据
     *  @param   buf: 待解包的消息流
     *  @return   转换后的int数据
     ***/
    static int convert_int32(const char* buf);

 private:
};


#endif  // _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_TCP_CODE_H_

