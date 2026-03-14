#include "IniHelper.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include "resource.h"
#include "main.h"
#include "natives.h"
#include "Logger.h"

namespace fs = std::filesystem;

std::string IniHelper::IniPath = "";

void IniHelper::Initialize(HMODULE hModule) {
    // Use LOCALAPPDATA for TornadoVStuff
    char* localappdata = getenv("LOCALAPPDATA");
    
    // Path: TornadoVStuff\TornadoV.ini
    fs::path iniPath = fs::path(localappdata) / "TornadoVStuff" / "TornadoV.ini";
    
    IniPath = iniPath.string();
    
    if (!fs::exists(IniPath)) {
        DeployDefaultConfig(hModule);
    } else {
        // Validate and repair existing INI file
        ValidateAndRepairConfig();
    }
}

void IniHelper::DeployDefaultConfig(HMODULE hModule) {
    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(IDR_INI_DEFAULT), (LPCSTR)RT_RCDATA);
    if (hRes) {
        HGLOBAL hData = LoadResource(hModule, hRes);
        if (hData) {
            DWORD size = SizeofResource(hModule, hRes);
            void* ptr = LockResource(hData);
            
            // Ensure directory exists
            fs::path p(IniPath);
            fs::create_directories(p.parent_path());

            std::ofstream outFile(IniPath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write((const char*)ptr, size);
                outFile.close();
                return;
            }
        }
    }
    
    // Fallback if resource extraction fails
    ShowNotification("~r~Tornado V: Failed to deploy default configuration!");
}

void IniHelper::WriteValue(const std::string& section, const std::string& key, const std::string& value) {
    WritePrivateProfileStringA(section.c_str(), key.c_str(), value.c_str(), IniPath.c_str());
}

void IniHelper::ValidateAndRepairConfig() {
    bool repairsMade = false;
    
    // Define all required settings with their default values
    struct IniSetting {
        std::string section;
        std::string key;
        std::string defaultValue;
    };
    
    std::vector<IniSetting> requiredSettings = {
        // KeyBinds section
        {"KeyBinds", "KeybindsEnabled", "true"},
        {"KeyBinds", "ToggleMenu", "F5"},
        {"KeyBinds", "ToggleTornado", "F6"},
        
        // Vortex section
        {"Vortex", "MovementEnabled", "true"},
        {"Vortex", "MoveSpeedScale", "1.0"},
        {"Vortex", "MaxEntitySpeed", "45.0"},
        {"Vortex", "MaxEntityDistance", "57.0"},
        {"Vortex", "MaxEntityCount", "200"},
        {"Vortex", "HorizontalForceScale", "2.0"},
        {"Vortex", "VerticalForceScale", "1.6"},
        {"Vortex", "VortexRadius", "9.4"},
        {"Vortex", "RotationSpeed", "2.4"},
        {"Vortex", "ReverseRotation", "false"},
        {"Vortex", "TornadoSpawnDistance", "100.0"},
        {"Vortex", "FollowPlayer", "true"},
        {"Vortex", "SpawnInFront", "true"},
        {"Vortex", "TornadoMaxDistance", "1000.0"},
        
        // VortexAdvanced section
        {"VortexAdvanced", "MaxParticleLayers", "47"},
        {"VortexAdvanced", "ParticlesPerLayer", "9"},
        {"VortexAdvanced", "LayerSeparationAmount", "22.0"},
        {"VortexAdvanced", "CloudTopEnabled", "true"},
        {"VortexAdvanced", "CloudTopParticlesEnabled", "true"},
        {"VortexAdvanced", "ParticleMod", "false"},
        {"VortexAdvanced", "SurfaceDetectionEnabled", "true"},
        {"VortexAdvanced", "UseInternalPool", "true"},
        {"VortexAdvanced", "ParticleName", "ent_amb_smoke_foundry"},
        {"VortexAdvanced", "ParticleAsset", "core"},
        
        // Other section
        {"Other", "Notifications", "true"},
        {"Other", "SpawnInStorm", "true"},
        {"Other", "AffectPlayer", "true"},
        {"Other", "EnableEAS", "true"},
        {"Other", "EnableSirens", "true"},
        {"Other", "EnableTornadoSound", "true"},
        {"Other", "SirenVolume", "1.0"},
        {"Other", "TornadoVolume", "1.0"},
        {"Other", "EasVolume", "1.0"},
        {"Other", "LodDistance", "500.0"},
        {"Other", "AddBlip", "true"}
    };
    
    // Check each setting and repair if missing
    for (const auto& setting : requiredSettings) {
        char buffer[255];
        GetPrivateProfileStringA(setting.section.c_str(), setting.key.c_str(), "", buffer, 255, IniPath.c_str());
        
        if (std::string(buffer).empty()) {
            // Setting is missing, repair it
            WritePrivateProfileStringA(setting.section.c_str(), setting.key.c_str(), setting.defaultValue.c_str(), IniPath.c_str());
            repairsMade = true;
            Logger::Log("Repaired missing INI setting: [" + setting.section + "] " + setting.key + " = " + setting.defaultValue);
        }
    }
    
    if (repairsMade) {
        ShowNotification("~g~Tornado V: INI file repaired successfully!");
        Logger::Log("INI validation and repair completed");
    }
}

void IniHelper::ShowNotification(const std::string& message) {
    UI::_SET_NOTIFICATION_TEXT_ENTRY(const_cast<char*>("STRING"));
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(message.c_str()));
    UI::_DRAW_NOTIFICATION(false, true);
}
