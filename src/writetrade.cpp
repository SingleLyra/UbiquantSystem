//
// Created by Administrator on 2023/8/28.
//

#include "writetrade.h"
// 输出: twap_order & pnl_and_pos 作为结果.
void write_trade(const string& filename, vector<pnl_and_pos>& pnl_and_poses,
                 int batch_size, int start_pos) {
    FILE* fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
        std::cout << "open file failed" << std::endl;
        return;
    }
    fseek(fp, start_pos * sizeof(pnl_and_pos), SEEK_SET);
    for (int i = start_pos; i < start_pos + batch_size; ++i) {
        fwrite(&pnl_and_poses[i], sizeof(pnl_and_pos), 1, fp);
    }
}

// 好丑; 但是不想改了.
void write_twap(const string& filename, vector<twap_order>& twap_orders,
                int batch_size, int start_pos) {
    FILE* fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
        std::cout << "open file failed" << std::endl;
        return;
    }
    fseek(fp, start_pos * sizeof(twap_order), SEEK_SET);
    for (int i = start_pos; i < start_pos + batch_size; ++i) {
        fwrite(&twap_orders[i], sizeof(twap_order), 1, fp);
    }
}