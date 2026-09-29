#include "XmlHelper.h"
#include "tinyxml2/tinyxml2.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <algorithm>
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
    if (std::string(element->Name()) != segment) return defaultValue;

    while (std::getline(ss, segment, '.')) {
        element = element->FirstChildElement(segment.c_str());
        if (!element) return defaultValue;
    }

    // For new format, value is in the "value" attribute
    const char* attrVal = element->Attribute("value");
    return attrVal ? attrVal : defaultValue;
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
    if (std::string(element->Name()) != segment) return defaultValue;

    while (std::getline(ss, segment, '.')) {
        element = element->FirstChildElement(segment.c_str());
        if (!element) return defaultValue;
    }

    // Try new user-friendly format first (nested elements with value attributes)
    XmlHelper::Color result = defaultValue;
    
    if (auto* redElement = element->FirstChildElement("red")) {
        if (redElement->Attribute("value")) result.r = std::stoi(redElement->Attribute("value"));
    }
    if (auto* greenElement = element->FirstChildElement("green")) {
        if (greenElement->Attribute("value")) result.g = std::stoi(greenElement->Attribute("value"));
    }
    if (auto* blueElement = element->FirstChildElement("blue")) {
        if (blueElement->Attribute("value")) result.b = std::stoi(blueElement->Attribute("value"));
    }
    if (auto* alphaElement = element->FirstChildElement("alpha")) {
        if (alphaElement->Attribute("value")) result.a = std::stoi(alphaElement->Attribute("value"));
    }
    
    // Fallback to old attribute format for backwards compatibility
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
    if (std::string(element->Name()) != segment) return;

    while (std::getline(ss, segment, '.')) {
        tinyxml2::XMLElement* next = element->FirstChildElement(segment.c_str());
        if (!next) {
            next = element->InsertNewChild(segment.c_str());
        }
        element = next;
    }

    // Set value attribute (new format)
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
    if (std::string(element->Name()) != segment) return;

    while (std::getline(ss, segment, '.')) {
        tinyxml2::XMLElement* next = element->FirstChildElement(segment.c_str());
        if (!next) {
            next = element->InsertNewChild(segment.c_str());
        }
        element = next;
    }

    // Write in new user-friendly format (nested elements with value attributes)
    auto* redElement = element->FirstChildElement("red");
    if (!redElement) {
        redElement = element->InsertNewChild("red");
    }
    redElement->attributes["value"] = std::to_string(value.r);
    
    auto* greenElement = element->FirstChildElement("green");
    if (!greenElement) {
        greenElement = element->InsertNewChild("green");
    }
    greenElement->attributes["value"] = std::to_string(value.g);
    
    auto* blueElement = element->FirstChildElement("blue");
    if (!blueElement) {
        blueElement = element->InsertNewChild("blue");
    }
    blueElement->attributes["value"] = std::to_string(value.b);
    
    auto* alphaElement = element->FirstChildElement("alpha");
    if (!alphaElement) {
        alphaElement = element->InsertNewChild("alpha");
    }
    alphaElement->attributes["value"] = std::to_string(value.a);
    
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
    bool formatUpgraded = false;
    tinyxml2::XMLElement* root = doc.RootElement();
    
    if (!root || std::string(root->Name()) != "MenuConfig") {
        Logger::Log("Invalid XML root element, deploying default...");
        DeployDefaultConfig(NULL);
        return;
    }
    
    // ═════════════════════════════════════════════════════════════════════
    // FIRST: Upgrade old format to new user-friendly format
    // ═════════════════════════════════════════════════════════════════════
    
    // Convert old color format (r/g/b/a attributes) to new format (nested elements)
    const char* colorElements[] = {"TitleBox", "SubtitleBox", "Background", "SelectionBar"};
    const char* textColorElements[] = {"TitleText", "SubtitleText", "CountText", "OptionNormal", "OptionSelected"};
    
    // Convert Frame colors
    tinyxml2::XMLElement* frameElement = root->FirstChildElement("Frame");
    if (frameElement) {
        for (const char* elemName : colorElements) {
            tinyxml2::XMLElement* colorElem = frameElement->FirstChildElement(elemName);
            if (colorElem) {
                // Check if it has old format attributes
                if (colorElem->Attribute("r") || colorElem->Attribute("g") || 
                    colorElem->Attribute("b") || colorElem->Attribute("a")) {
                    
                    Logger::Log("Converting old color format to new format for: Frame." + std::string(elemName));
                    
                    // Get old values or use defaults
                    const char* rVal = colorElem->Attribute("r");
                    const char* gVal = colorElem->Attribute("g");
                    const char* bVal = colorElem->Attribute("b");
                    const char* aVal = colorElem->Attribute("a");
                    
                    // Create new nested elements with value attributes
                    if (rVal) {
                        tinyxml2::XMLElement* redElem = colorElem->InsertNewChild("red");
                        redElem->attributes["value"] = rVal;
                    }
                    if (gVal) {
                        tinyxml2::XMLElement* greenElem = colorElem->InsertNewChild("green");
                        greenElem->attributes["value"] = gVal;
                    }
                    if (bVal) {
                        tinyxml2::XMLElement* blueElem = colorElem->InsertNewChild("blue");
                        blueElem->attributes["value"] = bVal;
                    }
                    if (aVal) {
                        tinyxml2::XMLElement* alphaElem = colorElem->InsertNewChild("alpha");
                        alphaElem->attributes["value"] = aVal;
                    }
                    
                    // Remove old attributes
                    colorElem->DeleteAttribute("r");
                    colorElem->DeleteAttribute("g");
                    colorElem->DeleteAttribute("b");
                    colorElem->DeleteAttribute("a");
                    
                    formatUpgraded = true;
                    repairsMade = true;
                }
            }
        }
    }
    
    // Convert TextColors
    tinyxml2::XMLElement* textColorsElement = root->FirstChildElement("TextColors");
    if (textColorsElement) {
        for (const char* elemName : textColorElements) {
            tinyxml2::XMLElement* colorElem = textColorsElement->FirstChildElement(elemName);
            if (colorElem) {
                if (colorElem->Attribute("r") || colorElem->Attribute("g") || 
                    colorElem->Attribute("b") || colorElem->Attribute("a")) {
                    
                    Logger::Log("Converting old color format to new format for: TextColors." + std::string(elemName));
                    
                    const char* rVal = colorElem->Attribute("r");
                    const char* gVal = colorElem->Attribute("g");
                    const char* bVal = colorElem->Attribute("b");
                    const char* aVal = colorElem->Attribute("a");
                    
                    if (rVal) {
                        tinyxml2::XMLElement* redElem = colorElem->InsertNewChild("red");
                        redElem->attributes["value"] = rVal;
                    }
                    if (gVal) {
                        tinyxml2::XMLElement* greenElem = colorElem->InsertNewChild("green");
                        greenElem->attributes["value"] = gVal;
                    }
                    if (bVal) {
                        tinyxml2::XMLElement* blueElem = colorElem->InsertNewChild("blue");
                        blueElem->attributes["value"] = bVal;
                    }
                    if (aVal) {
                        tinyxml2::XMLElement* alphaElem = colorElem->InsertNewChild("alpha");
                        alphaElem->attributes["value"] = aVal;
                    }
                    
                    colorElem->DeleteAttribute("r");
                    colorElem->DeleteAttribute("g");
                    colorElem->DeleteAttribute("b");
                    colorElem->DeleteAttribute("a");
                    
                    formatUpgraded = true;
                    repairsMade = true;
                }
            }
        }
    }
    
    // Convert Layout settings (X/Y to horizontal_position/vertical_position)
    tinyxml2::XMLElement* layoutElement = root->FirstChildElement("Layout");
    if (layoutElement) {
        tinyxml2::XMLElement* xElem = layoutElement->FirstChildElement("X");
        tinyxml2::XMLElement* yElem = layoutElement->FirstChildElement("Y");
        
        if (xElem && xElem->Attribute("value")) {
            Logger::Log("Converting old layout format: X -> horizontal_position");
            tinyxml2::XMLElement* newPosElem = layoutElement->InsertNewChild("horizontal_position");
            newPosElem->attributes["value"] = xElem->Attribute("value");
            // Remove old element
            auto it = std::find(layoutElement->children.begin(), layoutElement->children.end(), xElem);
            if (it != layoutElement->children.end()) {
                layoutElement->children.erase(it);
                delete xElem;
            }
            formatUpgraded = true;
            repairsMade = true;
        }
        
        if (yElem && yElem->Attribute("value")) {
            Logger::Log("Converting old layout format: Y -> vertical_position");
            tinyxml2::XMLElement* newPosElem = layoutElement->InsertNewChild("vertical_position");
            newPosElem->attributes["value"] = yElem->Attribute("value");
            // Remove old element
            auto it = std::find(layoutElement->children.begin(), layoutElement->children.end(), yElem);
            if (it != layoutElement->children.end()) {
                layoutElement->children.erase(it);
                delete yElem;
            }
            formatUpgraded = true;
            repairsMade = true;
        }
    }
    
    // Convert General settings (IntStep/FloatStep to integer_step/float_step)
    tinyxml2::XMLElement* generalElement = root->FirstChildElement("General");
    if (generalElement) {
        tinyxml2::XMLElement* intStepElem = generalElement->FirstChildElement("IntStep");
        tinyxml2::XMLElement* floatStepElem = generalElement->FirstChildElement("FloatStep");
        
        if (intStepElem && intStepElem->Attribute("value")) {
            Logger::Log("Converting old general format: IntStep -> integer_step");
            tinyxml2::XMLElement* newStepElem = generalElement->InsertNewChild("integer_step");
            newStepElem->attributes["value"] = intStepElem->Attribute("value");
            // Remove old element
            auto it = std::find(generalElement->children.begin(), generalElement->children.end(), intStepElem);
            if (it != generalElement->children.end()) {
                generalElement->children.erase(it);
                delete intStepElem;
            }
            formatUpgraded = true;
            repairsMade = true;
        }
        
        if (floatStepElem && floatStepElem->Attribute("value")) {
            Logger::Log("Converting old general format: FloatStep -> float_step");
            tinyxml2::XMLElement* newStepElem = generalElement->InsertNewChild("float_step");
            newStepElem->attributes["value"] = floatStepElem->Attribute("value");
            // Remove old element
            auto it = std::find(generalElement->children.begin(), generalElement->children.end(), floatStepElem);
            if (it != generalElement->children.end()) {
                generalElement->children.erase(it);
                delete floatStepElem;
            }
            formatUpgraded = true;
            repairsMade = true;
        }
    }
    
    if (formatUpgraded) {
        Logger::Log("XML format upgraded to user-friendly format, preserving user settings!");
    }
    
    // ═════════════════════════════════════════════════════════════════════
    // SECOND: Validate and add missing elements
    // ═════════════════════════════════════════════════════════════════════
    
    // Define required XML structure
    struct RequiredElement {
        const char* path;
        const char* attribute;
        const char* defaultValue;
        bool isColorElement; // New format: nested elements for colors
    };
    
    RequiredElement requiredElements[] = {
        // Frame styling (new user-friendly format)
        {"Frame.TitleBox", "red", "184", true},
        {"Frame.TitleBox", "green", "162", true},
        {"Frame.TitleBox", "blue", "57", true},
        {"Frame.TitleBox", "alpha", "255", true},
        {"Frame.SubtitleBox", "red", "0", true},
        {"Frame.SubtitleBox", "green", "0", true},
        {"Frame.SubtitleBox", "blue", "0", true},
        {"Frame.SubtitleBox", "alpha", "255", true},
        {"Frame.Background", "red", "0", true},
        {"Frame.Background", "green", "0", true},
        {"Frame.Background", "blue", "0", true},
        {"Frame.Background", "alpha", "200", true},
        {"Frame.SelectionBar", "red", "255", true},
        {"Frame.SelectionBar", "green", "255", true},
        {"Frame.SelectionBar", "blue", "255", true},
        {"Frame.SelectionBar", "alpha", "255", true},
        
        // Text colors (new user-friendly format)
        {"TextColors.TitleText", "red", "255", true},
        {"TextColors.TitleText", "green", "255", true},
        {"TextColors.TitleText", "blue", "255", true},
        {"TextColors.TitleText", "alpha", "255", true},
        {"TextColors.SubtitleText", "red", "184", true},
        {"TextColors.SubtitleText", "green", "162", true},
        {"TextColors.SubtitleText", "blue", "57", true},
        {"TextColors.SubtitleText", "alpha", "255", true},
        {"TextColors.CountText", "red", "184", true},
        {"TextColors.CountText", "green", "162", true},
        {"TextColors.CountText", "blue", "57", true},
        {"TextColors.CountText", "alpha", "255", true},
        {"TextColors.OptionNormal", "red", "255", true},
        {"TextColors.OptionNormal", "green", "255", true},
        {"TextColors.OptionNormal", "blue", "255", true},
        {"TextColors.OptionNormal", "alpha", "255", true},
        {"TextColors.OptionSelected", "red", "0", true},
        {"TextColors.OptionSelected", "green", "0", true},
        {"TextColors.OptionSelected", "blue", "0", true},
        {"TextColors.OptionSelected", "alpha", "255", true},
        
        // Layout settings (new user-friendly format)
        {"Layout.horizontal_position", "value", "0.15", false},
        {"Layout.vertical_position", "value", "0.10", false},
        
        // General settings (new user-friendly format)
        {"General.integer_step", "value", "5", false},
        {"General.float_step", "value", "0.1", false}
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
            if (element.isColorElement) {
                // For color elements, check if nested element exists
                tinyxml2::XMLElement* colorChild = current->FirstChildElement(element.attribute);
                if (!colorChild) {
                    colorChild = current->InsertNewChild(element.attribute);
                    colorChild->attributes["value"] = element.defaultValue;
                    Logger::Log("Added missing XML color element: " + path + " " + element.attribute + " = " + element.defaultValue);
                    repairsMade = true;
                }
            } else {
                // For regular elements, check if attribute exists
                if (!current->Attribute(element.attribute)) {
                    current->attributes[element.attribute] = element.defaultValue;
                    Logger::Log("Added missing XML attribute: " + path + " " + element.attribute + " = " + element.defaultValue);
                    repairsMade = true;
                }
            }
        }
    }
    
    if (repairsMade) {
        doc.SaveFile(XmlPath.c_str());
        Logger::Log("XML file repaired successfully!");
        
        if (formatUpgraded) {
            // Notify user about format upgrade (only if menu system is initialized)
            IniHelper::ShowNotification("~g~Menu config upgraded to new format!");
            IniHelper::ShowNotification("~w~Your custom settings were preserved.");
        }
    } else {
        Logger::Log("XML file validation passed - no repairs needed");
    }
}
