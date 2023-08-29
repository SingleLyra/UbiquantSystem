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

#define BATCH_SIZE 100000

struct Singleton {
    struct Settings {
        const string data_path = "/mnt/data/";
        order_log order_logs[BATCH_SIZE];
        prev_trade_info prev_trade_infos[BATCH_SIZE];
        alpha alphas[BATCH_SIZE];
        twap_order twap_orders[BATCH_SIZE];
        pnl_and_pos pnl_and_poses[BATCH_SIZE];
    };
    static Settings& get_instance() {
        static Settings instance;
        return instance;
    }
};

#endif //MAIN_COMMON_H
