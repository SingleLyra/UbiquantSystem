#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

struct pnl_and_pos {
    char instrument_id[8];
    int position;
    double pnl;
} __attribute__((packed));

bool compare_double(double a, double b) {
    return std::fabs(a - b) < 1e-6;
}

int main() {
    std::string sourceFolderPath = "/home/team9/pnl_and_pos/";
    std::string targetFolderPath = "/mnt/output_adjuest/pnl_and_position/";

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

        std::vector<pnl_and_pos> sourceRecords, targetRecords;
        pnl_and_pos record;

        while (sourceFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            sourceRecords.push_back(record);
        }

        while (targetFile.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            targetRecords.push_back(record);
        }

        for (int i = 0; i < sourceRecords.size(); ++i) {
            pnl_and_pos sourceRecord = sourceRecords[i];
            pnl_and_pos targetRecord = targetRecords[i];
            if (std::strcmp(sourceRecord.instrument_id, targetRecord.instrument_id) != 0 ||
                sourceRecord.position != targetRecord.position ||
                !compare_double(sourceRecord.pnl, targetRecord.pnl)) {
                std::cout << compare_double(sourceRecord.pnl, targetRecord.pnl) << std::endl;
                printf("%.10lf %.10lf\n", sourceRecord.pnl, targetRecord.pnl);
                std::cout << sourceRecord.instrument_id << " " << sourceRecord.position << " " << targetRecord.position << " " << sourceRecord.pnl << " " << targetRecord.pnl << std::endl;
                std::cout << targetRecord.instrument_id << " " << sourceRecord.position << " " << targetRecord.position << " " << sourceRecord.pnl << " " << targetRecord.pnl << std::endl;
                std::cout << "Records are not identical: " << sourceFileName << " vs " << targetFileName << std::endl;
                return -1;
            }
        }
        std::cout << "Records are identical: " << sourceFileName << " vs " << targetFileName << std::endl;
    }
    printf("All files are identical.\n");
    return 0;
}
