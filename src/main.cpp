#include "cstdio"
#include "iostream"
#include "common.h"
#include "readlogs.h"

using namespace std;


int main() {
    vector<string> files;
    get_dir_files(Singleton::get_instance().data_path, files);
    for (auto& file: files) {
        cout << file << endl;
    }
    vector<order_log> order_logs;
    cout << "order logs:" << endl;
    cout << "sizeof order_log: " << sizeof(order_log) << endl; // 32
    for (auto& file: files) {
        if (file.find("order") != string::npos)
        {
            read_order_log(file, order_logs, 100, 0); // 读取order log. <第0条到第10条>
            // read_order_log(file, order_logs, 5, 5); // 读取order log. <第5条到第10条>
            break;
        }
    }
    for (auto& order_log: order_logs) {
        cout << order_log.instrument_id << " " << order_log.timestamp << " " << order_log.type << " " << order_log.direction << " " << order_log.volume << " " << order_log.price_off << endl;
    }
    return 0;
}