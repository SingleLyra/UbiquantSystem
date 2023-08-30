#ifndef MAIN_READLOGS_H
#define MAIN_READLOGS_H
#include "common.h"
#include "vector"

void start_read_order(const string& date);
size_t read_order_log(std::vector<order_log> &vec, int batch_size);
size_t read_prev_trade_info(const string& date);
size_t read_alpha(const string& date);

#endif //MAIN_READLOGS_H
