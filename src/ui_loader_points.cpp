#include "ui_loader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

namespace ImGui
{
    bool load_points(const char* path, Points* out_points)
    {
        std::ifstream file(path, std::ios::binary);

        out_points->clear();

        // 파일 열기
        if (!file.is_open()) {
            // [파일이 안열림]
            cout << "[Error] [Loader] 파일 열기 실패: " << path << endl;
            return false;
        }

        std::string line;
        int numVertices     = 0;
        int propertyCount   = 0;
        bool isBinary       = false;

        // 헤더 파싱
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line.find("format binary_little_endian") != std::string::npos) {
                isBinary = true;
            } else if (line.find("element vertex") != std::string::npos) {
                sscanf(line.c_str(), "element vertex %d", &numVertices);
            } else if (line.find("property float") != std::string::npos) {
                propertyCount++;
            } else if (line == "end_header") {
                break;
            }
        }

        // 예외 처리
        if (numVertices == 0) {
            cout << "[Error] [Loader] 데이터가 없음: " << numVertices << " points" << endl;
            return false;
        }


        out_points->reserve(numVertices);

        //  데이터 파싱
        if (isBinary) {
            // [바이너리 파일]
            int bytesPerVertex = propertyCount * sizeof(float);
            std::vector<char> buffer(bytesPerVertex);

            for (int i = 0; i < numVertices; ++i) {
                file.read(buffer.data(), bytesPerVertex);
                if (!file) break;
                float* floats = reinterpret_cast<float*>(buffer.data());
                out_points->push_back({ floats[0], floats[1], floats[2] });
            }
        }
        else {
            // [텍스트 파일]
            for (int i = 0; i < numVertices; ++i) {
                if (!std::getline(file, line)) break;
                std::stringstream ss(line);
                Vector3 p;
                if (ss >> p.x >> p.y >> p.z) out_points->push_back(p);
            }
        }

        return true;
    }
}