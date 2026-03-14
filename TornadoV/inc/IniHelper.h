#pragma once
#include <string>
#include <vector>
#include <Windows.h>

class IniHelper {
public:
    static void Initialize(HMODULE hModule = NULL);
    static void DeployDefaultConfig(HMODULE hModule);
    static void ValidateAndRepairConfig();
    
    static void WriteValue(const std::string& section, const std::string& key, const std::string& value);
    
    template<typename T>
    static T GetValue(const std::string& section, const std::string& key, T defaultValue);

    static void ShowNotification(const std::string& message);
    static std::string GetIniPath() { return IniPath; }

private:
    static std::string IniPath;
};

// Template implementations
template<typename T>
inline T IniHelper::GetValue(const std::string& section, const std::string& key, T defaultValue) {
    char buffer[255];
    GetPrivateProfileStringA(section.c_str(), key.c_str(), "", buffer, 255, IniPath.c_str());
    
    std::string val(buffer);
    if (val.empty()) {
        return defaultValue;
    }

    try {
        if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else if constexpr (std::is_same_v<T, bool>) {
            return (val == "true" || val == "1" || val == "yes");
        } else if constexpr (std::is_same_v<T, int>) {
            return std::stoi(val);
        } else if constexpr (std::is_same_v<T, float>) {
            return std::stof(val);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(val);
        } else {
            return defaultValue;
        }
    } catch (...) {
        return defaultValue;
    }
}
