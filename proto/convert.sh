#!/bin/bash

# 生成 C++ 文件
protoc --cpp_out=./include/ ./cs_proto/*.proto
# protoc --cpp_out=./include/ ./ss_proto/*.proto

# 后缀名修改
for file in ./include/cs_proto/*.cc; do
    mv "$file" "${file%.cc}.cpp"
done

# for file in ./include/ss_proto/*.cc; do
#     mv "$file" "${file%.cc}.cpp"
# done