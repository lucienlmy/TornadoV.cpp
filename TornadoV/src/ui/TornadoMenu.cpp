#include "TornadoMenu.h"
#include "natives.h"
#include "IniHelper.h"
#include "XmlHelper.h"
#include "TornadoFactory.h"
#include "Logger.h"
#include "MathEx.h"
#include "script.h"
#include "keyboard.h"
#include <cmath>
#include <memory>
#include <cstdio>

extern std::unique_ptr<TornadoFactory> g_Factory;

bool TornadoMenu::m_visible = false;
int TornadoMenu::m_currentSubmenu = 0;
int TornadoMenu::m_currentOption = 0;
int TornadoMenu::m_scrollOffset = 0;
std::vector<Submenu> TornadoMenu::m_submenus;
DWORD TornadoMenu::m_lastKey = 0;
DWORD TornadoMenu::m_lastRepeatTime = 0;
int TornadoMenu::m_repeatCount = 0;

bool TornadoMenu::m_movementEnabled = true;
bool TornadoMenu::m_reverseRotation = false;
bool TornadoMenu::m_cloudTopEnabled = false;
bool TornadoMenu::m_cloudTopParticlesEnabled = false;
bool TornadoMenu::m_surfaceDetectionEnabled = true;
bool TornadoMenu::m_useInternalPool = true;
bool TornadoMenu::m_particleMod = true;
bool TornadoMenu::m_notifications = true;
bool TornadoMenu::m_spawnInStorm = true;
bool TornadoMenu::m_enableEAS = true;
bool TornadoMenu::m_enableSirens = true;
bool TornadoMenu::m_enableTornadoSound = true;
float TornadoMenu::m_sirenVolume = 1.0f;
float TornadoMenu::m_tornadoVolume = 1.0f;
float TornadoMenu::m_easVolume = 1.0f;
float TornadoMenu::m_moveSpeedScale = 1.0f;
float TornadoMenu::m_maxEntityDistance = 57.0f;
int TornadoMenu::m_maxEntityCount = 200;
DWORD TornadoMenu::m_toggleKey = VK_F5;
DWORD TornadoMenu::m_tornadoHotkey = VK_F6;

float TornadoMenu::m_lodDistance = 500.0f;
bool TornadoMenu::m_drawBlip = true;
bool TornadoMenu::m_affectPlayer = true;

float TornadoMenu::m_vortexRadius = 9.4f;
int TornadoMenu::m_particlesPerLayer = 9;
int TornadoMenu::m_maxParticleLayers = 48;
float TornadoMenu::m_layerSeparation = 22.0f;
float TornadoMenu::m_rotationSpeed = 2.4f;
float TornadoMenu::m_vortexVerticalForceScale = 2.29f;
float TornadoMenu::m_vortexHorizontalForceScale = 1.7f;
float TornadoMenu::m_vortexMaxEntitySpeed = 40.0f;
float TornadoMenu::m_tornadoSpawnDistance = 100.0f;
bool TornadoMenu::m_followPlayer = true;
bool TornadoMenu::m_spawnInFront = true;
float TornadoMenu::m_tornadoMaxDistance = 1000.0f;

float TornadoMenu::m_menuX = 0.15f;
float TornadoMenu::m_menuY = 0.1f;
int TornadoMenu::m_themeR = 184;
int TornadoMenu::m_themeG = 162;
int TornadoMenu::m_themeB = 57;
int TornadoMenu::m_bgR = 0;
int TornadoMenu::m_bgG = 0;
int TornadoMenu::m_bgB = 0;
int TornadoMenu::m_bgA = 200;
int TornadoMenu::m_selectionR = 255;
int TornadoMenu::m_selectionG = 255;
int TornadoMenu::m_selectionB = 255;
int TornadoMenu::m_titleTextR = 255;
int TornadoMenu::m_titleTextG = 255;
int TornadoMenu::m_titleTextB = 255;
int TornadoMenu::m_subtitleTextR = 184;
int TornadoMenu::m_subtitleTextG = 162;
int TornadoMenu::m_subtitleTextB = 57;
int TornadoMenu::m_countTextR = 184;
int TornadoMenu::m_countTextG = 162;
int TornadoMenu::m_countTextB = 57;
int TornadoMenu::m_intStep = 5;
float TornadoMenu::m_floatStep = 0.1f;

int TornadoMenu::m_lastScreenWidth = 0;
int TornadoMenu::m_lastScreenHeight = 0;
float TornadoMenu::m_pixelX = 1.0f / 1920.0f;
float TornadoMenu::m_pixelY = 1.0f / 1080.0f;

XmlHelper::Color TornadoMenu::m_titleBoxColor = { 184, 162, 57, 255 };
XmlHelper::Color TornadoMenu::m_subtitleBoxColor = { 0, 0, 0, 255 };
XmlHelper::Color TornadoMenu::m_backgroundColor = { 0, 0, 0, 200 };
XmlHelper::Color TornadoMenu::m_selectionBarColor = { 255, 255, 255, 255 };
XmlHelper::Color TornadoMenu::m_titleTextColor = { 255, 255, 255, 255 };
XmlHelper::Color TornadoMenu::m_subtitleTextColor = { 184, 162, 57, 255 };
XmlHelper::Color TornadoMenu::m_countTextColor = { 184, 162, 57, 255 };
XmlHelper::Color TornadoMenu::m_optionNormalColor = { 255, 255, 255, 255 };
XmlHelper::Color TornadoMenu::m_optionSelectedColor = { 0, 0, 0, 255 };

const float TornadoMenu::MENU_WIDTH = 0.21f;
const float TornadoMenu::TITLE_HEIGHT = 0.08f;
const float TornadoMenu::OPTION_HEIGHT = 0.035f;

void TornadoMenu::Initialize() {
    m_movementEnabled = IniHelper::GetValue("Vortex", "MovementEnabled", true);
    m_reverseRotation = IniHelper::GetValue("Vortex", "ReverseRotation", false);
    m_cloudTopEnabled = IniHelper::GetValue("VortexAdvanced", "CloudTopEnabled", true);
    m_cloudTopParticlesEnabled = IniHelper::GetValue("VortexAdvanced", "CloudTopParticlesEnabled", true);
    m_surfaceDetectionEnabled = IniHelper::GetValue("VortexAdvanced", "SurfaceDetectionEnabled", true);
    m_useInternalPool = IniHelper::GetValue("VortexAdvanced", "UseInternalPool", true);
    m_particleMod = IniHelper::GetValue("VortexAdvanced", "ParticleMod", true);

    m_moveSpeedScale = IniHelper::GetValue("Vortex", "MoveSpeedScale", 1.0f);
    m_maxEntityDistance = IniHelper::GetValue("Vortex", "MaxEntityDistance", 57.0f);
    m_maxEntityCount = IniHelper::GetValue("Vortex", "MaxEntityCount", 200);

    m_vortexRadius = IniHelper::GetValue("Vortex", "VortexRadius", 9.4f);
    m_particlesPerLayer = IniHelper::GetValue("VortexAdvanced", "ParticlesPerLayer", 9);
    m_maxParticleLayers = IniHelper::GetValue("VortexAdvanced", "MaxParticleLayers", 48);
    m_layerSeparation = IniHelper::GetValue("VortexAdvanced", "LayerSeparationAmount", 22.0f);
    m_rotationSpeed = IniHelper::GetValue("Vortex", "RotationSpeed", 2.4f);
    m_vortexVerticalForceScale = IniHelper::GetValue("Vortex", "VerticalForceScale", 2.29f);
    m_vortexHorizontalForceScale = IniHelper::GetValue("Vortex", "HorizontalForceScale", 1.7f);
    m_vortexMaxEntitySpeed = IniHelper::GetValue("Vortex", "MaxEntitySpeed", 40.0f);
    m_tornadoSpawnDistance = IniHelper::GetValue("Vortex", "TornadoSpawnDistance", 100.0f);
    m_followPlayer = IniHelper::GetValue("Vortex", "FollowPlayer", true);
    m_spawnInFront = IniHelper::GetValue("Vortex", "SpawnInFront", true);
    m_tornadoMaxDistance = IniHelper::GetValue("Vortex", "TornadoMaxDistance", 1000.0f);

    m_notifications = IniHelper::GetValue("Other", "Notifications", true);
    m_spawnInStorm = IniHelper::GetValue("Other", "SpawnInStorm", true);
    m_affectPlayer = IniHelper::GetValue("Other", "AffectPlayer", true);
    m_enableEAS = IniHelper::GetValue("Other", "EnableEAS", true);
    m_enableSirens = IniHelper::GetValue("Other", "EnableSirens", true);
    m_enableTornadoSound = IniHelper::GetValue("Other", "EnableTornadoSound", true);
    m_sirenVolume = IniHelper::GetValue("Other", "SirenVolume", 1.0f);
    m_tornadoVolume = IniHelper::GetValue("Other", "TornadoVolume", 1.0f);
    m_easVolume = IniHelper::GetValue("Other", "EasVolume", 1.0f);
    m_lodDistance = IniHelper::GetValue("Other", "LodDistance", 500.0f);
    m_drawBlip = IniHelper::GetValue("Other", "AddBlip", true);
    
    m_intStep = XmlHelper::GetInt("MenuConfig.General.IntStep", 5);
    m_floatStep = XmlHelper::GetFloat("MenuConfig.General.FloatStep", 0.1f);

    // Keybinds
    m_toggleKey = StringToKey(IniHelper::GetValue("KeyBinds", "ToggleMenu", "F5"));
    m_tornadoHotkey = StringToKey(IniHelper::GetValue("KeyBinds", "ToggleTornado", "F6"));

    // UI Settings
    m_menuX = XmlHelper::GetFloat("MenuConfig.Position.X", 0.15f);
    m_menuY = XmlHelper::GetFloat("MenuConfig.Position.Y", 0.1f);
    
    // Read XML Styles
    m_titleBoxColor = XmlHelper::GetColor("MenuConfig.Frame.TitleBox", { 184, 162, 57, 255 });
    m_subtitleBoxColor = XmlHelper::GetColor("MenuConfig.Frame.SubtitleBox", { 0, 0, 0, 255 });
    m_backgroundColor = XmlHelper::GetColor("MenuConfig.Frame.Background", { 0, 0, 0, 200 });
    m_selectionBarColor = XmlHelper::GetColor("MenuConfig.Frame.SelectionBar", { 255, 255, 255, 255 });
    
    m_titleTextColor = XmlHelper::GetColor("MenuConfig.TextColors.TitleText", { 255, 255, 255, 255 });
    m_subtitleTextColor = XmlHelper::GetColor("MenuConfig.TextColors.SubtitleText", { 184, 162, 57, 255 });
    m_countTextColor = XmlHelper::GetColor("MenuConfig.TextColors.CountText", { 184, 162, 57, 255 });
    m_optionNormalColor = XmlHelper::GetColor("MenuConfig.TextColors.OptionNormal", { 255, 255, 255, 255 });
    m_optionSelectedColor = XmlHelper::GetColor("MenuConfig.TextColors.OptionSelected", { 0, 0, 0, 255 });

    // Sync slider variables with loaded colors
    m_themeR = m_titleBoxColor.r;
    m_themeG = m_titleBoxColor.g;
    m_themeB = m_titleBoxColor.b;
    m_bgR = m_backgroundColor.r;
    m_bgG = m_backgroundColor.g;
    m_bgB = m_backgroundColor.b;
    m_bgA = m_backgroundColor.a;
    m_selectionR = m_selectionBarColor.r;
    m_selectionG = m_selectionBarColor.g;
    m_selectionB = m_selectionBarColor.b;
    m_titleTextR = m_titleTextColor.r;
    m_titleTextG = m_titleTextColor.g;
    m_titleTextB = m_titleTextColor.b;
    m_subtitleTextR = m_subtitleTextColor.r;
    m_subtitleTextG = m_subtitleTextColor.g;
    m_subtitleTextB = m_subtitleTextColor.b;
    m_countTextR = m_countTextColor.r;
    m_countTextG = m_countTextColor.g;
    m_countTextB = m_countTextColor.b;

    // Initialize resolution tracking
    GRAPHICS::_GET_SCREEN_ACTIVE_RESOLUTION(&m_lastScreenWidth, &m_lastScreenHeight);
    m_pixelX = 1.0f / (float)m_lastScreenWidth;
    m_pixelY = 1.0f / (float)m_lastScreenHeight;

    SetupMenus();
}

void TornadoMenu::SetupMenus() {
    m_submenus.clear();
    
    // Main Menu (Index 0)
    Submenu main;
    main.title = "TornadoV++";
    main.subtitle = "Control Panel";
    main.items.push_back(MenuItem("Spawn Tornado", []() { SpawnTornado(); }));
    main.items.push_back(MenuItem("Despawn Tornado", []() { DespawnTornado(); }));
    main.items.push_back(MenuItem("Teleport to Tornado", []() { TeleportToTornado(); }));
    main.items.push_back(MenuItem("Tornado Settings", 1));
    main.items.push_back(MenuItem("General Settings", 2));
    main.items.push_back(MenuItem("Sound Options", 4));
    main.items.push_back(MenuItem("Menu Customization", 3));
    m_submenus.push_back(main);

    // Tornado Settings (Index 1) - Consolidates all legitimate settings from .ini
    Submenu tornado;
    tornado.title = "Tornado Settings";
    tornado.subtitle = "Physics & Particles";
    
    // [Vortex] Section
    tornado.items.push_back(MenuItem("Movement Enabled", &m_movementEnabled, []() {
        IniHelper::WriteValue("Vortex", "MovementEnabled", m_movementEnabled ? "true" : "false");
    }));
    tornado.items.push_back(MenuItem("Move Speed Scale", &m_moveSpeedScale, 0.1f, 20.0f, m_floatStep, []() {
        IniHelper::WriteValue("Vortex", "MoveSpeedScale", std::to_string(m_moveSpeedScale));
    }));
    tornado.items.push_back(MenuItem("Max Entity Speed", &m_vortexMaxEntitySpeed, 1.0f, 200.0f, m_floatStep, []() {
        IniHelper::WriteValue("Vortex", "MaxEntitySpeed", std::to_string(m_vortexMaxEntitySpeed));
    }));
    tornado.items.push_back(MenuItem("Max Entity Distance", &m_maxEntityDistance, 10.0f, 2000.0f, m_floatStep, []() {
        IniHelper::WriteValue("Vortex", "MaxEntityDistance", std::to_string(m_maxEntityDistance));
    }));
    tornado.items.push_back(MenuItem("Max Entity Count", &m_maxEntityCount, 10, 1000, m_intStep, []() {
        IniHelper::WriteValue("Vortex", "MaxEntityCount", std::to_string(m_maxEntityCount));
    }));
    tornado.items.push_back(MenuItem("Horizontal Force Scale", &m_vortexHorizontalForceScale, 0.1f, 10.0f, m_floatStep, []() {
        IniHelper::WriteValue("Vortex", "HorizontalForceScale", std::to_string(m_vortexHorizontalForceScale));
    }));
    tornado.items.push_back(MenuItem("Vertical Force Scale", &m_vortexVerticalForceScale, 0.1f, 10.0f, m_floatStep, []() {
        IniHelper::WriteValue("Vortex", "VerticalForceScale", std::to_string(m_vortexVerticalForceScale));
    }));
    tornado.items.push_back(MenuItem("Vortex Radius", &m_vortexRadius, 1.0f, 50.0f, m_floatStep, []() {
        IniHelper::WriteValue("Vortex", "VortexRadius", std::to_string(m_vortexRadius));
    }));
    tornado.items.push_back(MenuItem("Rotation Speed", &m_rotationSpeed, 0.1f, 10.0f, m_floatStep, []() {
        IniHelper::WriteValue("Vortex", "RotationSpeed", std::to_string(m_rotationSpeed));
    }));
    tornado.items.push_back(MenuItem("Reverse Rotation", &m_reverseRotation, []() {
        IniHelper::WriteValue("Vortex", "ReverseRotation", m_reverseRotation ? "true" : "false");
    }));

    // Tornado Customization Settings
    tornado.items.push_back(MenuItem("Tornado Spawn Distance", &m_tornadoSpawnDistance, 20.0f, 500.0f, 5.0f, []() {
        IniHelper::WriteValue("Vortex", "TornadoSpawnDistance", std::to_string(m_tornadoSpawnDistance));
    }));
    tornado.items.push_back(MenuItem("Follow Player", &m_followPlayer, []() {
        IniHelper::WriteValue("Vortex", "FollowPlayer", m_followPlayer ? "true" : "false");
    }));
    tornado.items.push_back(MenuItem("Spawn In-Front", &m_spawnInFront, []() {
        IniHelper::WriteValue("Vortex", "SpawnInFront", m_spawnInFront ? "true" : "false");
    }));
    tornado.items.push_back(MenuItem("Tornado Max Distance", &m_tornadoMaxDistance, 200.0f, 2000.0f, 50.0f, []() {
        IniHelper::WriteValue("Vortex", "TornadoMaxDistance", std::to_string(m_tornadoMaxDistance));
    }));

    // [VortexAdvanced] Section
    tornado.items.push_back(MenuItem("Max Particle Layers", &m_maxParticleLayers, 1, 200, 1, []() {
        IniHelper::WriteValue("VortexAdvanced", "MaxParticleLayers", std::to_string(m_maxParticleLayers));
    }));
    tornado.items.push_back(MenuItem("Particles Per Layer", &m_particlesPerLayer, 1, 50, 1, []() {
        IniHelper::WriteValue("VortexAdvanced", "ParticlesPerLayer", std::to_string(m_particlesPerLayer));
    }));
    tornado.items.push_back(MenuItem("Layer Separation Amount", &m_layerSeparation, 1.0f, 100.0f, m_floatStep, []() {
        IniHelper::WriteValue("VortexAdvanced", "LayerSeparationAmount", std::to_string(m_layerSeparation));
    }));
    tornado.items.push_back(MenuItem("Cloud Top Enabled", &m_cloudTopEnabled, []() {
        IniHelper::WriteValue("VortexAdvanced", "CloudTopEnabled", m_cloudTopEnabled ? "true" : "false");
    }));
    tornado.items.push_back(MenuItem("Cloud Top Particles Enabled", &m_cloudTopParticlesEnabled, []() {
        IniHelper::WriteValue("VortexAdvanced", "CloudTopParticlesEnabled", m_cloudTopParticlesEnabled ? "true" : "false");
    }));
    tornado.items.push_back(MenuItem("Surface Detection Enabled", &m_surfaceDetectionEnabled, []() {
        IniHelper::WriteValue("VortexAdvanced", "SurfaceDetectionEnabled", m_surfaceDetectionEnabled ? "true" : "false");
    }));
    tornado.items.push_back(MenuItem("Use Internal Pool", &m_useInternalPool, []() {
        IniHelper::WriteValue("VortexAdvanced", "UseInternalPool", m_useInternalPool ? "true" : "false");
    }));
    tornado.items.push_back(MenuItem("Particle Mod", &m_particleMod, []() {
        IniHelper::WriteValue("VortexAdvanced", "ParticleMod", m_particleMod ? "true" : "false");
    }));
    m_submenus.push_back(tornado);

    // General Settings (Index 2)
    Submenu general;
    general.title = "Tornado V Enhanced";
    general.subtitle = "General Settings";
    general.items.push_back(MenuItem("Notifications", &m_notifications, []() {
        IniHelper::WriteValue("Other", "Notifications", m_notifications ? "true" : "false");
    }));
    general.items.push_back(MenuItem("Spawn In Storm", &m_spawnInStorm, []() {
        IniHelper::WriteValue("Other", "SpawnInStorm", m_spawnInStorm ? "true" : "false");
    }));
    general.items.push_back(MenuItem("Affect Player", &m_affectPlayer, []() {
        IniHelper::WriteValue("Other", "AffectPlayer", m_affectPlayer ? "true" : "false");
    }));
    general.items.push_back(MenuItem("LOD Distance", &m_lodDistance, 100.0f, 2000.0f, m_floatStep, []() {
        IniHelper::WriteValue("Other", "LodDistance", std::to_string(m_lodDistance));
    }));
    general.items.push_back(MenuItem("Add Blip", &m_drawBlip, []() {
        IniHelper::WriteValue("Other", "AddBlip", m_drawBlip ? "true" : "false");
    }));
    
    // Note: No manual repair buttons needed - auto-repair handles everything
    
    std::string menuKeyName = KeyToString(m_toggleKey);
    general.items.push_back(MenuItem("Toggle Menu: " + menuKeyName, [menuKeyName]() {
        std::string input = GetUserInput("Enter Key (F1-F12)", menuKeyName, 3);
        if (!input.empty()) {
            m_toggleKey = StringToKey(input);
            IniHelper::WriteValue("KeyBinds", "ToggleMenu", input);
            SetupMenus();
        }
    }));

    std::string tornadoKeyName = KeyToString(m_tornadoHotkey);
    general.items.push_back(MenuItem("Toggle Tornado: " + tornadoKeyName, [tornadoKeyName]() {
        std::string input = GetUserInput("Enter Key (F1-F12)", tornadoKeyName, 3);
        if (!input.empty()) {
            m_tornadoHotkey = StringToKey(input);
            IniHelper::WriteValue("KeyBinds", "ToggleTornado", input);
            SetupMenus();
        }
    }));
    m_submenus.push_back(general);

    // Menu Settings (Index 5)
    Submenu menuSettings;
    menuSettings.title = "TornadoV++";
    menuSettings.subtitle = "Menu Settings";
    
    menuSettings.items.push_back(MenuItem("Menu X", &m_menuX, 0.0f, 1.0f, m_pixelX, []() {
        XmlHelper::WriteValue("MenuConfig.Position.X", std::to_string(m_menuX));
    }));
    menuSettings.items.push_back(MenuItem("Menu Y", &m_menuY, 0.0f, 1.0f, m_pixelY, []() {
        XmlHelper::WriteValue("MenuConfig.Position.Y", std::to_string(m_menuY));
    }));

    menuSettings.items.push_back(MenuItem("Int Step", &m_intStep, 1, 100, 1, []() {
        XmlHelper::WriteValue("MenuConfig.General.IntStep", std::to_string(m_intStep));
        SetupMenus();
    }));
    menuSettings.items.push_back(MenuItem("Float Step", &m_floatStep, 0.01f, 5.0f, 0.01f, []() {
        XmlHelper::WriteValue("MenuConfig.General.FloatStep", std::to_string(m_floatStep));
        SetupMenus();
    }));
    
    menuSettings.items.push_back(MenuItem("Theme Red", &m_themeR, 0, 255, m_intStep, []() {
         m_titleBoxColor.r = m_themeR;
         XmlHelper::WriteColor("MenuConfig.Frame.TitleBox", m_titleBoxColor);
    }));
    // ... (rest of color items)
    // I'll keep the color items but truncated for the toolcall
    
    menuSettings.items.push_back(MenuItem("Theme Green", &m_themeG, 0, 255, m_intStep, []() {
         m_titleBoxColor.g = m_themeG;
         XmlHelper::WriteColor("MenuConfig.Frame.TitleBox", m_titleBoxColor);
    }));
     menuSettings.items.push_back(MenuItem("Theme Blue", &m_themeB, 0, 255, m_intStep, []() {
         m_titleBoxColor.b = m_themeB;
         XmlHelper::WriteColor("MenuConfig.Frame.TitleBox", m_titleBoxColor);
     }));
     
     menuSettings.items.push_back(MenuItem("Background Red", &m_bgR, 0, 255, m_intStep, []() {
         m_backgroundColor.r = m_bgR;
         XmlHelper::WriteColor("MenuConfig.Frame.Background", m_backgroundColor);
     }));
     menuSettings.items.push_back(MenuItem("Background Green", &m_bgG, 0, 255, m_intStep, []() {
         m_backgroundColor.g = m_bgG;
         XmlHelper::WriteColor("MenuConfig.Frame.Background", m_backgroundColor);
     }));
     menuSettings.items.push_back(MenuItem("Background Blue", &m_bgB, 0, 255, m_intStep, []() {
         m_backgroundColor.b = m_bgB;
         XmlHelper::WriteColor("MenuConfig.Frame.Background", m_backgroundColor);
     }));
     menuSettings.items.push_back(MenuItem("Background Opacity", &m_bgA, 0, 255, m_intStep, []() {
         m_backgroundColor.a = m_bgA;
         XmlHelper::WriteColor("MenuConfig.Frame.Background", m_backgroundColor);
     }));

    // Add more items to demonstrate scrolling (MAX_VISIBLE_OPTIONS is 10)
     menuSettings.items.push_back(MenuItem("Selection Red", &m_selectionR, 0, 255, m_intStep, []() {
         m_selectionBarColor.r = m_selectionR;
     }));
     menuSettings.items.push_back(MenuItem("Selection Green", &m_selectionG, 0, 255, m_intStep, []() {
         m_selectionBarColor.g = m_selectionG;
     }));
     menuSettings.items.push_back(MenuItem("Selection Blue", &m_selectionB, 0, 255, m_intStep, []() {
          m_selectionBarColor.b = m_selectionB;
          XmlHelper::WriteColor("MenuConfig.Frame.SelectionBar", m_selectionBarColor);
      }));
 
       menuSettings.items.push_back(MenuItem("Title Text Red", &m_titleTextR, 0, 255, m_intStep, []() {
           m_titleTextColor.r = m_titleTextR;
           XmlHelper::WriteColor("MenuConfig.TextColors.TitleText", m_titleTextColor);
       }));
       menuSettings.items.push_back(MenuItem("Title Text Green", &m_titleTextG, 0, 255, m_intStep, []() {
           m_titleTextColor.g = m_titleTextG;
           XmlHelper::WriteColor("MenuConfig.TextColors.TitleText", m_titleTextColor);
       }));
       menuSettings.items.push_back(MenuItem("Title Text Blue", &m_titleTextB, 0, 255, m_intStep, []() {
           m_titleTextColor.b = m_titleTextB;
           XmlHelper::WriteColor("MenuConfig.TextColors.TitleText", m_titleTextColor);
       }));
   
        menuSettings.items.push_back(MenuItem("Subtitle Red", &m_subtitleTextR, 0, 255, m_intStep, []() {
            m_subtitleTextColor.r = m_subtitleTextR;
            XmlHelper::WriteColor("MenuConfig.TextColors.SubtitleText", m_subtitleTextColor);
        }));
        menuSettings.items.push_back(MenuItem("Subtitle Green", &m_subtitleTextG, 0, 255, m_intStep, []() {
            m_subtitleTextColor.g = m_subtitleTextG;
            XmlHelper::WriteColor("MenuConfig.TextColors.SubtitleText", m_subtitleTextColor);
        }));
        menuSettings.items.push_back(MenuItem("Subtitle Blue", &m_subtitleTextB, 0, 255, m_intStep, []() {
            m_subtitleTextColor.b = m_subtitleTextB;
            XmlHelper::WriteColor("MenuConfig.TextColors.SubtitleText", m_subtitleTextColor);
        }));
 
        menuSettings.items.push_back(MenuItem("Counter Red", &m_countTextR, 0, 255, m_intStep, []() {
            m_countTextColor.r = m_countTextR;
            XmlHelper::WriteColor("MenuConfig.TextColors.CountText", m_countTextColor);
        }));
        menuSettings.items.push_back(MenuItem("Counter Green", &m_countTextG, 0, 255, m_intStep, []() {
            m_countTextColor.g = m_countTextG;
            XmlHelper::WriteColor("MenuConfig.TextColors.CountText", m_countTextColor);
        }));
        menuSettings.items.push_back(MenuItem("Counter Blue", &m_countTextB, 0, 255, m_intStep, []() {
            m_countTextColor.b = m_countTextB;
            XmlHelper::WriteColor("MenuConfig.TextColors.CountText", m_countTextColor);
        }));

       m_submenus.push_back(menuSettings);

    // Sound Options (Index 4)
    Submenu sound;
    sound.title = "Tornado V Enhanced";
    sound.subtitle = "Sound Options";
    sound.items.push_back(MenuItem("Enable Tornado Sound", &m_enableTornadoSound, []() {
        IniHelper::WriteValue("Other", "EnableTornadoSound", m_enableTornadoSound ? "true" : "false");
    }));
    sound.items.push_back(MenuItem("Tornado Volume", &m_tornadoVolume, 0.0f, 2.0f, 0.1f, []() {
        IniHelper::WriteValue("Other", "TornadoVolume", std::to_string(m_tornadoVolume));
    }));
    sound.items.push_back(MenuItem("Enable Sirens", &m_enableSirens, []() {
        IniHelper::WriteValue("Other", "EnableSirens", m_enableSirens ? "true" : "false");
    }));
    sound.items.push_back(MenuItem("Siren Volume", &m_sirenVolume, 0.0f, 2.0f, 0.1f, []() {
        IniHelper::WriteValue("Other", "SirenVolume", std::to_string(m_sirenVolume));
    }));
    sound.items.push_back(MenuItem("Enable EAS", &m_enableEAS, []() {
        IniHelper::WriteValue("Other", "EnableEAS", m_enableEAS ? "true" : "false");
    }));
    sound.items.push_back(MenuItem("EAS Volume", &m_easVolume, 0.0f, 2.0f, 0.1f, []() {
        IniHelper::WriteValue("Other", "EasVolume", std::to_string(m_easVolume));
    }));
    m_submenus.push_back(sound);
}

void TornadoMenu::DrawRect(float x, float y, float width, float height, int r, int g, int b, int a) {
    GRAPHICS::DRAW_RECT(x, y, width, height, r, g, b, a);
}

void TornadoMenu::DrawText(std::string text, float x, float y, float scale, int font, int r, int g, int b, int a, bool center, bool right, bool outline) {
    UI::SET_TEXT_FONT(font);
    UI::SET_TEXT_SCALE(0.0f, scale);
    UI::SET_TEXT_COLOUR(r, g, b, a);
    UI::SET_TEXT_CENTRE(center);
    if (right) {
        UI::SET_TEXT_RIGHT_JUSTIFY(true);
        UI::SET_TEXT_WRAP(0.0f, x);
    }
    if (outline) UI::SET_TEXT_OUTLINE();
    UI::_SET_TEXT_ENTRY(const_cast<char*>("STRING"));
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(text.c_str()));
    UI::_DRAW_TEXT(x, y);
}

std::string TornadoMenu::GetUserInput(const std::string& title, const std::string& defaultText, int maxLength) {
    GAMEPLAY::DISPLAY_ONSCREEN_KEYBOARD(1, const_cast<char*>(title.c_str()), const_cast<char*>(""), const_cast<char*>(defaultText.c_str()), const_cast<char*>(""), const_cast<char*>(""), const_cast<char*>(""), maxLength);
    
    // Wait for keyboard to be closed or result to be ready
    while (GAMEPLAY::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        WAIT(0);
    }

    if (GAMEPLAY::UPDATE_ONSCREEN_KEYBOARD() == 1) {
        char* result = GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT();
        std::string res = result ? std::string(result) : "";
        // Reset key states to prevent menu from re-triggering textbox immediately
        ResetKeyState(VK_RETURN);
        ResetKeyState(VK_NUMPAD5);
        return res;
    }
    
    ResetKeyState(VK_RETURN);
    ResetKeyState(VK_NUMPAD5);
    return "";
}

DWORD TornadoMenu::StringToKey(const std::string& keyStr) {
    if (keyStr == "F1") return VK_F1;
    if (keyStr == "F2") return VK_F2;
    if (keyStr == "F3") return VK_F3;
    if (keyStr == "F4") return VK_F4;
    if (keyStr == "F5") return VK_F5;
    if (keyStr == "F6") return VK_F6;
    if (keyStr == "F7") return VK_F7;
    if (keyStr == "F8") return VK_F8;
    if (keyStr == "F9") return VK_F9;
    if (keyStr == "F10") return VK_F10;
    if (keyStr == "F11") return VK_F11;
    if (keyStr == "F12") return VK_F12;
    return 0;
}

std::string TornadoMenu::KeyToString(DWORD key) {
    switch (key) {
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        default: return "UNKNOWN";
    }
}

void TornadoMenu::RefreshVortexSettings() {
    if (g_Factory) {
        g_Factory->RefreshAllVortexSettings();
    }
}

void TornadoMenu::OnTick() {
    CheckResolution();
    if (m_visible) {
        // Disable game controls when menu is open
        CONTROLS::DISABLE_CONTROL_ACTION(0, 27, true); // INPUT_PHONE
        CONTROLS::DISABLE_CONTROL_ACTION(0, 19, true); // INPUT_CHARACTER_WHEEL
        CONTROLS::DISABLE_CONTROL_ACTION(0, 140, true); // INPUT_MELEE_ATTACK_LIGHT
        CONTROLS::DISABLE_CONTROL_ACTION(0, 141, true); // INPUT_MELEE_ATTACK_HEAVY
        CONTROLS::DISABLE_CONTROL_ACTION(0, 142, true); // INPUT_MELEE_ATTACK_ALTERNATE
        CONTROLS::DISABLE_CONTROL_ACTION(0, 257, true); // INPUT_ATTACK2
        CONTROLS::DISABLE_CONTROL_ACTION(0, 263, true); // INPUT_MELEE_ATTACK1
        CONTROLS::DISABLE_CONTROL_ACTION(0, 264, true); // INPUT_MELEE_ATTACK2
        CONTROLS::DISABLE_CONTROL_ACTION(0, 24, true); // INPUT_ATTACK
        CONTROLS::DISABLE_CONTROL_ACTION(0, 25, true); // INPUT_AIM
        CONTROLS::DISABLE_CONTROL_ACTION(0, 14, true); // INPUT_WEAPON_WHEEL_NEXT
        CONTROLS::DISABLE_CONTROL_ACTION(0, 15, true); // INPUT_WEAPON_WHEEL_PREV
        CONTROLS::DISABLE_CONTROL_ACTION(0, 16, true); // INPUT_SELECT_NEXT_WEAPON
        CONTROLS::DISABLE_CONTROL_ACTION(0, 17, true); // INPUT_SELECT_PREV_WEAPON
        
        // Disable arrow key game actions
        CONTROLS::DISABLE_CONTROL_ACTION(0, 172, true); // INPUT_CELLPHONE_UP
        CONTROLS::DISABLE_CONTROL_ACTION(0, 173, true); // INPUT_CELLPHONE_DOWN
        CONTROLS::DISABLE_CONTROL_ACTION(0, 174, true); // INPUT_CELLPHONE_LEFT
        CONTROLS::DISABLE_CONTROL_ACTION(0, 175, true); // INPUT_CELLPHONE_RIGHT
        
        DrawMenu();
        HandleInput();
    }
}

void TornadoMenu::OnKeyDown(DWORD key) {
    if (key == m_toggleKey) {
        m_visible = !m_visible;
        if (m_visible) {
            m_currentSubmenu = 0;
            m_currentOption = 0;
            AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("SELECT"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
        } else {
            AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("QUIT"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
        }
    }
}

void TornadoMenu::CheckResolution() {
    int screenW, screenH;
    GRAPHICS::_GET_SCREEN_ACTIVE_RESOLUTION(&screenW, &screenH);

    if (screenW != m_lastScreenWidth || screenH != m_lastScreenHeight) {
        m_lastScreenWidth = screenW;
        m_lastScreenHeight = screenH;
        m_pixelX = 1.0f / (float)screenW;
        m_pixelY = 1.0f / (float)screenH;
        
        // Refresh menus to update boundaries based on new resolution
        SetupMenus();
        Logger::Log("Menu: Resolution changed to " + std::to_string(screenW) + "x" + std::to_string(screenH));
    }
}

void TornadoMenu::DrawMenu() {
    if (m_currentSubmenu >= m_submenus.size()) return;

    const Submenu& current = m_submenus[m_currentSubmenu];
    int totalItems = (int)current.items.size();
    int visibleItems = (std::min)(totalItems, MAX_VISIBLE_OPTIONS);
    
    float totalHeight = TITLE_HEIGHT + OPTION_HEIGHT + (visibleItems * OPTION_HEIGHT);
    if (totalItems > MAX_VISIBLE_OPTIONS) totalHeight += OPTION_HEIGHT; // Add height for scroll indicators bar

    // Clamp menu position to stay on screen
    float minX = MENU_WIDTH / 2.0f;
    float maxX = 1.0f - (MENU_WIDTH / 2.0f);
    if (m_menuX < minX) m_menuX = minX;
    if (m_menuX > maxX) m_menuX = maxX;

    // Y clamping: Ensure the top (m_menuY) and bottom (m_menuY + totalHeight) are on screen
    if (m_menuY < 0.0f) m_menuY = 0.0f;
    if (m_menuY + totalHeight > 1.0f) m_menuY = 1.0f - totalHeight;

    float y = m_menuY;

    // Draw Title
    DrawRect(m_menuX, y + TITLE_HEIGHT / 2, MENU_WIDTH, TITLE_HEIGHT, m_titleBoxColor.r, m_titleBoxColor.g, m_titleBoxColor.b, m_titleBoxColor.a);
    DrawText(current.title, m_menuX, y + TITLE_HEIGHT / 2 - 0.02f, 0.8f, 1, m_titleTextColor.r, m_titleTextColor.g, m_titleTextColor.b, m_titleTextColor.a, true);
    y += TITLE_HEIGHT;

    // Draw Subtitle
    DrawRect(m_menuX, y + OPTION_HEIGHT / 2, MENU_WIDTH, OPTION_HEIGHT, m_subtitleBoxColor.r, m_subtitleBoxColor.g, m_subtitleBoxColor.b, m_subtitleBoxColor.a);
    DrawText(current.subtitle, m_menuX - MENU_WIDTH / 2 + 0.005f, y + OPTION_HEIGHT / 2 - 0.015f, 0.35f, 0, m_subtitleTextColor.r, m_subtitleTextColor.g, m_subtitleTextColor.b, m_subtitleTextColor.a, false);
    DrawText(std::to_string(m_currentOption + 1) + " / " + std::to_string(totalItems), m_menuX + MENU_WIDTH / 2 - 0.005f, y + OPTION_HEIGHT / 2 - 0.015f, 0.35f, 0, m_countTextColor.r, m_countTextColor.g, m_countTextColor.b, m_countTextColor.a, false, true);
    y += OPTION_HEIGHT;

    // Update Scroll Offset
    if (m_currentOption < m_scrollOffset) {
        m_scrollOffset = m_currentOption;
    } else if (m_currentOption >= m_scrollOffset + MAX_VISIBLE_OPTIONS) {
        m_scrollOffset = m_currentOption - MAX_VISIBLE_OPTIONS + 1;
    }

    // Draw Options
    for (int i = 0; i < visibleItems; ++i) {
        int index = m_scrollOffset + i;
        if (index >= totalItems) break;

        const MenuItem& item = current.items[index];
        bool isSelected = (index == m_currentOption);

        XmlHelper::Color rectColor = isSelected ? m_selectionBarColor : m_backgroundColor;
        XmlHelper::Color textColor = isSelected ? m_optionSelectedColor : m_optionNormalColor;

        DrawRect(m_menuX, y + OPTION_HEIGHT / 2, MENU_WIDTH, OPTION_HEIGHT, rectColor.r, rectColor.g, rectColor.b, isSelected ? 255 : rectColor.a);
        DrawText(item.text, m_menuX - MENU_WIDTH / 2 + 0.005f, y + OPTION_HEIGHT / 2 - 0.015f, 0.35f, 0, textColor.r, textColor.g, textColor.b, textColor.a, false);

        if (item.type == MenuItemType::Checkbox) {
            // Menyoo-style toggle squares
            float squareSize = 0.012f;
            float squareX = m_menuX + MENU_WIDTH / 2 - 0.015f;
            float squareY = y + OPTION_HEIGHT / 2;
            
            // Background of toggle
            DrawRect(squareX, squareY, squareSize, squareSize * 1.8f, 50, 50, 50, 255);
            
            // Toggle color
            if (*item.checkboxValue) {
                DrawRect(squareX, squareY, squareSize - 0.004f, (squareSize - 0.004f) * 1.8f, 0, 200, 0, 255); // Green
            } else {
                DrawRect(squareX, squareY, squareSize - 0.004f, (squareSize - 0.004f) * 1.8f, 200, 0, 0, 255); // Red
            }
        } else if (item.type == MenuItemType::Submenu) {
            DrawText(">", m_menuX + MENU_WIDTH / 2 - 0.01f, y + OPTION_HEIGHT / 2 - 0.015f, 0.35f, 0, textColor.r, textColor.g, textColor.b, textColor.a, false, true);
        } else if (item.type == MenuItemType::Int) {
            DrawText("< " + std::to_string(*item.intValue) + " >", m_menuX + MENU_WIDTH / 2 - 0.005f, y + OPTION_HEIGHT / 2 - 0.015f, 0.35f, 0, textColor.r, textColor.g, textColor.b, textColor.a, false, true);
        } else if (item.type == MenuItemType::Float) {
            char buf[32];
            sprintf_s(buf, "< %.2f >", *item.floatValue);
            DrawText(buf, m_menuX + MENU_WIDTH / 2 - 0.005f, y + OPTION_HEIGHT / 2 - 0.015f, 0.35f, 0, textColor.r, textColor.g, textColor.b, textColor.a, false, true);
        }

        y += OPTION_HEIGHT;
    }

    // Draw Scroll Indicators and Item Count
    if (totalItems > 0) {
        // Draw footer bar background
        DrawRect(m_menuX, y + OPTION_HEIGHT / 2, MENU_WIDTH, OPTION_HEIGHT, m_subtitleBoxColor.r, m_subtitleBoxColor.g, m_subtitleBoxColor.b, m_subtitleBoxColor.a);
        
        // Item count (e.g., 1 / 15)
        std::string countStr = std::to_string(m_currentOption + 1) + " / " + std::to_string(totalItems);
        DrawText(countStr, m_menuX + MENU_WIDTH / 2 - 0.005f, y + OPTION_HEIGHT / 2 - 0.015f, 0.35f, 0, m_countTextColor.r, m_countTextColor.g, m_countTextColor.b, m_countTextColor.a, false, true);

        // Scroll indicators
        if (totalItems > MAX_VISIBLE_OPTIONS) {
            if (m_scrollOffset > 0) {
                DrawText("^", m_menuX, y + OPTION_HEIGHT / 2 - 0.018f, 0.35f, 0, m_subtitleTextColor.r, m_subtitleTextColor.g, m_subtitleTextColor.b, m_subtitleTextColor.a, true);
            }
            if (m_scrollOffset + MAX_VISIBLE_OPTIONS < totalItems) {
                DrawText("v", m_menuX, y + OPTION_HEIGHT / 2 - 0.005f, 0.35f, 0, m_subtitleTextColor.r, m_subtitleTextColor.g, m_subtitleTextColor.b, m_subtitleTextColor.a, true);
            }
        }
    }
}

bool TornadoMenu::IsKeyDownWithRepeat(DWORD key) {
    if (IsKeyDown(key)) {
        if (m_lastKey != key) {
            m_lastKey = key;
            m_lastRepeatTime = GetTickCount();
            m_repeatCount = 0;
            return true;
        } else {
            DWORD currentTime = GetTickCount();
            // Initial delay of 500ms, then repeats every 50ms (getting faster)
            DWORD delay = (m_repeatCount == 0) ? 500 : (std::max)(10, 50 - (m_repeatCount * 2));
            if (currentTime - m_lastRepeatTime > delay) {
                m_lastRepeatTime = currentTime;
                m_repeatCount++;
                return true;
            }
        }
    } else if (m_lastKey == key) {
        m_lastKey = 0;
        m_repeatCount = 0;
    }
    return false;
}

void TornadoMenu::HandleInput() {
    const Submenu& current = m_submenus[m_currentSubmenu];

    if (IsKeyDownWithRepeat(VK_NUMPAD8) || IsKeyDownWithRepeat(VK_UP)) {
        m_currentOption--;
        if (m_currentOption < 0) {
            m_currentOption = (int)current.items.size() - 1;
            m_scrollOffset = (std::max)(0, (int)current.items.size() - MAX_VISIBLE_OPTIONS);
        }
        AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("NAV_UP_DOWN"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
    } else if (IsKeyDownWithRepeat(VK_NUMPAD2) || IsKeyDownWithRepeat(VK_DOWN)) {
        m_currentOption++;
        if (m_currentOption >= (int)current.items.size()) {
            m_currentOption = 0;
            m_scrollOffset = 0;
        }
        AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("NAV_UP_DOWN"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
    } else if (IsKeyJustUp(VK_NUMPAD5, true) || IsKeyJustUp(VK_RETURN, true)) {
        const MenuItem& item = current.items[m_currentOption];
        if (item.type == MenuItemType::Button) {
            if (item.action) item.action();
        } else if (item.type == MenuItemType::Checkbox) {
            if (item.checkboxValue) {
                *item.checkboxValue = !(*item.checkboxValue);
                if (item.action) item.action();
            }
        } else if (item.type == MenuItemType::Submenu) {
            m_currentSubmenu = item.targetSubmenu;
            m_currentOption = 0;
            m_scrollOffset = 0;
        } else if (item.type == MenuItemType::Int) {
            std::string input = GetUserInput("Enter Value (" + std::to_string(item.minInt) + "-" + std::to_string(item.maxInt) + ")", std::to_string(*item.intValue), 10);
            if (!input.empty()) {
                try {
                    int val = std::stoi(input);
                    if (val >= item.minInt && val <= item.maxInt) {
                        *item.intValue = val;
                        if (item.action) item.action();
                    }
                } catch (...) {}
            }
        } else if (item.type == MenuItemType::Float) {
            char buf[32];
            sprintf_s(buf, "%.2f", *item.floatValue);
            std::string input = GetUserInput("Enter Value (" + std::to_string(item.minFloat) + "-" + std::to_string(item.maxFloat) + ")", buf, 10);
            if (!input.empty()) {
                try {
                    float val = std::stof(input);
                    if (val >= item.minFloat && val <= item.maxFloat) {
                        *item.floatValue = val;
                        if (item.action) item.action();
                    }
                } catch (...) {}
            }
        }
        AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("SELECT"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
    } else if (IsKeyJustUp(VK_NUMPAD0, true) || IsKeyJustUp(VK_BACK, true)) {
        if (m_currentSubmenu > 0) {
            m_currentSubmenu = 0; // For now just back to main, can be improved to a stack
            m_currentOption = 0;
            m_scrollOffset = 0;
            AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("BACK"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
        } else {
            m_visible = false;
            AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("QUIT"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
        }
    } else if (IsKeyJustUp(VK_LEFT) || IsKeyJustUp(VK_RIGHT)) {
         // Check for textbox input on Enter/Numpad5 if left/right were being used for fine-tuning
         // But the user wants Enter to open textbox. Let's add that to SELECT handler.
    } else if (IsKeyDownWithRepeat(VK_NUMPAD4) || IsKeyDownWithRepeat(VK_LEFT)) {
        const MenuItem& item = current.items[m_currentOption];
        if (item.type == MenuItemType::Int) {
            *item.intValue -= item.stepInt;
            if (*item.intValue < item.minInt) *item.intValue = item.minInt;
            if (item.action) item.action();
            AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("NAV_UP_DOWN"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
        } else if (item.type == MenuItemType::Float) {
            *item.floatValue -= item.stepFloat;
            if (*item.floatValue < item.minFloat) *item.floatValue = item.minFloat;
            if (item.action) item.action();
            AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("NAV_UP_DOWN"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
        }
    } else if (IsKeyDownWithRepeat(VK_NUMPAD6) || IsKeyDownWithRepeat(VK_RIGHT)) {
        const MenuItem& item = current.items[m_currentOption];
        if (item.type == MenuItemType::Int) {
            *item.intValue += item.stepInt;
            if (*item.intValue > item.maxInt) *item.intValue = item.minInt;
            if (item.action) item.action();
            AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("NAV_UP_DOWN"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
        } else if (item.type == MenuItemType::Float) {
            *item.floatValue += item.stepFloat;
            if (*item.floatValue > item.maxFloat) *item.floatValue = item.minFloat;
            if (item.action) item.action();
            AUDIO::PLAY_SOUND_FRONTEND(-1, const_cast<char*>("NAV_UP_DOWN"), const_cast<char*>("HUD_FRONTEND_DEFAULT_SOUNDSET"), true);
        }
    }
}



void TornadoMenu::SpawnTornado() {
    Logger::Log("Menu: SpawnTornado starting...");
    if (!g_Factory) {
        Logger::Log("Menu: g_Factory is null!");
        return;
    }

    if (g_Factory->GetActiveVortexCount() > 0) {
        Logger::Log("Menu: Already a tornado active, multi-vortex disabled.");
        return;
    }

    // Original C# logic: remove particles in range and set wind
    GRAPHICS::REMOVE_PARTICLE_FX_IN_RANGE(0.0f, 0.0f, 0.0f, 500.0f);
    if (m_spawnInStorm) {
        GAMEPLAY::SET_WIND(70.0f);
    }

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    
    Vector3 spawnPos;
    
    if (m_spawnInFront) {
        // Spawn in front of player
        Vector3 forward = ENTITY::GET_ENTITY_FORWARD_VECTOR(playerPed);
        spawnPos = MathEx::Add(playerPos, MathEx::Multiply(forward, m_tornadoSpawnDistance));
    } else {
        // Spawn at random position around player
        float angle = (float)rand() / RAND_MAX * 6.28318f;
        spawnPos.x = playerPos.x + std::cos(angle) * m_tornadoSpawnDistance;
        spawnPos.y = playerPos.y + std::sin(angle) * m_tornadoSpawnDistance;
        spawnPos.z = playerPos.z;
    }
    
    Logger::Log("Menu: Calling CreateVortex...");
    if (g_Factory->CreateVortex(spawnPos)) {
        Logger::Log("Menu: CreateVortex succeeded.");
        if (m_notifications) {
            IniHelper::ShowNotification("Tornado spawned!");
        }
    } else {
        Logger::Log("Menu: CreateVortex failed (likely cooldown or already spawning).");
        if (m_notifications) {
            IniHelper::ShowNotification("~r~Failed to spawn tornado. Please wait.");
        }
    }
}

void TornadoMenu::DespawnTornado() {
    if (!g_Factory) return;
    g_Factory->RemoveAll();
    if (m_notifications) {
        IniHelper::ShowNotification("All tornadoes despawned!");
    }
}

void TornadoMenu::TeleportToTornado() {
    if (!g_Factory || g_Factory->GetActiveVortexCount() == 0) {
        if (m_notifications) {
            IniHelper::ShowNotification("No active tornado found.");
        }
        return;
    }

    TornadoVortex* vortex = g_Factory->GetFirstVortex();
    if (vortex) {
        Vector3 pos = vortex->GetPosition();
        Ped playerPed = PLAYER::PLAYER_PED_ID();
        
        // Teleport player slightly above ground at tornado position
        ENTITY::SET_ENTITY_COORDS_NO_OFFSET(playerPed, pos.x, pos.y, pos.z + 10.0f, false, false, false);
        
        if (m_notifications) {
            IniHelper::ShowNotification("Teleported to tornado.");
        }
    }
}
