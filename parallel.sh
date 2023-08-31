#!/bin/bash
rm -rf /home/team9/UbiquantSystem/cmake-build-release
/usr/bin/cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=/usr/bin/make -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ -G "CodeBlocks - Unix Makefiles" -S /home/team9/UbiquantSystem -B /home/team9/UbiquantSystem/cmake-build-release
make -C /home/team9/UbiquantSystem/cmake-build-release

rm /home/team9/pnl_and_pos/*
rm /home/team9/twap_order/*

# 记录当前时间(毫秒)
date_start=$(date +%s%N)

file_list=(/mnt/data/*)
total_files=${#file_list[@]}
files_per_section=$((total_files / 4))

# 并行执行子shell的函数
run_parallel() {
  local start_idx=$1
  local end_idx=$2
  local port=$3
  local pid=0

  for ((i = start_idx; i < end_idx; i++)); do
    ./cmake-build-release/main ${file_list[i]} $port & pid=$!
    wait $pid
  done
}

# 并行执行四个子shell
for ((section = 0; section < 4; section++)); do
  start=$((section * files_per_section))
  if ((section == 3)); then
    end=$total_files
  else
    end=$((start + files_per_section))
  fi

  run_parallel "$start" "$end" $((11110 + section)) &
done

wait

# 输出运行时间
end=$(date +%s%N)
time=$((end-start))
echo "运行时间：$((time/1000000))毫秒"


cd /home/team9/minitools
g++ check_all_twap.cpp -std=c++17 -o check_all_twap -lstdc++fs
g++ check_all_pos.cpp -std=c++17 -o check_all_pos -lstdc++fs
./check_all_pos
./check_all_twap
