#include "chuo.h"
#include <algorithm>
#include <cmath>

namespace Chuo {

int price_double2int(double price) { // price只有2位小数， *100四舍五入
    return (int)round(price * 100);

}

double price_int2double(int price) { // price只有2位小数， /100
    return (double)price / 100;
}


Worker::Worker() {
    umap.reserve(3513);
}

// 加载昨日收盘价格
void Worker::process_prev_trade(prev_trade_info prev_trade_infos[], size_t n) {
    for (int i = 0; i < n; i++) {
        auto & bid_and_asks = get_instrument(char8_to_ull(prev_trade_infos[i].instrument_id));
        bid_and_asks.prev_close_price = price_double2int(prev_trade_infos[i].prev_close_price);
        bid_and_asks.cur_position = bid_and_asks.last_position = prev_trade_infos[i].prev_position;
        bid_and_asks.up_limit = price_double2int(prev_trade_infos[i].prev_close_price * 1.1);
        bid_and_asks.down_limit = price_double2int(prev_trade_infos[i].prev_close_price * 0.9);
    }
}

void Worker::calc_pnl_and_pos(prev_trade_info prevTradeInfo[], pnl_and_pos pnl_and_poses[], size_t n) {
    for (int i = 0; i < n; i++) {
        memcpy(pnl_and_poses[i].instrument_id, prevTradeInfo[i].instrument_id, 8);
        auto & bid_and_asks = get_instrument(char8_to_ull(pnl_and_poses[i].instrument_id));
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
    std::cout << filename << std::endl;
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
        std::cout << "open file failed" << std::endl;
        return;
    }
    auto & pnl_and_poses = Singleton::get_instance().pnl_and_poses;
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
    std::cout << filename << std::endl;
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
        std::cout << "open file failed" << std::endl;
        return;
    }
    fwrite(twap_orders, sizeof(twap_order), twap_size, fp);
    fclose(fp);
}

Worker::BidsAndAsks & Worker::get_instrument(unsigned long long instrument_id) {
    return umap[instrument_id];
}

int Worker::get_bid(BidsAndAsks & bids_and_asks) {
    return bids_and_asks.bid.top().first.price;
}

int Worker::get_ask(BidsAndAsks & bids_and_asks) {
    return -bids_and_asks.ask.top().first.price;
}

// 基准价格
int Worker::get_base_price(BidsAndAsks & bid_and_asks, const order_log & order) {
    // 本方买入
    if (order.direction == 1) {
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
    } else {
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
}

inline unsigned long long Worker::char8_to_ull(const char * instrument_id) {
    return *(unsigned long long *)(instrument_id);
}


static int debug_msg = 0;
const int debug_limit = 140;

int Worker::get_price(const order_log & order, bool is_alpha) {
    auto & bids_and_asks = get_instrument(char8_to_ull(order.instrument_id));
    switch (order.type) {
        case 0:
        {
            // 限价申报 需要 check 0.98 和 1.02
            int base_price = get_base_price(bids_and_asks, order);

            if (is_alpha) { // 特判策略单
                return base_price;
            }

            // 0.98~1.02
            int price_off = price_double2int(order.price_off);
            if (price_off + base_price < bids_and_asks.down_limit) {
                return -1;
            }
            if (price_off + base_price > bids_and_asks.up_limit) {
                return -1;
            }
            if (order.direction == 1) {
                if (price_off + base_price > round(1.02 * base_price)) {
                    return -1;
                }
            } else {
                if (price_off + base_price < round(0.98 * base_price)) {
                    return -1;
                }
            }
            return base_price + price_off; // 限价申报的价格, 入队.
        }
        case 1: // 对手方最优价格申报, 订单可能入队
        {
            if (order.direction == 1) {
                // 买入对手方 --> ask
                if (bids_and_asks.ask.size() == 0) {
                    return -1;
                }
                return get_ask(bids_and_asks);
            } else {
                // 卖出对手方 --> bid
                if (bids_and_asks.bid.size() == 0) {
                    return -1;
                }
                return get_bid(bids_and_asks);
            }
        }
        case 2: // 本方最优价格申报, 订单可能入队
        {
            if (order.direction == 1) {
                // 买入本方 --> bid
                if (bids_and_asks.bid.size() == 0) {
                    return -1;
                }
                return get_bid(bids_and_asks);
            } else {
                // 卖出本方 --> ask
                if (bids_and_asks.ask.size() == 0) {
                    return -1;
                }
                return get_ask(bids_and_asks);
            }
        }
        case 3: // 最优五档即时成交剩余撤销, 可以成交一部分就不返回-1.
        {
            return 0;
        }
        case 4: // 即时成交剩余撤销申报, 可以成交一部分就不返回-1.
        {
            return 0;
        }
        case 5: // 全额成交或撤销申报, 可以全部成交才不返回-1.
        {
            if (order.direction == 1) {
                // 剩余卖 < 买
                if (bids_and_asks.ask_sum_volume < order.volume) {
                    return -1;
                } else {
                    // 可行
                    return 0;
                }
            } else {
                // 剩余买 < 卖
                if (bids_and_asks.bid_sum_volume < order.volume) {
                    return -1;
                } else {
                    // 可行
                    return 0;
                }
            }
        }
    }
}

inline void Worker::add_one_dir(BidsAndAsks & bids_and_asks, int volume, int direction) {
    direction == 1 ?
        bids_and_asks.bid_sum_volume += volume:
        bids_and_asks.ask_sum_volume += volume;
}

inline void Worker::reduce_one_dir(BidsAndAsks & bids_and_asks, int volume, int direction) {
    direction == 1 ? bids_and_asks.bid_sum_volume -= volume : bids_and_asks.ask_sum_volume -= volume;
}

int Worker::process_fix_price_order(const order_log& order, bool is_alpha, int price, BID_PQ& pq, BidsAndAsks& bidsAndAsks) { // 限价 / 对手方最优 / 本方最优; 没有使用order price.
    int volume = order.volume,  direction = order.direction;

    // pq与order的dir相反.
    while(pq.size() && volume > 0) {
        const auto & x = pq.top();
        if ( (direction == 1 && -x.first.price > price) || // 卖方价格 > 限价
                (direction == -1 && x.first.price < price) // 买方价格 < 限价
        ) {  // 没做成, 直接挂单. [适用于本方最优 与 限价单]
            return volume;
        }
        bidsAndAsks.last_price = abs(x.first.price); // 更新最近成交价.
        if (x.second > volume) {
            x.second -= volume; // 更新堆顶的volume.
            reduce_one_dir(bidsAndAsks, volume, -direction); // 更新总量, 反向减少;
            if(x.first.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位;
                bidsAndAsks.cur_position -= volume * direction, bidsAndAsks.cash += direction * volume * abs(x.first.price); // 当前仓位是dir的反向; i.e., dir = 1, 堆顶的策略卖单成交了;
            if(is_alpha)
                bidsAndAsks.cur_position += volume * direction, bidsAndAsks.cash -= direction * volume * abs(x.first.price); // 如果当前order是策略单, 则直接有影响;
            return 0;
        } else { // x.second < volume.
            volume -= x.second;
            reduce_one_dir(bidsAndAsks, x.second, -direction); // 更新总量, 反向减少;
            if(x.first.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位;
                bidsAndAsks.cur_position -= x.second * direction, bidsAndAsks.cash += direction * x.second * abs(x.first.price);
            if(is_alpha)
                bidsAndAsks.cur_position += x.second * direction, bidsAndAsks.cash -= direction * x.second * abs(x.first.price);
            pq.pop();
        }
    }
    return volume; // 返回还剩多少没成交;
}

int Worker::process_dynamic_price_order_level_5(const order_log& order, BID_PQ& pq, BidsAndAsks& bidsAndAsks) {
    unordered_map<int, bool>mp = unordered_map<int, bool>();
    // 比较order.instrument_id, 输出“000.UBE”对应的order用于debug.
    int volume = order.volume,  direction = order.direction, cnt = 0;
    // pq与order的dir相反.
    while(pq.size() && volume > 0) {
        const auto & x = pq.top();
        if(mp.find(abs(x.first.price)) == mp.end()) {
            mp[abs(x.first.price)] = true;
            cnt ++;
        }

        if(order.type == 3 && cnt > 5) break; // type==3 只成交五次
        bidsAndAsks.last_price = abs(x.first.price); // 更新最近成交价.
        if (x.second > volume) {
            x.second -= volume;
            reduce_one_dir(bidsAndAsks, volume, -direction); // 更新总量, 反向减少;
            if(x.first.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位;
                bidsAndAsks.cur_position -= volume * direction, bidsAndAsks.cash += direction * volume * abs(x.first.price); // 当前仓位是dir的反向; i.e., dir = 1, 堆顶的策略卖单成交了;
            return 0;
        } else {
            volume -= x.second;
            reduce_one_dir(bidsAndAsks, x.second, -direction); // 更新总量, 反向减少;
            if(x.first.order_id & 1) // 堆内的time stamp是策略单的, 则需要更新仓位;
                bidsAndAsks.cur_position -= x.second * direction, bidsAndAsks.cash += direction * x.second * abs(x.first.price);
            pq.pop();
        }
    }
    return volume; // 返回还剩多少没成交;
}

int Worker::make_order(const order_log & order, bool is_alpha /* = false */) {
    // 原始order(意味着timestamp是原始的).
    // 模拟单条历史订单 并把不能撮合的加入堆中;
    auto & bids_and_asks = get_instrument(char8_to_ull(order.instrument_id));
    int price = get_price(order, is_alpha); // 如果是alpha单, 则会返回base price.
    if (price == -1) {
        return -1; // 价格检查失败;
    }
    auto& pq_diff = (order.direction == -1) ? bids_and_asks.bid : bids_and_asks.ask;
    auto& pq_same = (order.direction != -1) ? bids_and_asks.bid : bids_and_asks.ask;
    // 卖出时需要检验买1, 买入时需要检验卖1
    switch (order.type) {
        case 0:
        {
            int deal_volume = process_fix_price_order(order, is_alpha, price, pq_diff, bids_and_asks); // 需要买order.volume.
            if (deal_volume > 0) {
                // 当前order还有剩余, 需要加入堆中.
                pq_same.push(std::move(PriceAndId{order.direction * price, static_cast<int>(order.timestamp<<1|is_alpha)}),
                             deal_volume);
                add_one_dir(bids_and_asks, deal_volume, order.direction); // 更新总量, 同向增加;
            }
            if (is_alpha) {
                return price;
            }
            break;
        }
        case 1: // 对手方最优价格申报, 订单可能入队
        {
            int deal_volume = process_fix_price_order(order, is_alpha, price, pq_diff, bids_and_asks);
            if (deal_volume > 0) {
                // 当前order还有剩余, 需要加入堆中.
                pq_same.push(std::move(PriceAndId{order.direction * price, static_cast<int>(order.timestamp<<1|is_alpha)}),
                             deal_volume);
                add_one_dir(bids_and_asks, deal_volume, order.direction); // 更新总量, 同向增加;
            }
            break;
        }
        case 2: // 本方最优价格申报
        {
            int deal_volume = process_fix_price_order(order, is_alpha, price, pq_diff, bids_and_asks);
            if(deal_volume > 0) {
                // 当前order还有剩余, 需要加入堆中.
                pq_same.push(std::move(PriceAndId{order.direction*price, static_cast<int>(order.timestamp<<1|is_alpha)}),
                             deal_volume);
                add_one_dir(bids_and_asks, deal_volume, order.direction); // 更新总量, 同向增加;
            }
            break;
        }
        case 3: // 最优五档即时成交剩余撤销, 可以成交一部分就不返回-1.
        {
            int vol = process_dynamic_price_order_level_5(order, pq_diff, bids_and_asks); // 剩余撤销了;
            break;
        }
        case 4: // 即时成交剩余撤销申报, 可以成交一部分就不返回-1.
        {
            int vol = process_dynamic_price_order_level_5(order, pq_diff, bids_and_asks); // 剩余撤销了;
            break;
        }
        case 5: // 全额成交或撤销申报, 可以全部成交才不返回-1.
        {
            // 到这里已经检验了仓位一定可行;
            int vol = process_dynamic_price_order_level_5(order, pq_diff, bids_and_asks); // 剩余撤销了;
            break;
        }
    }
    return 0;
}

};

/* int getbid() & int getask() are used to get the best bid and ask price
 * * 在访问PQ时, 不断弹出空订单, 直到遇到非空订单, 返回其价格. */
// int getprice() is used to return the price of the order. <在内部讨论5种定价方案>
// void make_order() is used to make an order. (加入撮合系统进行撮合)
// void revert_order() is used to revert an order. (从撮合系统中撤回订单)
// (因为某种原因: 1. 交易失败，部分撤销 2. 交易失败，全部撤销 3. 收盘的限价委托，策略单全部取消)
/*
 * orders流 ->
 * alpha流->返回一个orders的vector.
 * */
