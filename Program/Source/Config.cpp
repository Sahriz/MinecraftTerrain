#include "Helpers/Config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cctype>

namespace {
    std::string Trim(const std::string& str) {
        auto first = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) { return std::isspace(ch); });
        auto last = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
        return (first < last) ? std::string(first, last) : "";
    }
}

bool Config::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Config] Failed to open config file: " << path << ", using defaults.\n";
        return false;
    }

    std::string line;
    std::string section = "";

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line[0] == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            std::transform(section.begin(), section.end(), section.begin(), [](unsigned char c) { return std::tolower(c); });
            continue;
        }

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }

        std::string key = Trim(line.substr(0, eqPos));
        std::string valStr = Trim(line.substr(eqPos + 1));
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });

        try {
            if (section == "world") {
                if (key == "view_distance") viewDistance = std::stoi(valStr);
                else if (key == "tick_rate") tickRate = std::stoi(valStr);
            }
            else if (section == "chunk") {
                if (key == "width") chunkWidth = std::stoi(valStr);
                else if (key == "height") chunkHeight = std::stoi(valStr);
                else if (key == "depth") chunkDepth = std::stoi(valStr);
            }
            else if (section == "player") {
                if (key == "speed") playerSpeed = std::stof(valStr);
                else if (key == "sprint_speed") playerSprintSpeed = std::stof(valStr);
                else if (key == "sensitivity") playerSensitivity = std::stof(valStr);
            }
            else if (section == "noise") {
                if (key == "octaves") noiseOctaves = std::stoi(valStr);
                else if (key == "frequency") noiseFrequency = std::stof(valStr);
                else if (key == "lacunarity") noiseLacunarity = std::stof(valStr);
                else if (key == "persistance" || key == "persistence") noisePersistence = std::stof(valStr);
                else if (key == "amplitude") noiseAmplitude = std::stof(valStr);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[Config] Parse error on line: " << line << " (" << e.what() << ")\n";
        }
    }

    std::cout << "[Config] Loaded configuration from " << path << "\n";
    return true;
}
