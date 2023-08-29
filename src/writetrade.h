//
// Created by Administrator on 2023/8/28.
//

#ifndef MAIN_WRITETRADE_H
#define MAIN_WRITETRADE_H

#include "common.h"
void write_pnl(const string& filename, vector<pnl_and_pos>& pnl_and_poses, int batch_size, int start_pos); // write trade to a file.
void write_twap(const string& filename, vector<twap_order>& twap_orders, int batch_size, int start_pos); // write twap to a file.

#endif //MAIN_WRITETRADE_H
