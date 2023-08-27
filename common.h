#ifndef MAIN_COMMON_H
#define MAIN_COMMON_H

#include <iostream>
#include <vector>


using std::string;
using std::vector;


struct Singleton {
    struct Settings {
        const string data_path = "/mnt/data/";
    };
    static Settings& get_instance() {
        static Settings instance;
        return instance;
    }
};
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
    int target_volume;
}__attribute__((packed));

#endif //MAIN_COMMON_H
