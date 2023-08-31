#include "chuo.h"
#include "common.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <fenv.h>

namespace Chuo {
    // ======================================
    // =========== 调用频繁 ==================
    // ======================================

    int price_double2int(double price) { // price只有2位小数， *100四舍五入
        return rint(price * 100);
    }

    int round_mul(int price, int x) {
        int tmp = price * x;
        if (tmp % 100 >= 50) {
            return tmp / 100 + 1;
        } else {
            return tmp / 100;
        }
    }

    // ======================================
    // ======================================
    // ======================================

    double price_int2double(int price) { // price只有2位小数， /100
        return ((double) price) / 100;
    }

    Worker::Worker() {
        umap.reserve(3513);
    }

    void Worker::process_prev_trade(prev_trade_info prev_trade_infos[], size_t n) {
        for (int i = 0; i < n; i++) {
            auto & bid_and_asks = get_instrument(prev_trade_infos[i].instrument_id);
            bid_and_asks.prev_close_price = price_double2int(prev_trade_infos[i].prev_close_price);
            bid_and_asks.cur_position = bid_and_asks.last_position = prev_trade_infos[i].prev_position;
            bid_and_asks.up_limit = round_mul(bid_and_asks.prev_close_price, 110);
            bid_and_asks.down_limit = round_mul(bid_and_asks.prev_close_price, 90);
        }
    }

    void Worker::calc_pnl_and_pos(prev_trade_info prevTradeInfo[], pnl_and_pos pnl_and_poses[], size_t n) {
        for (int i = 0; i < n; i++) {
            memcpy(pnl_and_poses[i].instrument_id, prevTradeInfo[i].instrument_id, 8);
            auto & bid_and_asks = get_instrument(pnl_and_poses[i].instrument_id);
            pnl_and_poses[i].position = bid_and_asks.cur_position;
            pnl_and_poses[i].pnl = bid_and_asks.pnl();
        }
        std::sort(pnl_and_poses, pnl_and_poses + n, [this](const pnl_and_pos & l, const pnl_and_pos & r) {
            // 字典序sort
            return strcmp(l.instrument_id, r.instrument_id) < 0;
        });
    }

    void Worker::output_pnl_and_pos(size_t prev_trades_size, string date, int session_number, int session_length) {
        string filename = "/home/team9/pnl_and_pos/" + date + "_" + std::to_string(session_number) + "_" + std::to_string(session_length);
        FILE *fp = fopen(filename.c_str(), "wb");
        if (fp == nullptr) {
            std::cout << "open file failed" << std::endl;
            return;
        }
        fwrite(pnl_and_poses, sizeof(pnl_and_pos), prev_trades_size, fp);
        fclose(fp);
    }

    void Worker::output_twap_order(twap_order twap_orders[], size_t twap_size, string date, int session_number, int session_length) {
        std::sort(twap_orders, twap_orders + twap_size, [](const twap_order & l, const twap_order & r) {
            if (l.timestamp == r.timestamp)
                return strcmp(l.instrument_id, r.instrument_id) < 0;
            else
                return l.timestamp < r.timestamp;
        });
        string filename = "/home/team9/twap_order/" + date + "_" + std::to_string(session_number) + "_" + std::to_string(session_length);
        FILE *fp = fopen(filename.c_str(), "wb");
        if (fp == nullptr) {
            std::cout << "open file failed" << std::endl;
            return;
        }
        fwrite(twap_orders, sizeof(twap_order), twap_size, fp);
        fclose(fp);
    }

    // ======================================
    // =========== 调用频繁 ==================
    // ======================================

    Worker::BidsAndAsks & Worker::get_instrument(const char * instrument_id) {
        return umap[*(unsigned long long int *)instrument_id];
    }

    int Worker::get_bid(BidsAndAsks & bids_and_asks) const {
        return bids_and_asks.bid.top().price;
    }

    int Worker::get_ask(BidsAndAsks & bids_and_asks) const {
        return bids_and_asks.ask.top().price;
    }

    // 基准价格
    int Worker::BID_get_base_price(BidsAndAsks & bid_and_asks) {
        // 1. 最低卖出
        if (bid_and_asks.ask.size()) {
            return get_ask(bid_and_asks);
        }
        // 2. 最高买入
        if (bid_and_asks.bid.size()) {
            return get_bid(bid_and_asks);
        }
        // 3. 成交价格
        if (bid_and_asks.last_price != -1) {
            return bid_and_asks.last_price;
        }
        // 4. 昨日收盘
        return bid_and_asks.prev_close_price;
    }

    int Worker::ASK_get_base_price(BidsAndAsks & bid_and_asks) {
        // 1. 最高买入
        if (bid_and_asks.bid.size()) {
            return get_bid(bid_and_asks);
        }
        // 2. 最低卖出
        if (bid_and_asks.ask.size()) {
            return get_ask(bid_and_asks);
        }
        // 3. 成交价格
        if (bid_and_asks.last_price != -1 ) {
            return bid_and_asks.last_price;
        }
        // 4. 昨日收盘
        return bid_and_asks.prev_close_price;
    }

    int Worker::BID_process_fix_price_order(const order_log& order, bool is_alpha, int price, BidsAndAsks& bidsAndAsks) { // 限价 / 对手方最优 / 本方最优; 没有使用order price.
        ASK_PQ & ask = bidsAndAsks.ask;
        int volume = order.volume;

        while(ask.size() && volume > 0) {
            const auto & ask_top = ask.top();
            if (ask_top.price > price) {
                return volume;
            }

            int trade_price = ask_top.price;
            bidsAndAsks.last_price = trade_price;
            if (ask_top.volume > volume) {
                ask_top.volume -= volume;
                bidsAndAsks.ask_sum_volume -= volume;

                if(ask_top.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位
                    bidsAndAsks.cur_position -= volume, bidsAndAsks.cash += volume * trade_price;
                if(is_alpha)
                    bidsAndAsks.cur_position += volume, bidsAndAsks.cash -= volume * trade_price; // 如果当前order是策略单, 则直接有影响;

                return 0;
            } else {
                volume -= ask_top.volume;
                bidsAndAsks.ask_sum_volume -= ask_top.volume;

                if(ask_top.order_id & 1)
                    bidsAndAsks.cur_position -= ask_top.volume, bidsAndAsks.cash += ask_top.volume * trade_price;
                if(is_alpha)
                    bidsAndAsks.cur_position += ask_top.volume, bidsAndAsks.cash -= ask_top.volume * trade_price;

                ask.pop();
            }
        }
        return volume; // 返回还剩多少没成交;
    }

    int Worker::ASK_process_fix_price_order(const order_log& order, bool is_alpha, int price, BidsAndAsks& bidsAndAsks) { // 限价 / 对手方最优 / 本方最优; 没有使用order price.
        BID_PQ & bid = bidsAndAsks.bid;
        int volume = order.volume;

        while(bid.size() && volume > 0) {
            const auto & bid_top = bid.top();
            if (bid_top.price < price) {
                return volume;
            }

            int trade_price = bid_top.price;
            bidsAndAsks.last_price = trade_price;
            if (bid_top.volume > volume) {
                bid_top.volume -= volume;
                bidsAndAsks.bid_sum_volume -= volume;

                if(bid_top.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位
                    bidsAndAsks.cur_position += volume, bidsAndAsks.cash -= volume * trade_price;
                if(is_alpha)
                    bidsAndAsks.cur_position -= volume, bidsAndAsks.cash += volume * trade_price; // 如果当前order是策略单, 则直接有影响;

                return 0;
            } else {
                volume -= bid_top.volume;
                bidsAndAsks.bid_sum_volume -= bid_top.volume;

                if(bid_top.order_id & 1)
                    bidsAndAsks.cur_position += bid_top.volume, bidsAndAsks.cash -= bid_top.volume * trade_price;
                if(is_alpha)
                    bidsAndAsks.cur_position -= bid_top.volume, bidsAndAsks.cash += bid_top.volume * trade_price;

                bid.pop();
            }
        }
        return volume; // 返回还剩多少没成交;
    }

    int Worker::BID_process_dynamic_price_order_level_5(const order_log& order, BidsAndAsks& bidsAndAsks) {
        unordered_map<int, bool>mp = unordered_map<int, bool>();
        ASK_PQ & ask = bidsAndAsks.ask;
        int volume = order.volume, cnt = 0;

        while(ask.size() && volume > 0) {
            const auto & ask_top = ask.top();
            if (mp.find(ask_top.price) == mp.end()) {
                mp[ask_top.price] = true;
                cnt++;
            }

            // type==3 只成交五次
            if(order.type == 3 && cnt > 5) {
                break;
            }

            bidsAndAsks.last_price = ask_top.price; // 更新最近成交价
            if (ask_top.volume > volume) {
                ask_top.volume -= volume;
                bidsAndAsks.ask_sum_volume -= volume;

                if(ask_top.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位;
                    bidsAndAsks.cur_position -= volume, bidsAndAsks.cash += volume * ask_top.price; // 当前仓位是dir的反向; i.e., dir = 1, 堆顶的策略卖单成交了;

                return 0;
            } else {
                volume -= ask_top.volume;
                bidsAndAsks.ask_sum_volume -= ask_top.volume;

                if(ask_top.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位;
                    bidsAndAsks.cur_position -= ask_top.volume, bidsAndAsks.cash += ask_top.volume * ask_top.price;

                ask.pop();
            }
        }
        return volume; // 返回还剩多少没成交;
    }

    int Worker::ASK_process_dynamic_price_order_level_5(const order_log& order, BidsAndAsks& bidsAndAsks) {
        unordered_map<int, bool>mp = unordered_map<int, bool>();
        BID_PQ & bid = bidsAndAsks.bid;
        int volume = order.volume, cnt = 0;

        while(bid.size() && volume > 0) {
            const auto & bid_top = bid.top();
            if (mp.find(bid_top.price) == mp.end()) {
                mp[bid_top.price] = true;
                cnt ++;
            }

            // type==3 只成交五次
            if(order.type == 3 && cnt > 5) {
                break;
            }

            bidsAndAsks.last_price = bid_top.price; // 更新最近成交价.
            if (bid_top.volume > volume) {
                bid_top.volume -= volume;
                bidsAndAsks.bid_sum_volume -= volume;

                if(bid_top.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位;
                    bidsAndAsks.cur_position += volume, bidsAndAsks.cash -= volume * bid_top.price; // 当前仓位是dir的反向; i.e., dir = 1, 堆顶的策略卖单成交了;

                return 0;
            } else {
                volume -= bid_top.volume;
                bidsAndAsks.bid_sum_volume -= bid_top.volume;

                if(bid_top.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位;
                    bidsAndAsks.cur_position += bid_top.volume, bidsAndAsks.cash -= bid_top.volume * bid_top.price;

                bid.pop();
            }
        }

        return volume; // 返回还剩多少没成交;
    }

    int Worker::make_order(const order_log & order, bool is_alpha /* = false */) {
        if (order.direction == 1) {
            return BID_make_order(order, is_alpha);
        } else {
            return ASK_make_order(order, is_alpha);
        }
    }

    int Worker::BID_make_order(const order_log & order, bool is_alpha /* = false */) {
        BidsAndAsks & bids_and_asks = get_instrument(order.instrument_id);
        switch (order.type) {
            case 0:
            {
                int base_price = BID_get_base_price(bids_and_asks);
                int price = 0;

                // 特判策略单
                if (is_alpha) {
                    price = base_price;
                }
                    // 非策略单
                else {
                    int price_off_addbase = price_double2int(order.price_off) + base_price;
                    if (price_off_addbase < bids_and_asks.down_limit) {
                        return -1;
                    }
                    if (price_off_addbase> bids_and_asks.up_limit) {
                        return -1;
                    }
                    if (price_off_addbase > round_mul(base_price, 102)) {
                        return -1;
                    }
                    price = price_off_addbase;
                }

                int deal_volume = BID_process_fix_price_order(order, is_alpha, price, bids_and_asks);
                if (deal_volume > 0) {
                    // 当前order还有剩余, 需要加入堆中
                    bids_and_asks.bid.emplace(PriceAndIdAndVolume{price, static_cast<int>(order.timestamp << 1 | is_alpha), deal_volume});
                    bids_and_asks.bid_sum_volume += deal_volume;
                }
                if (is_alpha) {
                    return price;
                }
                break;
            }
            case 1: // 对手方最优价格申报, 订单可能入队
            {
                // 买入对手方 --> ask
                if (bids_and_asks.ask.size() == 0) {
                    return -1;
                }
                int price = get_ask(bids_and_asks);

                int deal_volume = BID_process_fix_price_order(order, is_alpha, price, bids_and_asks);
                if (deal_volume > 0) {
                    // 当前order还有剩余, 需要加入堆中.
                    bids_and_asks.bid.emplace(PriceAndIdAndVolume{price, static_cast<int>(order.timestamp << 1 | is_alpha), deal_volume});
                    bids_and_asks.bid_sum_volume += deal_volume;
                }
                break;
            }
            case 2: // 本方最优价格申报
            {
                if (bids_and_asks.bid.size() == 0) {
                    return -1;
                }
                int price = get_bid(bids_and_asks);
                int deal_volume = BID_process_fix_price_order(order, is_alpha, price, bids_and_asks);
                if (deal_volume > 0) {
                    // 当前order还有剩余, 需要加入堆中.
                    bids_and_asks.bid.emplace(PriceAndIdAndVolume{price, static_cast<int>(order.timestamp << 1 | is_alpha), deal_volume});
                    bids_and_asks.bid_sum_volume += deal_volume;
                }
                break;
            }
            case 3: // 最优五档即时成交剩余撤销, 可以成交一部分就不返回-1.
            {
                BID_process_dynamic_price_order_level_5(order, bids_and_asks); // 剩余撤销了;
                break;
            }
            case 4: // 即时成交剩余撤销申报, 可以成交一部分就不返回-1.
            {
                BID_process_dynamic_price_order_level_5(order, bids_and_asks); // 剩余撤销了;
                break;
            }
            case 5: // 全额成交或撤销申报, 可以全部成交才不返回-1.
            {
                // 剩余卖 < 买
                if (bids_and_asks.ask_sum_volume < order.volume) {
                    return -1;
                }
                // 到这里已经检验了仓位一定可行;
                BID_process_dynamic_price_order_level_5(order, bids_and_asks); // 剩余撤销了;
                break;
            }
        }
        return 0;
    }

    int Worker::ASK_make_order(const order_log & order, bool is_alpha /* = false */) {
        auto & bids_and_asks = get_instrument(order.instrument_id);
        switch (order.type) {
            case 0:
            {
                int base_price = ASK_get_base_price(bids_and_asks);
                int price = 0;

                // 特判策略单
                if (is_alpha) {
                    price = base_price;
                }
                    // 非策略单
                else {
                    int price_off_addbase = price_double2int(order.price_off) + base_price;
                    if (price_off_addbase < bids_and_asks.down_limit) {
                        return -1;
                    }
                    if (price_off_addbase> bids_and_asks.up_limit) {
                        return -1;
                    }
                    if (price_off_addbase < round_mul(base_price, 98)) {
                        return -1;
                    }
                    price = price_off_addbase;
                }

                int deal_volume = ASK_process_fix_price_order(order, is_alpha, price, bids_and_asks);
                if (deal_volume > 0) {
                    // 当前order还有剩余, 需要加入堆中
                    bids_and_asks.ask.emplace(PriceAndIdAndVolume{price, static_cast<int>(order.timestamp << 1 | is_alpha), deal_volume});
                    bids_and_asks.ask_sum_volume += deal_volume;
                }
                if (is_alpha) {
                    return price;
                }
                break;
            }
            case 1: // 对手方最优价格申报, 订单可能入队
            {
                // 卖出对手方 --> bid
                if (bids_and_asks.bid.size() == 0) {
                    return -1;
                }
                int price = get_bid(bids_and_asks);

                int deal_volume = ASK_process_fix_price_order(order, is_alpha, price, bids_and_asks);
                if (deal_volume > 0) {
                    // 当前order还有剩余, 需要加入堆中.
                    bids_and_asks.ask.emplace(PriceAndIdAndVolume{price, static_cast<int>(order.timestamp << 1 | is_alpha), deal_volume});
                    bids_and_asks.ask_sum_volume += deal_volume;
                }
                break;
            }
            case 2: // 本方最优价格申报
            {
                // 卖出本方 --> ask
                if (bids_and_asks.ask.size() == 0) {
                    return -1;
                }
                int price = get_ask(bids_and_asks);
                int deal_volume = ASK_process_fix_price_order(order, is_alpha, price, bids_and_asks);
                if (deal_volume > 0) {
                    // 当前order还有剩余, 需要加入堆中.
                    bids_and_asks.ask.emplace(PriceAndIdAndVolume{price, static_cast<int>(order.timestamp << 1 | is_alpha), deal_volume});
                    bids_and_asks.ask_sum_volume += deal_volume;
                }
                break;
            }
            case 3: // 最优五档即时成交剩余撤销, 可以成交一部分就不返回-1.
            {
                ASK_process_dynamic_price_order_level_5(order, bids_and_asks); // 剩余撤销了;
                break;
            }
            case 4: // 即时成交剩余撤销申报, 可以成交一部分就不返回-1.
            {
                ASK_process_dynamic_price_order_level_5(order, bids_and_asks); // 剩余撤销了;
                break;
            }
            case 5: // 全额成交或撤销申报, 可以全部成交才不返回-1.
            {
                // 剩余买 < 卖
                if (bids_and_asks.bid_sum_volume < order.volume) {
                    return -1;
                }
                // 到这里已经检验了仓位一定可行;
                ASK_process_dynamic_price_order_level_5(order, bids_and_asks); // 剩余撤销了;
                break;
            }
        }
        return 0;
    }

    // ======================================
    // ======================================
    // ======================================
};