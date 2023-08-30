//
// Created by Administrator on 2023/8/28.
//

#ifndef MAIN_CHUO_H
#define MAIN_CHUO_H

#include "common.h"
#include "prio_queue.hpp"
#include "unordered_map"
#include <cstring>
#include "algorithm"

namespace Chuo {
    struct PriceAndId {
        int price;
        int order_id; // 压一位history bool, 压到 &1的位置去.

        bool operator<(const PriceAndId & rhs) const {
            return price == rhs.price ? order_id < rhs.order_id : price < rhs.price;
        }
        bool operator>(const PriceAndId & rhs) const {
            return price == rhs.price ? order_id < rhs.order_id : price > rhs.price;
        }
    };
    using Volume = int;

    int price_double2int(double price);

    double price_int2double(int price);

    // TODO Perf: 调参 miniheap_size
    template <typename CMP>
    using PQ = rollbear::prio_queue<1024 /* miniheap_size */,
            PriceAndId /* Key first*/,
            Volume /* Value second */, CMP>;
    using BID_PQ = PQ<std::greater<PriceAndId>>; // 再想想.jpg 我去麦当劳(不是)
    using std::unordered_map;

    class Worker {
    public:
        /* 决赛数据包含714个交易日的数据
         * 每个交易日包含100-150支合约，每支合约大约400000～480000笔历史单和480个信号
         * */
        struct BidsAndAsks {
            BID_PQ bid;
            BID_PQ ask;
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
                static int id = 0;
                std::cout << "[" << id++ << "]" << "cur_position: " << cur_position << " " << cash << " " << (double)cur_position * price_int2double(last_price) -
                (double)last_position * price_int2double(prev_close_price) + (cash)/100.0 << std::endl;
                return (double)cur_position * price_int2double(last_price) + cash / 100.0
                - (double)last_position * price_int2double(prev_close_price);
            }
        };

        // TODO Perf: 调 Hash
        // char[8] 不知道看做 long long int 来 hash 会不会快点，他可能就取模
        unordered_map<unsigned long long int, BidsAndAsks> umap;

        Worker();

        void process_prev_trade(prev_trade_info prev_trade_infos[], size_t n);
        void calc_pnl_and_pos(prev_trade_info prevTradeInfo[], pnl_and_pos pnl_and_poses[], size_t n);
        BidsAndAsks & get_instrument(unsigned long long instrument_id);
        int get_bid(BidsAndAsks & bids_and_asks);
        int get_ask(BidsAndAsks & bids_and_asks);
        void output_pnl_and_pos(size_t prev_trades_size, string date, int session_number, int session_length);
        void output_twap_order(twap_order twap_orders[], size_t twap_size, string date, int session_number, int session_length);
        // 基准价格
        int get_base_price(BidsAndAsks & bid_and_asks, const order_log & order);
        inline void add_one_dir(BidsAndAsks & bids_and_asks, int volume, int direction);
        inline void reduce_one_dir(BidsAndAsks & bids_and_asks, int volume, int direction);
        unsigned long long char8_to_ull(const char * instrument_id);

        int get_price(const order_log & order, bool is_alpha);
        int process_fix_price_order(const order_log& order, bool is_alpha, int price, BID_PQ& pq, BidsAndAsks& bidsAndAsks);
        int process_dynamic_price_order_level_5(const order_log& order, BID_PQ& pq, BidsAndAsks& bidsAndAsks);
        int make_order(const order_log & order, bool is_alpha = false);
    };
};

#endif //MAIN_CHUO_H
