#include "common.h"
#include "readlogs.h"
#include "chuo.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fenv.h>
// #include <numaif.h>

using namespace std;

/*
static void bind_numa(){
    if (numa_available() < 0) {
        std::cerr << "NUMA is not available on this system." << std::endl;
        return 1;
    }

    // Set the preferred node and bind the current thread to it
    int preferredNode = 0;  // Change this to the desired NUMA node
    numa_set_preferred(preferredNode);

    if (numa_run_on_node(preferredNode) < 0) {
        std::cerr << "Failed to bind thread to NUMA node." << std::endl;
        return 1;
    }

}*/

//
// $ main.out date
//

struct OneSessionWorker {
    int session_number;
    int session_length;
    size_t prev_trades_size;
    size_t alphas_size;
    twap_order twap_orders[MAX_TWAP_ORDERS];
    size_t twaps_size = 0;
    size_t now_twap_order_id = 0;
    Chuo::Worker worker;

    void session_set(int number, int length) {
        session_number = number;
        session_length = length;
    }

    void calc_twap_order_volume(alpha * alphas) {
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
    }

    void work_order(const order_log & order) {
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

    void output_answer(const string & date) {
        worker.calc_pnl_and_pos(prev_trade_infos, pnl_and_poses, prev_trades_size);
        worker.output_pnl_and_pos(prev_trades_size, date, session_number, session_length);
        worker.output_twap_order(twap_orders, twaps_size, date, session_number, session_length);
    }
};

OneSessionWorker worker[5];
std::queue<std::vector<order_log>> orderQueue;
std::mutex mtx;
std::condition_variable cv;

void read_orders(const string & date) {
    start_read_order(date);
    bool end = false;
    while (!end) {
        std::vector<order_log> batch;
        size_t order_len = read_order_log(batch, BATCH_SIZE);
        // 判断是否读完
        if (order_len < BATCH_SIZE) {
            end = true;
        }
        std::unique_lock<std::mutex> lock(mtx);
        orderQueue.push(std::move(batch));
        lock.unlock();
        cv.notify_one();
    }
    // send empty batch means END
    std::vector<order_log> batch;
    std::unique_lock<std::mutex> lock(mtx);
    orderQueue.push(std::move(batch));
    lock.unlock();
    cv.notify_one();
}

void deal_orders(const string & date) {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return !orderQueue.empty(); });
        std::vector<order_log> batch = orderQueue.front();
        orderQueue.pop();
        lock.unlock();

        if (batch.size() == 0) {
            break;
        }

        int size = batch.size();
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < 5; j++) {
                worker[j].work_order(batch[i]);
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        worker[i].output_answer(date);
    }
}

int main(int argc, char * argv[]) {
    string date(argv[1]);
    fesetround(FE_TONEAREST); // use rint replace round, faster

    size_t prev_trades_size = read_prev_trade_info(date);
    size_t alphas_size = read_alpha(date);

    for (int i = 0; i < 5; i++) {
        worker[i].alphas_size = alphas_size;
        worker[i].prev_trades_size = prev_trades_size;
    }
    worker[0].session_set(3, 1);
    worker[1].session_set(3, 3);
    worker[2].session_set(3, 5);
    worker[3].session_set(5, 2);
    worker[4].session_set(5, 3);

    //
    // 计算策略单
    //
    for (int i = 0; i < 5; i++) {
        worker[i].calc_twap_order_volume(alphas);
        worker[i].worker.process_prev_trade(prev_trade_infos, prev_trades_size);
    }

    std::thread readerThread(read_orders, date);
    std::thread workerThread(deal_orders, date);

    readerThread.join();
    workerThread.join();

    return 0;
}