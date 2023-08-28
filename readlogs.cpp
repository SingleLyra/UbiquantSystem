#include <dirent.h>
#include <cstring>
#include "readlogs.h"

void read_order_log(const string& filename, vector<order_log>& order_logs, int batch_size, int start_pos) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (fp == nullptr) {
        std::cout << "Error: cannot open file " << filename << std::endl;
        exit(1);
    }
    fseek(fp, start_pos * sizeof(order_log), SEEK_SET);
    order_log* buffer = &Singleton::get_instance().order_logs[0]; // Todo: parralelize this.
    size_t read_size = fread(buffer, sizeof(order_log), BATCH_SIZE, fp);
    for (int i = 0; i < read_size; ++i) {
        order_logs.emplace_back(buffer[i]);
    }
    fclose(fp);
}

void get_dir_files(const string& dir, vector<string>& files) { // 找出当前dir目录下有哪些文件.
    DIR* dp;
    struct dirent* dirp;
    if ((dp = opendir(dir.c_str())) == nullptr) {
        std::cout << "Error: cannot open directory " << dir << std::endl;
        exit(1);
    }
    while ((dirp = readdir(dp)) != nullptr) {
        if (dirp->d_type == DT_REG) {
            files.emplace_back(dir + dirp->d_name);
        }
        else if(dirp->d_type == DT_DIR && strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0) {
            string subdir = dir + dirp->d_name + "/";
            get_dir_files(subdir, files);
        }
        else
            continue;
    }
    closedir(dp);
}