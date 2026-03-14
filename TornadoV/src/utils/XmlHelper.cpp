#include "XmlHelper.h"
#include "tinyxml2/tinyxml2.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include "resource.h"
#include "IniHelper.h" // For ShowNotification
#include "Logger.h"

namespace fs = std::filesystem;

std::string XmlHelper::XmlPath = "";

void XmlHelper::Initialize(HMODULE hModule) {
    // Use LOCALAPPDATA for TornadoVStuff
    char* localappdata = getenv("LOCALAPPDATA");
    
    // Path: TornadoVStuff\menu_config.xml
    fs::path xmlPath = fs::path(localappdata) / "TornadoVStuff" / "menu_config.xml";
    XmlPath = xmlPath.string();
    
    if (!fs::exists(XmlPath)) {
        DeployDefaultConfig(hModule);
    }
    
    // Validate and repair XML configuration
    ValidateAndRepairXml();
}

void XmlHelper::DeployDefaultConfig(HMODULE hModule) {
    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(IDR_XML_DEFAULT), (LPCSTR)RT_RCDATA);
    if (hRes) {
        HGLOBAL hData = LoadResource(hModule, hRes);
        if (hData) {
            DWORD size = SizeofResource(hModule, hRes);
            void* ptr = LockResource(hData);
            
            // Ensure directory exists
            fs::path p(XmlPath);
            fs::create_directories(p.parent_path());

            std::ofstream outFile(XmlPath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write((const char*)ptr, size);
                outFile.close();
                return;
            }
        }
    }
}

// Since we don't have tinyxml2 yet, these are stubs for now.
// I will implement the real logic once I've added tinyxml2 to the project.
std::string XmlHelper::GetString(const std::string& path, const std::string& defaultValue) {
    tinyxml2::XMLDocument doc;
    doc.LoadFile(XmlPath.c_str());
    
    tinyxml2::XMLElement* element = doc.RootElement();
    if (!element) return defaultValue;

    // Split path by dot: MenuConfig.Frame.TitleBox
    std::stringstream ss(path);
    std::string segment;
    std::getline(ss, segment, '.'); // Skip root if it matches
    if (element->name != segment) return defaultValue;

    while (std::getline(ss, segment, '.')) {
        element = element->FirstChildElement(segment.c_str());
        if (!element) return defaultValue;
    }

    const char* val = element->Attribute("value");
    return val ? val : defaultValue;
}

int XmlHelper::GetInt(const std::string& path, int defaultValue) {
    std::string val = GetString(path, "");
    if (val.empty()) return defaultValue;
    return std::stoi(val);
}

float XmlHelper::GetFloat(const std::string& path, float defaultValue) {
    std::string val = GetString(path, "");
    if (val.empty()) return defaultValue;
    return std::stof(val);
}

bool XmlHelper::GetBool(const std::string& path, bool defaultValue) {
    std::string val = GetString(path, "");
    if (val.empty()) return defaultValue;
    return (val == "true" || val == "1");
}

XmlHelper::Color XmlHelper::GetColor(const std::string& path, XmlHelper::Color defaultValue) {
    tinyxml2::XMLDocument doc;
    doc.LoadFile(XmlPath.c_str());
    
    tinyxml2::XMLElement* element = doc.RootElement();
    if (!element) return defaultValue;

    std::stringstream ss(path);
    std::string segment;
    std::getline(ss, segment, '.'); // Skip root if it matches
    if (element->name != segment) return defaultValue;

    while (std::getline(ss, segment, '.')) {
        element = element->FirstChildElement(segment.c_str());
        if (!element) return defaultValue;
    }

    XmlHelper::Color result = defaultValue;
    if (element->Attribute("r")) result.r = std::stoi(element->Attribute("r"));
    if (element->Attribute("g")) result.g = std::stoi(element->Attribute("g"));
    if (element->Attribute("b")) result.b = std::stoi(element->Attribute("b"));
    if (element->Attribute("a")) result.a = std::stoi(element->Attribute("a"));
    
    return result;
}

void XmlHelper::WriteValue(const std::string& path, const std::string& value) {
    tinyxml2::XMLDocument doc;
    doc.LoadFile(XmlPath.c_str());
    
    tinyxml2::XMLElement* element = doc.RootElement();
    if (!element) return;

    std::stringstream ss(path);
    std::string segment;
    std::getline(ss, segment, '.'); // Skip root if it matches
    if (element->name != segment) return;

    while (std::getline(ss, segment, '.')) {
        tinyxml2::XMLElement* next = element->FirstChildElement(segment.c_str());
        if (!next) {
            next = element->InsertNewChild(segment.c_str());
        }
        element = next;
    }

    element->attributes["value"] = value;
    doc.SaveFile(XmlPath.c_str());
}

void XmlHelper::WriteColor(const std::string& path, XmlHelper::Color value) {
    tinyxml2::XMLDocument doc;
    doc.LoadFile(XmlPath.c_str());
    
    tinyxml2::XMLElement* element = doc.RootElement();
    if (!element) return;

    std::stringstream ss(path);
    std::string segment;
    std::getline(ss, segment, '.'); // Skip root if it matches
    if (element->name != segment) return;

    while (std::getline(ss, segment, '.')) {
        tinyxml2::XMLElement* next = element->FirstChildElement(segment.c_str());
        if (!next) {
            next = element->InsertNewChild(segment.c_str());
        }
        element = next;
    }

    element->attributes["r"] = std::to_string(value.r);
    element->attributes["g"] = std::to_string(value.g);
    element->attributes["b"] = std::to_string(value.b);
    element->attributes["a"] = std::to_string(value.a);
    
    doc.SaveFile(XmlPath.c_str());
}

void XmlHelper::ValidateAndRepairXml() {
    Logger::Log("XML validation started...");
    
    tinyxml2::XMLDocument doc;
    doc.LoadFile(XmlPath.c_str());
    
    if (!doc.RootElement()) {
        Logger::Log("XML file corrupted or invalid, deploying default...");
        DeployDefaultConfig(NULL);
        return;
    }
    
    bool repairsMade = false;
    tinyxml2::XMLElement* root = doc.RootElement();
    
    if (!root || std::string(root->Name()) != "MenuConfig") {
        Logger::Log("Invalid XML root element, deploying default...");
        DeployDefaultConfig(NULL);
        return;
    }
    
    // Define required XML structure
    struct RequiredElement {
        const char* path;
        const char* attribute;
        const char* defaultValue;
    };
    
    RequiredElement requiredElements[] = {
        // Frame styling
        {"Frame.TitleBox", "r", "184"},
        {"Frame.TitleBox", "g", "162"},
        {"Frame.TitleBox", "b", "57"},
        {"Frame.TitleBox", "a", "255"},
        {"Frame.SubtitleBox", "r", "0"},
        {"Frame.SubtitleBox", "g", "0"},
        {"Frame.SubtitleBox", "b", "0"},
        {"Frame.SubtitleBox", "a", "255"},
        {"Frame.Background", "r", "0"},
        {"Frame.Background", "g", "0"},
        {"Frame.Background", "b", "0"},
        {"Frame.Background", "a", "200"},
        {"Frame.SelectionBar", "r", "255"},
        {"Frame.SelectionBar", "g", "255"},
        {"Frame.SelectionBar", "b", "255"},
        {"Frame.SelectionBar", "a", "255"},
        
        // Text colors
        {"TextColors.TitleText", "r", "255"},
        {"TextColors.TitleText", "g", "255"},
        {"TextColors.TitleText", "b", "255"},
        {"TextColors.TitleText", "a", "255"},
        {"TextColors.SubtitleText", "r", "184"},
        {"TextColors.SubtitleText", "g", "162"},
        {"TextColors.SubtitleText", "b", "57"},
        {"TextColors.SubtitleText", "a", "255"},
        {"TextColors.CountText", "r", "184"},
        {"TextColors.CountText", "g", "162"},
        {"TextColors.CountText", "b", "57"},
        {"TextColors.CountText", "a", "255"},
        {"TextColors.OptionNormal", "r", "255"},
        {"TextColors.OptionNormal", "g", "255"},
        {"TextColors.OptionNormal", "b", "255"},
        {"TextColors.OptionNormal", "a", "255"},
        {"TextColors.OptionSelected", "r", "0"},
        {"TextColors.OptionSelected", "g", "0"},
        {"TextColors.OptionSelected", "b", "0"},
        {"TextColors.OptionSelected", "a", "255"},
        
        // Layout settings
        {"Layout.X", "value", "0.15"},
        {"Layout.Y", "value", "0.10"},
        
        // General settings
        {"General.IntStep", "value", "5"},
        {"General.FloatStep", "value", "0.1"}
    };
    
    // Check each required element and attribute
    for (const auto& element : requiredElements) {
        tinyxml2::XMLElement* current = root;
        
        // Navigate to the element
        std::string path = element.path;
        std::stringstream ss(path);
        std::string segment;
        
        while (std::getline(ss, segment, '.')) {
            current = current->FirstChildElement(segment.c_str());
            if (!current) {
                // Element missing, create it
                Logger::Log("Missing XML element: " + path + ", creating...");
                
                // Navigate to parent and create missing element
                current = root;
                std::stringstream ss2(path);
                std::string segment2;
                std::string parentPath = "";
                
                while (std::getline(ss2, segment2, '.')) {
                    tinyxml2::XMLElement* next = current->FirstChildElement(segment2.c_str());
                    if (!next) {
                        next = current->InsertNewChild(segment2.c_str());
                        Logger::Log("Created XML element: " + parentPath + segment2);
                        repairsMade = true;
                    }
                    parentPath += segment2 + ".";
                    current = next;
                }
                break;
            }
        }
        
        if (current) {
            // Check if attribute exists
            if (!current->Attribute(element.attribute)) {
                current->attributes[element.attribute] = element.defaultValue;
                Logger::Log("Added missing XML attribute: " + path + " " + element.attribute + " = " + element.defaultValue);
                repairsMade = true;
            }
        }
    }
    
    if (repairsMade) {
        doc.SaveFile(XmlPath.c_str());
        Logger::Log("XML file repaired successfully!");
    } else {
        Logger::Log("XML file validation passed - no repairs needed");
    }
}
