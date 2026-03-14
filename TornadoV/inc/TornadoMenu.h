#pragma once

#include "types.h"
#include "XmlHelper.h"
#include <string>
#include <vector>
#include <functional>

enum class MenuItemType {
    Button,
    Checkbox,
    Submenu,
    Int,
    Float
};

struct MenuItem {
    std::string text;
    MenuItemType type;
    bool* checkboxValue;
    int* intValue;
    float* floatValue;
    int minInt, maxInt, stepInt;
    float minFloat, maxFloat, stepFloat;
    int targetSubmenu;
    std::function<void()> action;

    MenuItem(std::string t, std::function<void()> a) 
        : text(t), type(MenuItemType::Button), checkboxValue(nullptr), intValue(nullptr), floatValue(nullptr), targetSubmenu(-1), action(a) {}
    
    MenuItem(std::string t, bool* val, std::function<void()> a = nullptr) 
        : text(t), type(MenuItemType::Checkbox), checkboxValue(val), intValue(nullptr), floatValue(nullptr), targetSubmenu(-1), action(a) {}

    MenuItem(std::string t, int submenu) 
        : text(t), type(MenuItemType::Submenu), checkboxValue(nullptr), intValue(nullptr), floatValue(nullptr), targetSubmenu(submenu), action(nullptr) {}

    MenuItem(std::string t, int* val, int min, int max, int step, std::function<void()> a = nullptr)
        : text(t), type(MenuItemType::Int), checkboxValue(nullptr), intValue(val), floatValue(nullptr), 
          minInt(min), maxInt(max), stepInt(step), targetSubmenu(-1), action(a) {}

    MenuItem(std::string t, float* val, float min, float max, float step, std::function<void()> a = nullptr)
        : text(t), type(MenuItemType::Float), checkboxValue(nullptr), intValue(nullptr), floatValue(val), 
          minFloat(min), maxFloat(max), stepFloat(step), targetSubmenu(-1), action(a) {}
};

struct Submenu {
    std::string title;
    std::string subtitle;
    std::vector<MenuItem> items;
};

class TornadoMenu {
public:
    static void Initialize();
    static void OnTick();
    static void OnKeyDown(DWORD key);
    
    static bool IsVisible() { return m_visible; }
    static void SetVisible(bool visible) { m_visible = visible; }

    // Settings (mirrored from IniHelper/Menu.cs)
    static bool m_movementEnabled;
    static bool m_reverseRotation;
    static bool m_cloudTopEnabled;
    static bool m_cloudTopParticlesEnabled;
    static bool m_surfaceDetectionEnabled;
    static bool m_useInternalPool;
    static bool m_particleMod;
    static bool m_notifications;
    static bool m_spawnInStorm;
    static bool m_enableEAS;
    static bool m_enableSirens;
    static bool m_enableTornadoSound;
    static float m_sirenVolume;
    static float m_tornadoVolume;
    static float m_easVolume;
    static float m_moveSpeedScale;
    static float m_maxEntityDistance;
    static int m_maxEntityCount;
    static DWORD m_toggleKey;
    static DWORD m_tornadoHotkey;

    // INI options
    static float m_lodDistance;
    static bool m_drawBlip;
    static bool m_affectPlayer;

    // Physics
    static float m_vortexRadius;
    static int m_particlesPerLayer;
    static int m_maxParticleLayers;
    static float m_layerSeparation;
    static float m_rotationSpeed;
    static float m_vortexVerticalForceScale;
    static float m_vortexHorizontalForceScale;
    static float m_vortexMaxEntitySpeed;

    // Tornado customization settings
    static float m_tornadoSpawnDistance;
    static bool m_followPlayer;
    static bool m_spawnInFront;
    static float m_tornadoMaxDistance;

    // UI Settings
    static float m_menuX;
    static float m_menuY;
    static int m_themeR, m_themeG, m_themeB;
    static int m_bgR, m_bgG, m_bgB, m_bgA;
    static int m_selectionR, m_selectionG, m_selectionB;
    static int m_titleTextR, m_titleTextG, m_titleTextB;
    static int m_subtitleTextR, m_subtitleTextG, m_subtitleTextB;
    static int m_countTextR, m_countTextG, m_countTextB;
    static int m_intStep;
    static float m_floatStep;

private:
    static void DrawMenu();
    static void HandleInput();
    static void DrawRect(float x, float y, float width, float height, int r, int g, int b, int a);
    static void DrawText(std::string text, float x, float y, float scale, int font, int r, int g, int b, int a, bool center = false, bool right = false, bool outline = false);
    
    static void SetupMenus();
    static bool IsKeyDownWithRepeat(DWORD key);
    static void CheckResolution();
    static std::string GetUserInput(const std::string& title, const std::string& defaultText, int maxLength);
    static DWORD StringToKey(const std::string& keyStr);
    static std::string KeyToString(DWORD key);

public:
    static void SpawnTornado();
    static void DespawnTornado();
    static void TeleportToTornado();
    static void RefreshVortexSettings();

private:
    static bool m_visible;
    static int m_currentSubmenu;
    static int m_currentOption;
    static int m_scrollOffset;
    static const int MAX_VISIBLE_OPTIONS = 10;
    static std::vector<Submenu> m_submenus;
    
    static int m_lastScreenWidth;
    static int m_lastScreenHeight;
    static float m_pixelX;
    static float m_pixelY;

    // XML Styling variables
    static XmlHelper::Color m_titleBoxColor;
    static XmlHelper::Color m_subtitleBoxColor;
    static XmlHelper::Color m_backgroundColor;
    static XmlHelper::Color m_selectionBarColor;
    static XmlHelper::Color m_titleTextColor;
    static XmlHelper::Color m_subtitleTextColor;
    static XmlHelper::Color m_countTextColor;
    static XmlHelper::Color m_optionNormalColor;
    static XmlHelper::Color m_optionSelectedColor;

    static DWORD m_lastKey;
    static DWORD m_lastRepeatTime;
    static int m_repeatCount;

    static const float MENU_WIDTH;
    static const float TITLE_HEIGHT;
    static const float OPTION_HEIGHT;
};
