#pragma once
#include <string>
#include <vector>
#include <Windows.h>

class XmlHelper {
public:
    // Color helper
    struct Color {
        int r, g, b, a;
    };

    static void Initialize(HMODULE hModule = NULL);
    static void DeployDefaultConfig(HMODULE hModule);
    static void ValidateAndRepairXml();
    
    // Generic value getters
    static std::string GetString(const std::string& path, const std::string& defaultValue = "");
    static int GetInt(const std::string& path, int defaultValue = 0);
    static float GetFloat(const std::string& path, float defaultValue = 0.0f);
    static bool GetBool(const std::string& path, bool defaultValue = false);
    
    // Setters
    static void WriteValue(const std::string& path, const std::string& value);
    static void WriteColor(const std::string& path, Color value);
    
    static Color GetColor(const std::string& path, Color defaultValue = { 255, 255, 255, 255 });

    static std::string GetXmlPath() { return XmlPath; }

private:
    static std::string XmlPath;
};
