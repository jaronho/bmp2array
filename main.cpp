#include <stdlib.h>
#include <stdio.h>
#include "bitmap/bitmap.h"
#include "cmdline/cmdline.h"

std::vector<std::string> stripFileInfo(const std::string& filePath) {
    std::string dirname = "", filename = filePath, basename = "", extname = "";
    size_t pos = filePath.find_last_of("/\\");
    if (pos < filePath.size()) {
        dirname = filePath.substr(0, pos + 1);
        filename = filePath.substr(pos + 1, filePath.size() - 1);
    }
    pos = filename.find_last_of(".");
    if (pos < filename.size()) {
        basename = filename.substr(0, pos);
        extname = filename.substr(pos, filename.size() - 1);
    } else {
        basename = filename;
    }
    std::vector<std::string> infos;
    infos.push_back(dirname);
    infos.push_back(filename);
    infos.push_back(basename);
    infos.push_back(extname);
    return infos;
}

bool writeDataToFile(const unsigned char* data, long dataSize, const std::string& filePath) {
    FILE* fp = fopen(filePath.c_str(), "wb");
    if (!fp) {
        return false;
    }
    if (data && dataSize > 0) {
        fwrite(data, dataSize, sizeof(unsigned char), fp);
        fflush(fp);
    }
    fclose(fp);
    return true;
}

int main(int argc, char* argv[]) {
    /* step1: 参数解析 */
    cmdline::parser a;
    a.add<std::string>("file", 'f', "Bitmap file name (Required)", true, "");
    a.add<std::string>("output", 'o', "Output file name (Optional)", false, "");
    a.add<int>("bit", 'b', "Color bits (Optional), [16, 24]", false, 16, cmdline::range(16, 24));
    a.add<int>("type", 't', "Output file type (Optional), [1.raw data, 2.h file]", false, 2, cmdline::range(1, 2));
    a.parse_check(argc, argv);
    std::string file = a.get<std::string>("file");
    std::string output = a.get<std::string>("output");
    int bit = a.get<int>("bit");
    int type = a.get<int>("type");
    std::vector<std::string> fileInfo = stripFileInfo(file);
    if (output.empty()) {
        std::string sufix = ".output";
        if (2 == type) {
            sufix = ".h";
        }
        output = fileInfo[2] + sufix;
    }
    /* step2: 打开文件 */
    Bitmap bmp;
    int ret = bmp.open(file);
    if (1 == ret) {
        printf("Cannot open bitmap file \"%s\"\n", file.c_str());
        return 0;
    } else if (2 == ret) {
        printf("File \"%s\" is not in proper BMP format\n", file.c_str());
        return 0;
    } else if (3 == ret) {
        printf("Only support for 24-bit bitmap file\n");
        return 0;
    } else if (4 == ret) {
        printf("Only support uncompressed bitmap file\n");
        return 0;
    }
    /* step3: 写入文件 */
    if (1 == type) {
        if (16 == bit) {
            bmp.toFile(output, 3);
        } else if (24 == bit) {
            bmp.toFile(output, 2);
        }
    } else if (2 == type) {
        std::string arrayBuffer;
        arrayBuffer += "static const unsigned int bmp_" + fileInfo[2] + "_width = " + std::to_string(bmp.width()) + ";";
        arrayBuffer += "\n";
        arrayBuffer += "static const unsigned int bmp_" + fileInfo[2] + "_height = " + std::to_string(bmp.height()) + ";";
        arrayBuffer += "\n";
        if (16 == bit) {
            arrayBuffer += "static const unsigned short bmp_" + fileInfo[2] + "_raw_data[] = {";
        } else if (24 == bit) {
            arrayBuffer += "static const unsigned int bmp_" + fileInfo[2] + "_raw_data[] = {";
        }
        PixelMatrix pixels = bmp.toPixelMatrix();
        unsigned int bmpSize = (unsigned int)bmp.width() * (unsigned int)bmp.height();
        for (int i = 0; i < bmp.height(); ++i) {
            if (i > 0) {
                arrayBuffer += ",";
            }
            arrayBuffer += "\n    ";
            for (int j = 0; j < bmp.width(); ++j) {
                if (j > 0) {
                    arrayBuffer += ",";
                }
                arrayBuffer += std::to_string(16 == bit ? pixels[i][j].rgb16() : pixels[i][j].rgb32());
            }
        }
        arrayBuffer += "\n";
        arrayBuffer += "};";
        if (!writeDataToFile((const unsigned char*)arrayBuffer.c_str(), arrayBuffer.size(), output)) {
            printf("Cannot write to file \"%s\"\n", output.c_str());
        }
    }
    return 0;
}
