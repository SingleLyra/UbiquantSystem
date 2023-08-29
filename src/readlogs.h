//
// Created by Administrator on 2023/8/28.
//

#ifndef MAIN_READLOGS_H
#define MAIN_READLOGS_H
#include "common.h"

// get all filenames in a directory.
vector<string> get_dir_files(const string& dir);

// read order logs from a file.
// 返回读取数量
size_t read_order_log(const string& filename, int batch_size, int start_pos);

// read prev trade info from a file.
// 返回读取数量
size_t read_prev_trade_info(const string& filename, int batch_size, int start_pos);

// read alpha from a file.
// 返回读取数量
size_t read_alpha(const string& filename, int batch_size, int start_pos);

#endif //MAIN_READLOGS_H
