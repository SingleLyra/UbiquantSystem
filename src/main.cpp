#include "common.h"
#include "readlogs.h"
#include "chuo.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fenv.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

/* 用于网络传输 */
static std::string ipa = "172.28.142.150";
static std::string ipb = "172.28.142.148";
static int port_answer_send;
const int N = 3600;
char buffer[N];

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

    void calc_pnl_and_pos() {
        worker.calc_pnl_and_pos(prev_trade_infos, pnl_and_poses, prev_trades_size);
    }

    void output_answer_local(const string & date) {
        worker.output_pnl_and_pos(prev_trades_size, date, session_number, session_length);
        worker.output_twap_order(twap_orders, twaps_size, date, session_number, session_length);
    }

#ifdef NETWORK_SEND
    int output_answer_network(const string & date, int sockfd) {
        int ret = worker.output_twap_order_to_network(twap_orders, twaps_size, date, session_number, session_length, sockfd, N, buffer);
        if (ret == -1) {
            return -1;
        }
        ret = worker.output_pnl_and_pos_to_network(prev_trades_size, date, session_number, session_length, sockfd, N, buffer);
        if (ret == -1) {
            return -1;
        }
        return 0;
    }
#endif

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

#ifdef NETWORK_SEND
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_answer_send);
    inet_pton(AF_INET, ipb.c_str(), &serverAddr.sin_addr);
    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        // 没连接上 本地输出
        // 本地的文件都是没发过去的 dui
        // 那就在shell里面check下现在的文件夹
        perror("connect failed");
        for (int i = 0; i < 5; i++) {
            worker[i].calc_pnl_and_pos();
            worker[i].output_answer_local(date);
        }
    } else {
        // 网络发送
        printf("connect success\n");
        for (int i = 0; i < 5; i++) {
            worker[i].calc_pnl_and_pos();
            int ret = worker[i].output_answer_network(date, sockfd);
            if (ret == -1) {
                // send failed
                worker[i].output_answer_local(date);
            }
        }
        close(sockfd);
    }
#else
    for (int i = 0; i < 5; i++) {
        worker[i].calc_pnl_and_pos();
        worker[i].output_answer_local(date);
    }
#endif
}

int main(int argc, char * argv[]) {
#ifndef NETWORK_SEND
    cout << "warning: " << "not using network send" << endl;
#endif

    string date(argv[1]);
    fesetround(FE_TONEAREST); // use rint replace round, faster
    port_answer_send = {atoi(argv[2])};

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