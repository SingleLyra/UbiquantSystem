#!/bin/bash
./build/main 20150101 3 1 & pid1=$!
./build/main 20160202 3 1 & pid2=$!
./build/main 20170303 3 1 & pid3=$!
./build/main 20180404 3 1 & pid4=$!

wait $pid1
echo "./main 1 已完成"

wait $pid2
echo "./main 2 已完成"

wait $pid3
echo "./main 3 已完成"

wait $pid4
echo "./main 4 已完成"
