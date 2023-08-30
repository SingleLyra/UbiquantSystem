#include "common.h"
#include "readlogs.h"
#include "chuo.h"
#include <algorithm>
#include <cstring>
#include <cmath>

using namespace std;

//
// $ main.out date session_number session_length
//
int main(int argc, char * argv[]) {
    string date(argv[1]);
    int session_number = stoi(argv[2]);
    int session_length = stoi(argv[3]);

    size_t prev_trades_size = read_prev_trade_info(date);
    size_t alphas_size = read_alpha(date);

    //
    // 计算策略单
    //
    size_t twaps_size = 0;
    unordered_map<string, int /* Volume */> instrument_volume;
    // 1. load 昨日仓位
    for (int i = 0; i < prev_trades_size; i++) {
        string instrument(prev_trade_infos[i].instrument_id);
        instrument_volume[instrument] = prev_trade_infos[i].prev_position;
    }
    // 2. 信号生成策略单
    for (int i = 0; i < alphas_size; i++) {
        const auto & it = instrument_volume.find(string(alphas[i].instrument_id));
        int last_volume = it->second;
        int target_volume = alphas[i].target_volume;
        for (int j = 0; j < session_number; j++) {
            memcpy(twap_orders[twaps_size].instrument_id, alphas[i].instrument_id, sizeof(alphas[i].instrument_id));
            if (target_volume > last_volume) {
                twap_orders[twaps_size].volume = int(1.0*(target_volume - last_volume)*(j+1)/ session_number) - int(1.0*(target_volume - last_volume)*(j)/ session_number);
                twap_orders[twaps_size].direction = 1;
                twap_orders[twaps_size].timestamp = alphas[i].timestamp + j * session_length * 1000;
            } else if (target_volume < last_volume) {
                twap_orders[twaps_size].volume = int(1.0*(last_volume - target_volume)*(j+1)/ session_number) - int(1.0*(last_volume - target_volume)*(j)/ session_number);
                twap_orders[twaps_size].direction = -1;
                twap_orders[twaps_size].timestamp = alphas[i].timestamp + j * session_length * 1000;
            } else if (target_volume == last_volume) {
                continue;
            }
            twaps_size++;
        }
        it->second = target_volume;
    }
    // 3. 策略单排序
    sort(twap_orders, twap_orders + twaps_size, [](const twap_order & l, const twap_order & r) {
        return l.timestamp < r.timestamp;
    });

    //
    // 模拟 order_log
    //
    Chuo::Worker worker;
    worker.process_prev_trade(prev_trade_infos, prev_trades_size);

    size_t now_twap_order_id = 0;
    bool end = false;
    start_read_order(date);
    while (!end) {
        size_t order_len = read_order_log(BATCH_SIZE);
        // 判断是否读完
        if (order_len < BATCH_SIZE) {
            end = true;
        }
        for(int i = 0; i < order_len; i++) {
            order_log& order = order_logs[i];
            while (now_twap_order_id < twaps_size && twap_orders[now_twap_order_id].timestamp < order.timestamp) {
                twap_order& twapOrder = twap_orders[now_twap_order_id];
                order_log order_log1 = order_log {
                    .timestamp =  twapOrder.timestamp,
                    .type = 0,
                    .direction = twapOrder.direction,
                    .volume = twapOrder.volume,
                };
                memcpy(order_log1.instrument_id, twapOrder.instrument_id, 8);
                int price = worker.make_order(order_log1, true);
                twapOrder.price = Chuo::price_int2double(price);
                now_twap_order_id++;
            }
            worker.make_order(order, false);
        }
    }
    worker.calc_pnl_and_pos(prev_trade_infos, pnl_and_poses, worker.umap.size());
    worker.output_pnl_and_pos(prev_trades_size, date, session_number, session_length);
    worker.output_twap_order(twap_orders, twaps_size, date, session_number, session_length);
    return 0;
}