#!/bin/bash

# 生成 C++ 文件
protoc --cpp_out=./include/ ./cs_proto/*.proto

# 使用 mv 命令替代 rename
for file in ./include/cs_proto/*.cc; do
    mv "$file" "${file%.cc}.cpp"
done