#include "TornadoFactory.h"
#include "TornadoMenu.h"
#include "natives.h"
#include "MathEx.h"
#include "IniHelper.h"
#include "Logger.h"
#include "AudioManager.h"
#include <algorithm>
#include <cmath>

TornadoFactory::TornadoFactory()
    : m_spawnDelayAdditive(0), m_spawnDelayStartTime(0),
      m_lastSpawnAttempt(0), m_lastSpawnCompleteTime(0),
      m_spawnInProgress(false), m_isScheduledSpawn(false), m_delaySpawn(false),
      m_easHandle(0), m_sirenHandle(0) {
}

TornadoFactory::~TornadoFactory() {
    Dispose();
}

TornadoVortex* TornadoFactory::CreateVortex(Vector3 position) {
    int gameTime = GAMEPLAY::GET_GAME_TIMER();
    Logger::Log("Factory: CreateVortex at (" + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z) + ")");
    
    // OPTIMIZATION: Enforce cooldown between spawns
    if (gameTime - m_lastSpawnCompleteTime < SPAWN_COOLDOWN) {
        Logger::Log("Factory: Spawn rejected (cooldown)");
        return nullptr;
    }
    if (m_spawnInProgress) {
        Logger::Log("Factory: Spawn rejected (already in progress)");
        return nullptr;
    }
    if (std::isnan(position.x) || std::isnan(position.y) || std::isnan(position.z)) return nullptr;

    m_spawnInProgress = true; // Set BEFORE any operations

    // Maintain limit by removing oldest if necessary
    if (m_activeVortexList.size() >= VortexLimit) {
        m_activeVortexList.front()->Dispose();
        m_activeVortexList.erase(m_activeVortexList.begin());
    }

    float groundZ;
    GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(position.x, position.y, 1000.0f, &groundZ, false);
    
    if (groundZ < -1000.0f || std::isnan(groundZ)) {
        groundZ = position.z;
    }

    position.z = groundZ - 10.0f;

    auto tVortex = std::make_unique<TornadoVortex>(position, false);

    try {
        // OPTIMIZATION: Clear old particles before building new ones
        GRAPHICS::REMOVE_PARTICLE_FX_IN_RANGE(position.x, position.y, position.z, 200.0f);

        // Wait one frame to let particles clear
        WAIT(0);

        Logger::Log("Factory: Calling Build()...");
        tVortex->Build();
        Logger::Log("Factory: Build() returned.");

        // OPTIMIZATION: Wait another frame after building
        WAIT(0);
    }
    catch (const std::exception& e) {
        Logger::Error("Factory: Error during Build: " + std::string(e.what()));
        m_spawnInProgress = false;
        return nullptr;
    }
    catch (...) {
        Logger::Error("Factory: Unknown error during Build");
        m_spawnInProgress = false;
        return nullptr;
    }

    TornadoVortex* ptr = tVortex.get();
    m_activeVortexList.push_back(std::move(tVortex));

    // Play Global Sounds (2D) if not already playing
    if (TornadoMenu::m_enableSirens || (TornadoMenu::m_enableEAS && m_easHandle == 0)) {
        if (TornadoMenu::m_enableEAS && m_easHandle == 0) {
            m_easHandle = AudioManager::Get().Play2D("eas_beeps", TornadoMenu::m_easVolume, false);
        }
        if (TornadoMenu::m_enableSirens && m_sirenHandle == 0) {
            m_sirenHandle = AudioManager::Get().Play2D("city_siren", TornadoMenu::m_sirenVolume, false);
        }
    }

    if (ptr && TornadoMenu::m_notifications) {
        IniHelper::ShowNotification("~g~Tornado spawned nearby.");
    }

    m_lastSpawnCompleteTime = GAMEPLAY::GET_GAME_TIMER(); // OPTIMIZATION: Track completion time
    m_spawnInProgress = false; // Reset flag

    return ptr;
}

void TornadoFactory::OnUpdate(int gameTime) {
    if (m_activeVortexList.empty()) {
        // Stop global sounds if they are playing
        if (m_easHandle != 0) {
            AudioManager::Get().Stop(m_easHandle);
            m_easHandle = 0;
        }
        if (m_sirenHandle != 0) {
            AudioManager::Get().Stop(m_sirenHandle);
            m_sirenHandle = 0;
        }

        if (TornadoMenu::m_spawnInStorm) {
                bool isStorming = GAMEPLAY::GET_RAIN_LEVEL() > 0.1f || 
                                 GAMEPLAY::IS_PREV_WEATHER_TYPE(const_cast<char*>("CLEARING")) ||
                                 GAMEPLAY::IS_PREV_WEATHER_TYPE(const_cast<char*>("THUNDER")) ||
                                 GAMEPLAY::IS_PREV_WEATHER_TYPE(const_cast<char*>("RAIN")) ||
                                 GAMEPLAY::IS_PREV_WEATHER_TYPE(const_cast<char*>("BLIZZARD")) ||
                                 GAMEPLAY::IS_PREV_WEATHER_TYPE(const_cast<char*>("SNOWLIGHT")) ||
                                 GAMEPLAY::IS_PREV_WEATHER_TYPE(const_cast<char*>("XMAS"));

                if (isStorming && !m_spawnInProgress && !m_isScheduledSpawn && gameTime - m_lastSpawnAttempt > 1000) {
                    // Increased probability from 0.15f to 0.30f for better "Spawn In Storm" responsiveness
                    if ((float)rand() / RAND_MAX < 0.30f) {
                        m_spawnDelayStartTime = gameTime;
                        m_spawnDelayAdditive = rand() % 20000; // 0-20s delay

                        GAMEPLAY::SET_WIND_SPEED(70.0f);
                        m_isScheduledSpawn = true;
                        Logger::Log("Factory: Storm detected, scheduled random spawn in " + std::to_string(m_spawnDelayAdditive) + "ms");
                    }
                    m_lastSpawnAttempt = gameTime;
                }
        }
    }

    if (m_isScheduledSpawn && m_spawnDelayStartTime != 0) {
        if (gameTime - m_spawnDelayStartTime > m_spawnDelayAdditive) {
            Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true);
            float angle = (float)rand() / RAND_MAX * 6.28318f;
            
            // Use TornadoSpawnDistance setting instead of hardcoded 200-400 range
            float baseDistance = TornadoMenu::m_tornadoSpawnDistance;
            float distanceVariation = TornadoMenu::m_tornadoSpawnDistance * 0.5f; // 50% variation
            float dist = baseDistance + (float)rand() / RAND_MAX * distanceVariation;
            
            // If SpawnInFront is true, bias the angle towards the player's forward direction
            if (TornadoMenu::m_spawnInFront) {
                Vector3 playerForward = ENTITY::GET_ENTITY_FORWARD_VECTOR(PLAYER::PLAYER_PED_ID());
                float playerAngle = std::atan2(playerForward.y, playerForward.x);
                // Bias angle towards player's forward direction with some randomness
                angle = playerAngle + ((float)rand() / RAND_MAX - 0.5f) * 1.5708f; // ±90 degrees
            }
            
            playerPos.x += std::cos(angle) * dist;
            playerPos.y += std::sin(angle) * dist;

            m_isScheduledSpawn = false; // Reset before calling CreateVortex
            CreateVortex(playerPos);
            m_spawnDelayStartTime = 0;
            m_spawnDelayAdditive = 0;
        }
    }

    for (auto it = m_activeVortexList.begin(); it != m_activeVortexList.end();) {
        if ((*it)->DespawnRequested) {
            (*it)->Dispose();
            it = m_activeVortexList.erase(it);
        } else {
            (*it)->OnUpdate(gameTime);
            ++it;
        }
    }

    // Update global sound volumes
    if (m_easHandle != 0) {
        if (TornadoMenu::m_enableEAS) {
            AudioManager::Get().SetVolume(m_easHandle, TornadoMenu::m_easVolume);
        } else {
            AudioManager::Get().Stop(m_easHandle);
            m_easHandle = 0;
        }
    }
    if (m_sirenHandle != 0) {
        if (TornadoMenu::m_enableSirens) {
            AudioManager::Get().SetVolume(m_sirenHandle, TornadoMenu::m_sirenVolume);
        } else {
            AudioManager::Get().Stop(m_sirenHandle);
            m_sirenHandle = 0;
        }
    }
}

void TornadoFactory::RemoveAll() {
    m_spawnInProgress = false;

    if (m_easHandle != 0) {
        AudioManager::Get().Stop(m_easHandle);
        m_easHandle = 0;
    }
    if (m_sirenHandle != 0) {
        AudioManager::Get().Stop(m_sirenHandle);
        m_sirenHandle = 0;
    }

    for (auto& vortex : m_activeVortexList) {
        vortex->Dispose();
    }
    m_activeVortexList.clear();

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true);
    GRAPHICS::REMOVE_PARTICLE_FX_IN_RANGE(playerPos.x, playerPos.y, playerPos.z, 1000.0f);
}

void TornadoFactory::RefreshAllVortexSettings() {
    for (auto& vortex : m_activeVortexList) {
        vortex->RefreshCachedVars();
    }
}

void TornadoFactory::Dispose() {
    RemoveAll();
}
