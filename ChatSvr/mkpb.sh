#!/bin/bash
pb_path="./chatpb/"
echo "proto path is:${pb_path}"
echo "mk common statrt..."
protoc --proto_path=$pb_path --go_out=$pb_path ${pb_path}common.proto
protoc --proto_path=$pb_path --cpp_out=$pb_path ${pb_path}common.proto
echo "mk common end..."
echo "mk common end..."
echo "mk chat start..."
protoc --proto_path=$pb_path --go_out=$pb_path ${pb_path}chat.proto
protoc --proto_path=$pb_path --cpp_out=$pb_path ${pb_path}chat.proto
echo "mk common end..."
echo "mk chat end.."
echo "end..........."
