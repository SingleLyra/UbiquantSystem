#include "iostream"
#include "common.h"
#include "readlogs.h"
#include "chuo.h"
#include <algorithm>
#include <cstring>
#include <cmath>

using namespace std;

#define debugmode

#ifdef debugmode
static int debug_cnt = 0;
#endif

//
// $ main.out session_number session_length
//
int main(int argc, char * argv[]) {
    int session_number = stoi(argv[1]);
    int session_length = stoi(argv[2]);
    vector<string> dates = get_dir_files(Singleton::get_instance().data_path);

    for (auto & date: dates) {
        // prev_tade_info 一次读完
        size_t prev_trades_size = read_prev_trade_info(Singleton::get_instance().data_path + date + "/prev_trade_info", BATCH_SIZE, 0);
        // alpha 一次读完
        size_t alphas_size = read_alpha(Singleton::get_instance().data_path + date + "/alpha", BATCH_SIZE, 0);

        // 计算策略单
        prev_trade_info * prev_trades = Singleton::get_instance().prev_trade_infos;
        alpha * alphas = Singleton::get_instance().alphas;
        twap_order * twaps = Singleton::get_instance().twap_orders;
        unordered_map<string, int /* Volume */> instrument_volume;
        size_t twaps_size = 0;
        // 1. load 昨日仓位
        for (int i = 0; i < prev_trades_size; i++) {
            string instrument = string(prev_trades[i].instrument_id);
            instrument_volume[instrument] = prev_trades[i].prev_position;
        }
        // 2. 信号生成策略单
        for (int i = 0; i < alphas_size; i++) {
            string instrument = string(alphas[i].instrument_id);
            auto it = instrument_volume.find(instrument);
            // 之前有信号
            int last_volume = it->second;
            int target_volume = alphas[i].target_volume;
            for (int j = 0; j < session_number; j++) {
                memcpy(twaps[twaps_size].instrument_id, alphas[i].instrument_id, sizeof(alphas[i].instrument_id));
                if (target_volume > last_volume) {
                    // 买入
                    twaps[twaps_size].volume = int(1.0*(target_volume - last_volume)*(j+1)/ session_number) - int(1.0*(target_volume - last_volume)*(j)/ session_number);
                    // org: (target_volume - last_volume) / session_number;
                    twaps[twaps_size].direction = 1;
                    // 时间戳单位是毫秒, * 1000
                    twaps[twaps_size].timestamp = alphas[i].timestamp + j * session_length * 1000;
                } else if (target_volume < last_volume) {
                    // 卖出
                    twaps[twaps_size].volume = int(1.0*(last_volume - target_volume)*(j+1)/ session_number) - int(1.0*(last_volume - target_volume)*(j)/ session_number);
                    twaps[twaps_size].direction = -1;
                    // 时间戳单位是毫秒, * 1000
                    twaps[twaps_size].timestamp = alphas[i].timestamp + j * session_length * 1000;
                } else if (target_volume == last_volume) {
                    continue;
                }
                twaps_size++;
            }
            it->second = target_volume;
        }
        // 3. 策略单排序
        sort(twaps, twaps + twaps_size, [](const twap_order & l, const twap_order & r) {
            return l.timestamp < r.timestamp;
        });

        Chuo::Worker worker;
        // 加载昨日收盘信息
        worker.process_prev_trade(Singleton::get_instance().prev_trade_infos, prev_trades_size);
        // order 边读边处理
        string order_filename = Singleton::get_instance().data_path + date + "/order_log";
        size_t now_order_id = 0, now_twap_order_id = 0;
        order_log * orders = Singleton::get_instance().order_logs;
        bool end = false;
        while (!end) {
            size_t order_len = read_order_log(order_filename, BATCH_SIZE, now_order_id);
            // 判断是否读完
            if (order_len < BATCH_SIZE) {
                end = true;
            }
            // 按照时间顺序处理 make_order
            for(int i = 0; i < order_len; i++) {
                order_log& order = orders[i];
                while (now_twap_order_id < twaps_size && twaps[now_twap_order_id].timestamp < order.timestamp) {
                    twap_order& twapOrder = twaps[now_twap_order_id];
                    order_log order_log1 = order_log();
                    order_log1.timestamp = twapOrder.timestamp;
                    order_log1.type = 0;
                    order_log1.direction = twapOrder.direction;
                    order_log1.volume = twapOrder.volume;
                    memcpy(order_log1.instrument_id, twapOrder.instrument_id, 8); // 8个char.
                    int price = worker.make_order(order_log1, true);
                    twapOrder.price = Chuo::price_int2double(price);
                    now_twap_order_id++;
                }
                worker.make_order(order, false);
            }
            now_order_id += BATCH_SIZE;
        }
        worker.calc_pnl_and_pos(Singleton::get_instance().prev_trade_infos, Singleton::get_instance().pnl_and_poses, worker.umap.size());
        worker.output_pnl_and_pos(prev_trades_size, date, session_number, session_length);
        worker.output_twap_order(twaps, twaps_size, date, session_number, session_length);

        return 0;
    }
    return 0;
}