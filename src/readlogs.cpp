#include <cstdio>
#include <cstring>
#include <string>
#include "common.h"
#include "readlogs.h"

//
//
// order_log read
//
//

FILE* order_fp = nullptr;

void start_read_order(const string& date) {
    string file = data_path + date + "/order_log";
    order_fp = fopen(file.c_str(), "rb");
    if (order_fp == nullptr) {
        printf("Error: Order File Not Found %s", file.c_str());
        exit(1);
    }
}

size_t read_order_log(std::vector<order_log> &vec, int batch_size) {
    vec.resize(BATCH_SIZE);
    size_t read_size = fread(vec.data(), sizeof(order_log), BATCH_SIZE, order_fp);
    return read_size;
}

//
//
// Other
//
//

size_t read_prev_trade_info(const string& date) {
    string file = data_path + date + "/prev_trade_info";
    FILE* fp = fopen(file.c_str(), "rb");
    if (fp == nullptr) {
        printf("Error: File Not Found %s", file.c_str());
        exit(1);
    }
    size_t read_size = fread(prev_trade_infos, sizeof(prev_trade_info), MAX_PREV_TRADES, fp);
    fclose(fp);
    return read_size; 
}

size_t read_alpha(const string& date) {
    string file = data_path + date + "/alpha";
    FILE* fp = fopen(file.c_str(), "rb");
    if (fp == nullptr) {
        printf("Error: File Not Found %s", file.c_str());
        exit(1);
    }
    size_t read_size = fread(alphas, sizeof(alpha), MAX_ALPHAS, fp);
    fclose(fp);
    return read_size;
}