#ifndef MAIN_COMMON_H
#define MAIN_COMMON_H

#include <iostream>
#include <vector>

using std::string;
using std::vector;

struct prev_trade_info{
    char instrument_id[8];
    double prev_close_price;
    int prev_position;
}__attribute__((packed)); // trade info of the last day.

struct order_log {
    char instrument_id[8];
    long timestamp;
    int type;
    int direction;
    int volume;
    double price_off;
}__attribute__((packed)); // orders of each day.

struct alpha {
    char instrument_id[8];
    long timestamp;
    int target_volume; // 仓位
}__attribute__((packed));

struct twap_order {
    char instrument_id[8];
    long timestamp;
    int direction;
    int volume;
    double price;
}__attribute__((packed));

struct pnl_and_pos {
    char instrument_id[8];
    int position;
    double pnl;
}__attribute__((packed));

// 数据路径
const string data_path = "/mnt/data/";

// 单次读入量
const int BATCH_SIZE = 1000000;
extern order_log order_logs[BATCH_SIZE];

// 150 合约
const int MAX_PREV_TRADES = 1000;
extern prev_trade_info prev_trade_infos[MAX_PREV_TRADES];

// 150 合约 * 480 信号 = 72000
const int MAX_ALPHAS = 100000;
extern alpha alphas[MAX_ALPHAS];

// 72000 * 5 (session_number) = 36w
const int MAX_TWAP_ORDERS = 400000;
extern twap_order twap_orders[MAX_TWAP_ORDERS];

// 150 合约
const int MAX_PNL_ANS_POSES = 1000;
extern pnl_and_pos pnl_and_poses[MAX_PNL_ANS_POSES];

#endif //MAIN_COMMON_H
