#!/bin/bash
/usr/bin/cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=/usr/bin/make -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ -G "CodeBlocks - Unix Makefiles" -S /home/team9/UbiquantSystem -B /home/team9/UbiquantSystem/cmake-build-release
make -C /home/team9/UbiquantSystem/cmake-build-release

rm /home/team9/pnl_and_pos/*
rm /home/team9/twap_order/*

# 记录当前时间(毫秒)
start=$(date +%s%N)
./cmake-build-release/main 20150101 3 1 & pid1=$!
./cmake-build-release/main 20160202 3 1 & pid2=$!
./cmake-build-release/main 20170303 3 1 & pid3=$!
./cmake-build-release/main 20180404 3 1 & pid4=$!

wait $pid1
echo "./main 1 已完成"

wait $pid2
echo "./main 2 已完成"

wait $pid3
echo "./main 3 已完成"

wait $pid4
echo "./main 4 已完成"

# 输出运行时间
end=$(date +%s%N)
time=$((end-start))
echo "运行时间：$((time/1000000))毫秒"


cd /home/team9/minitools
g++ check_all_twap.cpp -std=c++17 -o check_all_twap -lstdc++fs
g++ check_all_pos.cpp -std=c++17 -o check_all_pos -lstdc++fs
./check_all_pos
./check_all_twap
