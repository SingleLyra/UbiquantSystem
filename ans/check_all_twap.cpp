#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

struct twap_order {
    char instrument_id[8];
    long timestamp;
    int direction;
    int volume;
    double price;
}__attribute__((packed));
bool compare_double(double a, double b) {
    return std::fabs(a - b) < 1e-6;
}

int main() {
    std::string sourceFolderPath = "/home/team9/twap_order/";
    std::string targetFolderPath = "/mnt/output_adjuest/twap_order/";

    for (const auto& sourceFileEntry : fs::directory_iterator(sourceFolderPath)) {
        if (!fs::is_regular_file(sourceFileEntry))
            continue;

        std::string sourceFileName = sourceFileEntry.path().filename().string();
        std::string targetFileName = targetFolderPath + sourceFileName;

        std::ifstream sourceFile(sourceFileEntry.path(), std::ios::binary);
        std::ifstream targetFile(targetFileName, std::ios::binary);

        if (!targetFile) {
            std::cout << "Target file not found: " << targetFileName << std::endl;
            continue;
        }

        std::vector<twap_order> sourceRecords, targetRecords;
        twap_order record;

        while (sourceFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            sourceRecords.push_back(record);
        }

        while (targetFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            targetRecords.push_back(record);
        }

        for (int i = 0; i < sourceRecords.size(); ++i) {
            twap_order sourceRecord = sourceRecords[i];
            twap_order targetRecord = targetRecords[i];
            if (std::strcmp(sourceRecord.instrument_id, targetRecord.instrument_id) != 0 ||
                sourceRecord.volume != targetRecord.volume ||
                sourceRecord.direction != targetRecord.direction ||
                sourceRecord.timestamp != targetRecord.timestamp ||
                !compare_double(sourceRecord.price, targetRecord.price)) {
                std::cout << sourceRecord.instrument_id << " " << sourceRecord.volume << " " << sourceRecord.direction << " " << sourceRecord.timestamp << " " << sourceRecord.price << std::endl;
                std::cout << targetRecord.instrument_id << " " << targetRecord.volume << " " << targetRecord.direction << " " << targetRecord.timestamp << " " << targetRecord.price << std::endl;
                std::cout << "Records are not identical: " << sourceFileName << " vs " << targetFileName << std::endl;
                return -1;
            }
        }
        std::cout << "File is identical: " << sourceFileName << " vs " << targetFileName << std::endl;
    }
    printf("All files are identical.\n");
    return 0;
}
