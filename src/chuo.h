//
// Created by Administrator on 2023/8/28.
//

#ifndef MAIN_CHUO_H
#define MAIN_CHUO_H

#include "common.h"
#include "unordered_map"
#include <functional>
#include <queue>
#include <cstring>
#include <vector>
#include <list>
#include "algorithm"

namespace Chuo {
    struct PriceAndIdAndVolume {
        int price;
        int order_id; // 压一位history bool, 压到 &1的位置去.
        mutable int volume;

        bool operator<(const PriceAndIdAndVolume & rhs) const {
            return price == rhs.price ? order_id < rhs.order_id : price < rhs.price;
        }
        bool operator>(const PriceAndIdAndVolume & rhs) const {
            return price == rhs.price ? order_id < rhs.order_id : price > rhs.price;
        }
    };

    int price_double2int(double price);
    double price_int2double(int price);

    using BID_PQ = std::priority_queue<PriceAndIdAndVolume>;
    using ASK_PQ = std::priority_queue<PriceAndIdAndVolume, vector<PriceAndIdAndVolume>, std::greater<PriceAndIdAndVolume>>;
    using std::unordered_map;

    class Worker {
    public:
        /* 决赛数据包含714个交易日的数据
         * 每个交易日包含100-150支合约，每支合约大约400000～480000笔历史单和480个信号
         * */
        struct BidsAndAsks {
            BID_PQ bid;
            ASK_PQ ask;
            int last_price = -1; // 最近成交价 -> 用于更新收盘价.
            int prev_close_price = -1; // 昨日收盘
            int up_limit = 0;
            int down_limit = 0;
            int bid_sum_volume = 0; // bid volume 总量
            int ask_sum_volume = 0; // ask volume 总量
            int cur_position; // 当前仓位, 和昨日收盘仓位一致;
            int last_position = 0; // 昨日仓位
            long long cash = 0; // 现金

            double pnl() {
                return (double)cur_position * price_int2double(last_price) + cash / 100.0
                       - (double)last_position * price_int2double(prev_close_price);
            }
        };

#ifdef HASH_USE_3BYTE
        int x_idx = 0, y_idx = 0, z_idx = 0;
        std::list<std::pair<unsigned long long int, BidsAndAsks>> hash[65536];
#else
        unordered_map<unsigned long long int, BidsAndAsks> umap;
#endif

        Worker();

        void process_prev_trade(prev_trade_info prev_trade_infos[], size_t n);
        void calc_pnl_and_pos(prev_trade_info prevTradeInfo[], pnl_and_pos pnl_and_poses[], size_t n);
        BidsAndAsks & get_instrument(const char * instrument_id);
        int get_bid(BidsAndAsks & bids_and_asks) const;
        int get_ask(BidsAndAsks & bids_and_asks) const;
        void output_pnl_and_pos(size_t prev_trades_size, string date, int session_number, int session_length);
        void output_twap_order(twap_order twap_orders[], size_t twap_size, string date, int session_number, int session_length);

        int make_order(const order_log & order, bool is_alpha = false);
        int ASK_make_order(const order_log & order, bool is_alpha = false);
        int ASK_process_dynamic_price_order_level_5(const order_log& order, BidsAndAsks& bidsAndAsks);
        int ASK_process_fix_price_order(const order_log& order, bool is_alpha, int price, BidsAndAsks& bidsAndAsks);
        int ASK_get_base_price(BidsAndAsks & bid_and_asks);

        int BID_make_order(const order_log & order, bool is_alpha = false);
        int BID_process_dynamic_price_order_level_5(const order_log& order, BidsAndAsks& bidsAndAsks);
        int BID_process_fix_price_order(const order_log& order, bool is_alpha, int price, BidsAndAsks& bidsAndAsks);
        int BID_get_base_price(BidsAndAsks & bid_and_asks);

        int
        output_pnl_and_pos_to_network(size_t prev_trades_size, string date, int session_number, int session_length,
                                      int sockfd,
                                      int N, char *buffer);

        int output_twap_order_to_network(twap_order *twap_orders, size_t twap_size, string date, int session_number,
                                          int session_length, int sockfd, int N, char *buffer);
    };
};

#endif //MAIN_CHUO_H
