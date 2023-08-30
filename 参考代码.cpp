#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

const int NUM_BATCH = 10;

struct order_log {
    char instrument_id[8];
    long timestamp;
    int type;
    int direction;
    int volume;
    double price_off;
}__attribute__((packed)); // orders of each day.

std::mutex mtx;
std::condition_variable cv;
std::queue<std::vector<order_log>> orderQueue;

static order_log* buffer = new order_log[NUM_BATCH];
void read_orders(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return;
    }

    while (!file.eof()) {
        std::vector<order_log> batch;
        batch.reserve(NUM_BATCH);
        // 一次读取 NUM_BATCH个
        file.read((char*)buffer, NUM_BATCH * sizeof(order_log));
        for(int i = 0; i < NUM_BATCH; i++) {
            batch.emplace_back(buffer[i]);
        }
        std::unique_lock<std::mutex> lock(mtx);
        orderQueue.push(std::move(batch));
        lock.unlock();
        cv.notify_one();
    }
}

void deal_orders() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return !orderQueue.empty(); });

        std::vector<order_log> batch = orderQueue.front();
        orderQueue.pop();
        lock.unlock();

        // Process the batch of orders using deal_orders()
        // You should implement this part
    }
}

int main() {
    std::string filename = "order_logs.bin"; // Change this to the actual file path
    std::thread readerThread(read_orders, filename);
    std::thread workerThread(deal_orders);

    readerThread.join();
    workerThread.join();

    return 0;
}

for () {
    order[100000] = read_order;
    thread1.worker.make_order;
    thread2.worker.make_order;
    thread3.worker.make_order;
    thread4.worker.make_order;
    thread5.worker.make_order;
    wait? // 不对吧, read可以一直做prefetch
}

// zhe yang?
// 我想想
// 切了个交易日咋办
// 是不是不用管， 切了个交易日-> thread0读完了-> 有个核是空闲的 -> 瓶颈不在盘?
// 不对 可以提前读一个文件似乎

// if read block slow -> thread0 slower than thread1~5
// ./main 2012
// wait thread 1~5
// ./main 2013 ---> thread0的队列是空的, 但thread0在等, 没有读
/*
 *    -- read -- |---|---------- wait for cpu --  --read|---|
 *               |---|cpu-----------------------        |---|cpu---
 *               |overlap|
 *    提前读下一天的文件
 *    -- read -- |---|---------- wait for --read|---|
 *               |---|cpu-----------------------|---|cpu---
 * */
// 单个程序里面
// read是一直在做的
// 好像很难实现
// 好像还好吧? 可能需要清零一下pq?
// 对于每个order丢进去, 然后thread0给（1~5）一个特殊的信号, 用来判断是否读完

// 那代码就要这样了

// 交易日不并行
// ...
thread0() { // 就做这个吧，不维护队列了就，每次预读一个交易日，这样好写不少
    for(day : days) {
        order[100000] = read_order;
        order1.push(order[100000])
        ...
        order5.push(order[100000])

        if (end) {
            order1.push(next_day)
        }
    }
}

// this?
// this sames nice..
// 我在想有没有可能thread0变成一个读队列的情况，好像不太可能
// 如果变成读队列它可能可以自己处理一些order, 但之后它就空闲掉了
// 甚至有可能最后和现在速度一样（
// 是不是最快也就5线程啊 yes cpu is 4 core -> 意味着没办法5进程
// 5 thread 好写 嗯 可以试试
// 可以试试
// 难道要搞发数据到server b.............

thread1() {
    for () {
        if (read_from_order1 == next_day) {
            output
            Chuo::worker.clear()
        } else {
            order[100000] = read_from_order1;
            make_order
        }
    }
}
...
thread5() {
    for () {
        order[100000] = read_from_order5;
        make_order
    }
}

