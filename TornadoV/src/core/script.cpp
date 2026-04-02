#include "script.h"
#include "TornadoFactory.h"
#include "TornadoMenu.h"
#include "IniHelper.h"
#include "XmlHelper.h"
#include "MathEx.h"
#include "keyboard.h"
#include "Logger.h"
#include "AudioManager.h"
#include "resource.h"
#include <string>
#include <memory>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

std::unique_ptr<TornadoFactory> g_Factory = nullptr;
static HMODULE g_hModule = NULL;

void SetModuleHandle(HMODULE hModule) {
    g_hModule = hModule;
}

void ExtractResource(int resId, const fs::path& targetPath) {
    if (fs::exists(targetPath)) return;

    HRSRC hRes = FindResourceA(g_hModule, MAKEINTRESOURCEA(resId), (LPCSTR)RT_RCDATA);
    if (hRes) {
        HGLOBAL hData = LoadResource(g_hModule, hRes);
        if (hData) {
            DWORD size = SizeofResource(g_hModule, hRes);
            void* ptr = LockResource(hData);
            
            fs::create_directories(targetPath.parent_path());
            std::ofstream outFile(targetPath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write((const char*)ptr, size);
                outFile.close();
            }
        }
    }
}

void Bootstrap() {
    char path[MAX_PATH];
    GetModuleFileNameA(g_hModule, path, MAX_PATH);
    fs::path dllPath(path);

    // Use LOCALAPPDATA for TornadoVStuff to avoid sandboxing on Game Pass
    char* localappdata = getenv("LOCALAPPDATA");
    fs::path root = fs::path(localappdata) / "TornadoVStuff";
    fs::path sounds = root / "TornadoVSounds";

    fs::create_directories(sounds);

    // Extract default files if they don't exist
    ExtractResource(IDR_INI_DEFAULT, root / "TornadoV.ini");
    ExtractResource(IDR_XML_DEFAULT, root / "menu_config.xml");
    ExtractResource(IDR_WAV_RUMBLE,  sounds / "rumble-bass-2.wav");
    ExtractResource(IDR_WAV_EAS,     sounds / "eas_alert.wav");
    ExtractResource(IDR_WAV_SIREN,   sounds / "tornado-weather-alert.wav");
}

void update() {
    int gameTime = GAMEPLAY::GET_GAME_TIMER();
    
    if (g_Factory) {
        g_Factory->OnUpdate(gameTime);
    }
    
    // Update Audio Listener
    Vector3 camPos = CAM::GET_GAMEPLAY_CAM_COORD();
    Vector3 camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
    Vector3 forward = MathEx::RotationToDirection(camRot);
    AudioManager::Get().UpdateListener(camPos.x, camPos.y, camPos.z, forward.x, forward.y, forward.z, 0, 0, 1);

    // Logger::Log("Calling Menu::OnTick");
    TornadoMenu::OnTick();
}

void ModMain() {
    // Initialize Logger as early as possible in TornadoVStuff folder
    char path[MAX_PATH];
    GetModuleFileNameA(g_hModule, path, MAX_PATH);
    fs::path dllPath(path);

    // Use LOCALAPPDATA for TornadoVStuff
    char* localappdata = getenv("LOCALAPPDATA");
    fs::path logPath = fs::path(localappdata) / "TornadoVStuff" / "TornadoV.log";
    Logger::Initialize(logPath.string());

    try {
        // Run bootstrapping first
        Bootstrap();
        
        IniHelper::Initialize(g_hModule);
        XmlHelper::Initialize(g_hModule);
        
        MathEx::Initialize();
        
        AudioManager::Get().Init();

        // Load sounds from TornadoVStuff\TornadoVSounds
        fs::path soundsFolder = fs::path(localappdata) / "TornadoVStuff" / "TornadoVSounds";
        std::string folder = soundsFolder.string();
        
        AudioManager::Get().LoadSound("tornado_loop", folder + "\\rumble-bass-2.wav");
        AudioManager::Get().LoadSound("eas_beeps", folder + "\\eas_alert.wav");
        AudioManager::Get().LoadSound("city_siren", folder + "\\tornado-weather-alert.wav");

        TornadoMenu::Initialize();
        
        g_Factory = std::make_unique<TornadoFactory>();
        
        Logger::Log("Tornado V Enhanced initialized successfully.");
        
        IniHelper::ShowNotification("~g~TornadoV Loaded! Made by BlueIsAtlantic");

        while (true) {
            try {
                // Handle menu toggle key
                if (IsKeyJustUp(TornadoMenu::m_toggleKey)) {
                    TornadoMenu::OnKeyDown(TornadoMenu::m_toggleKey);
                }

                // Handle Tornado toggle key
                if (IsKeyJustUp(TornadoMenu::m_tornadoHotkey)) {
                    if (g_Factory && g_Factory->GetActiveVortexCount() > 0) {
                        TornadoMenu::DespawnTornado();
                    } else {
                        TornadoMenu::SpawnTornado();
                    }
                }

                update();
            } catch (const std::exception& e) {
                std::string msg = "TornadoV Script Error: ";
                msg += e.what();
                Logger::Error(msg);
                IniHelper::ShowNotification("~r~" + msg);
            } catch (...) {
                Logger::Error("Unknown script error occurred in main loop!");
                IniHelper::ShowNotification("~r~TornadoV: Unknown script error occurred!");
            }
            
            WAIT(0);
        }
    } catch (const std::exception& e) {
        std::string msg = "TornadoV Fatal Init Error: ";
        msg += e.what();
        Logger::Error(msg);
    } catch (...) {
        Logger::Error("TornadoV Fatal Unknown Init Error!");
    }
}

void ScriptMain() {
    srand(static_cast<unsigned int>(GAMEPLAY::GET_GAME_TIMER()));
    ModMain();
}
