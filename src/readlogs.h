//
// Created by Administrator on 2023/8/28.
//

#ifndef MAIN_READLOGS_H
#define MAIN_READLOGS_H
#include "common.h"


void get_dir_files(const string& dir, vector<string>& files); // get all filenames in a directory.
void read_order_log(const string& filename, vector<order_log>& order_logs, int batch_size, int start_pos); // read order logs from a file.


#endif //MAIN_READLOGS_H
