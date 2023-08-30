#include <dirent.h>
#include <cstring>
#include <string>
#include <unordered_map>
#include "readlogs.h"

std::unordered_map<string, FILE *> files;
size_t read_order_log(const string& filename, int batch_size, int start_pos) {
    auto it = files.find(filename);
    FILE * fp = nullptr;
    if (it != files.end()) {
        fp = it->second;
    } else {
        fp = fopen(filename.c_str(), "rb");
        files[filename] = fp;
    }
    order_log* buffer = Singleton::get_instance().order_logs;
    size_t read_size = fread(buffer, sizeof(order_log), BATCH_SIZE, fp);
    return read_size;
}

size_t read_prev_trade_info(const string& filename, int batch_size, int start_pos) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (fp == nullptr) {
        std::cout << "Error: cannot open file " << filename << std::endl;
        exit(1);
    }
    fseek(fp, (long int)start_pos * sizeof(prev_trade_info), SEEK_SET);
    prev_trade_info* buffer = Singleton::get_instance().prev_trade_infos;
    size_t read_size = fread(buffer, sizeof(prev_trade_info), BATCH_SIZE, fp);
    fclose(fp);
    return read_size; 
}

size_t read_alpha(const string& filename, int batch_size, int start_pos) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (fp == nullptr) {
        std::cout << "Error: cannot open file " << filename << std::endl;
        exit(1);
    }
    fseek(fp, (long int)start_pos * sizeof(prev_trade_info), SEEK_SET);
    alpha* buffer = Singleton::get_instance().alphas;
    size_t read_size = fread(buffer, sizeof(alpha), BATCH_SIZE, fp);
    fclose(fp);
    return read_size;
}

// 读入 /mnt/data 下的文件夹 (files: 都是日期)
// example: [20250202, 20160202]
vector<string> get_dir_files(const string& dir) {
    DIR* dp;
    struct dirent* dirp;
    if ((dp = opendir(dir.c_str())) == nullptr) {
        std::cout << "Error: cannot open directory " << dir << std::endl;
        exit(1);
    }

    vector<string> files;
    while ((dirp = readdir(dp)) != nullptr) {
        if(dirp->d_type == DT_DIR && strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0) {
            files.emplace_back(dirp->d_name);
        }
    }

    closedir(dp);
    return files;
}