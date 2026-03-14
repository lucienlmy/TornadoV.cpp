#include "TornadoVortex.h"
#include "TornadoParticle.h"
#include "TornadoMenu.h"
#include "IniHelper.h"
#include "Logger.h"
#include "natives.h"
#include "MathEx.h"
#include "AudioManager.h"
#include <algorithm>
#include <cmath>
#include <random>

TornadoVortex::TornadoVortex(Vector3 initialPosition, bool neverDespawn)
    : _position(initialPosition), _destination({ 0.0f, 0, 0.0f, 0, 0.0f, 0 }), _despawnRequested(false), 
      m_blip(0), _updateFrameCounter(0), m_soundHandle(0) {
    
    Position = initialPosition;
    _createdTime = GAMEPLAY::GET_GAME_TIMER();
    
    // Probability.GetInteger(160000, 600000)
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(160000, 600000);
    _lifeSpan = neverDespawn ? -1 : dis(gen); 
    
    RefreshCachedVars();

    // Start 3D roar
    if (TornadoMenu::m_enableTornadoSound) {
        m_soundHandle = AudioManager::Get().Play3D("tornado_loop", _position.x, _position.y, _position.z, TornadoMenu::m_tornadoVolume, true);
    }
}

TornadoVortex::~TornadoVortex() {
    Dispose();
}

void TornadoVortex::RefreshCachedVars() {
    _cachedVerticalForce = TornadoMenu::m_vortexVerticalForceScale;
    _cachedHorizontalForce = TornadoMenu::m_vortexHorizontalForceScale;
    _cachedTopSpeed = TornadoMenu::m_vortexMaxEntitySpeed;
    
    // Use the values from TornadoMenu which are synchronized with the menu UI
    MaxEntityDist = TornadoMenu::m_maxEntityDistance;
    MaxEntityCount = TornadoMenu::m_maxEntityCount;
    
    _lastVarCacheTime = GAMEPLAY::GET_GAME_TIMER();
}

void TornadoVortex::ChangeDestination(bool trackToPlayer) {
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(playerPed, true);

    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> angleDis(0.0f, 6.28318f);
    std::uniform_real_distribution<float> distDis130(0.0f, 130.0f);
    std::uniform_real_distribution<float> distDis100(0.0f, 100.0f);

    for (int i = 0; i < 50; i++) {
        if (trackToPlayer) {
            float angle = angleDis(gen);
            float dist = distDis130(gen);
            _destination.x = playerPos.x + std::cos(angle) * dist;
            _destination.y = playerPos.y + std::sin(angle) * dist;
        } else {
            float angle = angleDis(gen);
            float dist = distDis100(gen);
            _destination.x = _destination.x + std::cos(angle) * dist;
            _destination.y = _destination.y + std::sin(angle) * dist;
        }

        float groundZ;
        if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(_destination.x, _destination.y, 1000.0f, &groundZ, false)) {
            _destination.z = groundZ - 10.0f;
        }

        Vector3 outPos;
        if (PATHFIND::GET_CLOSEST_VEHICLE_NODE(_destination.x, _destination.y, _destination.z, &outPos, 1, 3.0f, 0)) {
            if (MathEx::Distance(_destination, outPos) < 40.0f && std::abs(outPos.z - _destination.z) < 10.0f) {
                return; // Found a valid destination
            }
        }
        
        // Safety yield if this is taking too long
        if (i > 10 && i % 5 == 0) {
            WAIT(0);
        }
    }
    
    // Fallback if no vehicle node found after 50 attempts
    if (trackToPlayer) {
        _destination = playerPos;
    }
}

void TornadoVortex::Build() {
    Logger::Log("Vortex: Build starting...");
    float radius = IniHelper::GetValue("Vortex", "VortexRadius", 9.4f);
    int particleCount = IniHelper::GetValue("VortexAdvanced", "ParticlesPerLayer", 9);
    int maxLayers = IniHelper::GetValue("VortexAdvanced", "MaxParticleLayers", 48);
    std::string particleAsset = IniHelper::GetValue("VortexAdvanced", "ParticleAsset", "core");
    std::string particleName = IniHelper::GetValue("VortexAdvanced", "ParticleName", "ent_amb_smoke_foundry");
    bool enableClouds = TornadoMenu::m_cloudTopEnabled;

    Logger::Log("Vortex: Layers=" + std::to_string(maxLayers) + ", ParticlesPerLayer=" + std::to_string(particleCount));

    // OPTIMIZATION: Reduce particle count significantly for better performance
    // User reported "Tornado spawned" but no tornado - let's ensure we build enough layers
    maxLayers = (std::min)(maxLayers, 64); // Increased cap for visibility
    particleCount = (std::min)(particleCount, 12); // Increased cap for density
    if (particleCount < 1) particleCount = 1; // Prevent division by zero

    int multiplier = 360 / particleCount;
    float particleSize = 3.0685f;
    int layers = enableClouds ? 8 : maxLayers;

    float layerSepScale = IniHelper::GetValue("VortexAdvanced", "LayerSeparationAmount", 22.0f);
    if (layerSepScale < 1.0f) layerSepScale = 22.0f; // Safety default if INI is broken

    Logger::Log("Vortex: Requesting assets...");
    
    // Ensure assets are loaded before building
    bool isCore = (particleAsset == "core");
    if (!isCore) {
        Logger::Log("Vortex: Requesting PTFX asset: " + particleAsset);
        STREAMING::REQUEST_NAMED_PTFX_ASSET(const_cast<char*>(particleAsset.c_str()));
    }
    
    Logger::Log("Vortex: Requesting secondary PTFX asset: scr_agencyheistb");
    STREAMING::REQUEST_NAMED_PTFX_ASSET(const_cast<char*>("scr_agencyheistb"));
    
    Hash model = GAMEPLAY::GET_HASH_KEY(const_cast<char*>("prop_beach_volball02"));
    Logger::Log("Vortex: Requesting model: prop_beach_volball02");
    STREAMING::REQUEST_MODEL(model);

    int timeout = 0;
    Logger::Log("Vortex: Waiting for assets to load (max 5s)...");
    while (timeout < 300) { // 5 seconds
        bool ptfx1Loaded = isCore || STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(const_cast<char*>(particleAsset.c_str()));
        bool ptfx2Loaded = STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(const_cast<char*>("scr_agencyheistb"));
        bool modelLoaded = STREAMING::HAS_MODEL_LOADED(model);

        if (ptfx1Loaded && ptfx2Loaded && modelLoaded) {
            Logger::Log("Vortex: All assets loaded.");
            break;
        }

        WAIT(0);
        timeout++;
    }
    
    if (timeout >= 300) {
        Logger::Log("Vortex: Some assets not loaded after 5s, proceeding anyway.");
    }

    Logger::Log("Vortex: Assets loaded. Building " + std::to_string(layers) + " layers...");

    for (int layerIdx = 0; layerIdx < layers; layerIdx++) {
        int particlesThisLayer = (layerIdx > layers - 4) ? particleCount + 2 : particleCount;

        for (int angle = 0; angle < particlesThisLayer; angle++) {
            Vector3 pos = _position;
            pos.z += layerSepScale * layerIdx;
            Vector3 rot = { (float)(angle * multiplier), 0, 0.0f, 0, 0.0f, 0 }; // Initialize padding

            if (TornadoMenu::m_particleMod && layerIdx < 2 && angle % 2 == 0) {
                auto extraParticle = std::make_unique<TornadoParticle>(this, pos, rot, "scr_agencyheistb", "scr_env_agency3b_smoke", radius, layerIdx);
                extraParticle->StartFx(4.7f);
                
                // MATCH C# Shocking Event
                if (ENTITY::DOES_ENTITY_EXIST(extraParticle->Ref)) {
                    DECISIONEVENT::ADD_SHOCKING_EVENT_FOR_ENTITY(86, extraParticle->Ref, 0.0f);
                }
                
                _particles.push_back(std::move(extraParticle));
            }

            bool isTop = false;
            if (enableClouds && layerIdx > layers - 3) {
                pos.z += 12.0f;
                particleSize += 6.0f;
                radius += 7.0f;
                isTop = true;
            }

            auto mainParticle = std::make_unique<TornadoParticle>(this, pos, rot, particleAsset, particleName, radius, layerIdx, isTop);
            mainParticle->StartFx(particleSize);
            
            // MATCH C# Shocking Event
            if (ENTITY::DOES_ENTITY_EXIST(mainParticle->Ref)) {
                DECISIONEVENT::ADD_SHOCKING_EVENT_FOR_ENTITY(86, mainParticle->Ref, 0.0f);
            }

            radius += 0.08f * (0.72f * layerIdx);
            particleSize += 0.01f * (0.12f * layerIdx);
            _particles.push_back(std::move(mainParticle));

            // Yield more frequently during build to prevent watchdog trigger
            if (_particles.size() % 10 == 0) {
                WAIT(0);
            }
        }
        
        Logger::Log("Vortex: Built layer " + std::to_string(layerIdx) + " (" + std::to_string(_particles.size()) + " total particles)");
    }
    Logger::Log("Vortex: Build complete. Total particles: " + std::to_string(_particles.size()));
}

void TornadoVortex::CollectNearbyEntities(int gameTime, float maxDistanceDelta) {
    if (gameTime < _nextUpdateTime) return;
    
    if (_pulledEntities.size() >= MaxEntityCount) {
        // Still scan occasionally to replace invalid entities, but slower
        _nextUpdateTime = gameTime + 2000;
        return;
    }

    const int POOL_SIZE = 1024;
    int entities[POOL_SIZE];
    
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> scalarDis(-1.0f, 1.0f);

    int addedTotal = 0;
    // Increase limit significantly to ensure we don't "skip" entities in large radii
    // Processing 1024 entities' distance is fast enough for modern CPUs
    const int MAX_ADD_PER_TICK = 300; 

    // Helper to process entities from a pool
    auto processPool = [&](int count) {
        for (int i = 0; i < count; i++) {
            Entity ent = entities[i];
            if (!ENTITY::DOES_ENTITY_EXIST(ent)) continue;
            if (_pulledEntities.count(ent)) continue;
            if (addedTotal >= MAX_ADD_PER_TICK) break;
            if (_pulledEntities.size() >= MaxEntityCount) break;

            Vector3 pos = ENTITY::GET_ENTITY_COORDS(ent, true);
            float dist2d = MathEx::Distance2D(pos, _position);
            
            // THOROUGH SCAN: 
            // 1. Entities entering the outer radius
            // 2. Entities already inside the radius (anywhere)
            if (dist2d > maxDistanceDelta + 5.0f) continue;
            
            // Don't pull entities that are too high up already
            if (ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(ent) > 300.0f) continue;

            if (ENTITY::IS_ENTITY_A_PED(ent)) {
                if (!PED::IS_PED_RAGDOLL(ent)) {
                    PED::SET_PED_TO_RAGDOLL(ent, 800, 1500, 2, 1, 1, 0);
                }
            }

            // Check if this entity is the player (either ped or vehicle player is in)
            bool isPlayerEntity = false;
            if (ent == PLAYER::PLAYER_PED_ID()) {
                isPlayerEntity = true;
            } else if (ENTITY::IS_ENTITY_A_VEHICLE(ent)) {
                // Check if player is in this vehicle
                Ped playerPed = PLAYER::PLAYER_PED_ID();
                Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
                if (playerVehicle == ent) {
                    isPlayerEntity = true;
                }
            }

            AddEntity(ActiveEntity(ent, 3.0f * scalarDis(gen), 3.0f * scalarDis(gen), isPlayerEntity));
            addedTotal++;
        }
    };

    // Process all pools. 
    // We don't stop after Peds if we still have room, ensuring inner vehicles/objects are also caught.
    processPool(worldGetAllPeds(entities, POOL_SIZE));
    
    if (_pulledEntities.size() < MaxEntityCount && addedTotal < MAX_ADD_PER_TICK) {
        processPool(worldGetAllVehicles(entities, POOL_SIZE));
    }

    if (_pulledEntities.size() < MaxEntityCount && addedTotal < MAX_ADD_PER_TICK) {
        processPool(worldGetAllObjects(entities, POOL_SIZE));
    }

    // Dynamic update rate: C++ can handle much faster scanning than C#
    // 50ms (20 times per second) provides a near-instant response
    int nextUpdateDelay = 50; 
    if (_pulledEntities.size() >= MaxEntityCount) nextUpdateDelay = 1000;

    _nextUpdateTime = gameTime + nextUpdateDelay;
}

void TornadoVortex::UpdatePulledEntities(int gameTime, float maxDistanceDelta) {
    // OPTIMIZATION: Refresh cached vars every 5 seconds instead of reading every frame
    if (gameTime - _lastVarCacheTime > 5000) {
        RefreshCachedVars();
    }

    _pendingRemovalEntities.clear();
    _entitySnapshot.clear();
    for (auto const& [handle, activeEnt] : _pulledEntities) {
        _entitySnapshot.push_back({ handle, activeEnt });
    }

    int processedCount = 0;
    const int MAX_ENTITIES_PER_FRAME = 500; // Increased for C++ performance, ensures we process all entities in most cases

    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> floatDis(0.0f, 1.0f);
    std::uniform_real_distribution<float> scalarDis(-1.0f, 1.0f);

    for (auto const& kvp : _entitySnapshot) {
        int key = kvp.first;
        ActiveEntity value = kvp.second;
        Entity entity = value.entity;

        // CLEANUP: Always check existence and range for EVERY entity in the snapshot
        if (!ENTITY::DOES_ENTITY_EXIST(entity)) {
            ReleaseEntity(key);
            continue;
        }

        Vector3 pos = ENTITY::GET_ENTITY_COORDS(entity, true);
        float dist = MathEx::Distance2D(pos, _position);
        
        // Match collection filter to prevent immediate release: maxDistanceDelta + 4.0f
        if (dist > maxDistanceDelta + 4.0f || ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(entity) > 300.0f) {
            ReleaseEntity(key);
            continue;
        }

        // OPTIMIZATION: Limit expensive physics calculations per frame if we have many entities
        // However, we MUST NOT break the loop here, because we still need to check the remaining
        // entities for the existence/range cleanup above.
        if (processedCount >= MAX_ENTITIES_PER_FRAME) continue;
        processedCount++;

        // Fix narrowing conversion warnings by using explicit float initializers
        Vector3 targetPos = { _position.x + value.xBias, 0, _position.y + value.yBias, 0, pos.z, 0 };
        Vector3 dirVec = MathEx::Subtract(targetPos, pos);
        if (MathEx::Length(dirVec) < 0.0001f)
            continue;

        Vector3 direction = MathEx::Normalize(dirVec);
        float forceBias = floatDis(gen);
        float force = ForceScale * (forceBias + forceBias / (std::max)(dist, 1.0f));

        float verticalForce = _cachedVerticalForce;
        float horizontalForce = _cachedHorizontalForce;

        // Skip affecting player if the setting is disabled - this must check BEFORE any forces are applied
        if (value.isPlayer && !TornadoMenu::m_affectPlayer) {
            continue;
        }

        if (value.isPlayer) {
            verticalForce *= 1.62f;
            horizontalForce *= 1.2f;

            if (gameTime - _lastPlayerShapeTestTime > 1000) {
                int ray = WORLDPROBE::_CAST_RAY_POINT_TO_POINT(pos.x, pos.y, pos.z, targetPos.x, targetPos.y, targetPos.z, 1, entity, 7);
                BOOL hit;
                Vector3 endCoords, surfaceNormal;
                Entity entHit;
                WORLDPROBE::_GET_RAYCAST_RESULT(ray, &hit, &endCoords, &surfaceNormal, &entHit);
                _lastRaycastResultFailed = hit;
                _lastPlayerShapeTestTime = gameTime;
            }

            if (_lastRaycastResultFailed) continue;
        }

        Hash model = ENTITY::GET_ENTITY_MODEL(entity);
        if (VEHICLE::IS_THIS_MODEL_A_PLANE(model)) {
            force *= 6.0f;
            verticalForce *= 6.0f;
        }

        // SHV APPLY_FORCE_TO_ENTITY: matches SHVDN ApplyForce (p10=false, p11=true)
        // Force type 1 is Force, type 3 is Impulse. C# uses type 1 for ApplyForce in some contexts but let's check Helpers.
        // Actually, suction might need a stronger kick - let's try 3 (impulse) if 1 (force) is too weak, 
        // but C# ScriptHookVDotNet's ApplyForce usually maps to type 3 if impulse is true.
        // TVortex.cs uses entity.ApplyForce which is type 1 by default in SHVDN unless specified.
        ENTITY::APPLY_FORCE_TO_ENTITY(entity, 3, direction.x * horizontalForce, direction.y * horizontalForce, direction.z * horizontalForce, 
                                     floatDis(gen), 0.0f, scalarDis(gen), 0, false, true, true, false, true);
        
        // Apply Vertical Force
        // MATCH C# upDir = Vector3.Normalize(new Vector3(_position.X, _position.Y, _position.Z + 1000.0f) - entity.Position);
        Vector3 upTarget = { _position.x, 0, _position.y, 0, _position.z + 1000.0f, 0 };
        Vector3 upDir = MathEx::Normalize(MathEx::Subtract(upTarget, pos));
        // SHV APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS: matches Helpers.cs extension (p7=0, p8=1)
        ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(entity, 1, upDir.x * verticalForce, upDir.y * verticalForce, upDir.z * verticalForce, 0, 0, 1, 1);
        
        // Apply Rotational Force (Cross product)
        // MATCH C# entity.ApplyForceToCenterOfMass(Vector3.Normalize(cross) * force * horizontalForce);
        Vector3 worldUp = { 0.0f, 0, 0.0f, 0, 1.0f, 0 };
        Vector3 cross = MathEx::Cross(direction, worldUp);
        
        Vector3 normCross = MathEx::Normalize(cross);
        // SHV APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS: matches Helpers.cs extension (p7=0, p8=1)
        ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(entity, 1, normCross.x * force * horizontalForce, normCross.y * force * horizontalForce, normCross.z * force * horizontalForce, 0, 0, 1, 1);

        // Rumble/Shake for Player
        if (value.isPlayer && TornadoMenu::m_enableTornadoSound) {
            CAM::SHAKE_GAMEPLAY_CAM(const_cast<char*>("LARGE_EXPLOSION_SHAKE"), 0.012f * (std::max)(1.0f, 30.0f / (std::max)(dist, 1.0f)));
            CONTROLS::_SET_CONTROL_NORMAL(0, 214, 0.1f); // Set Rumble
        }

        if (ENTITY::IS_ENTITY_A_PED(entity)) {
            if (!PED::IS_PED_RAGDOLL(entity)) {
                PED::SET_PED_TO_RAGDOLL(entity, 800, 1500, 2, 1, 1, 0);
            }
        }

        ENTITY::SET_ENTITY_MAX_SPEED(entity, _cachedTopSpeed);
    }

    for (int handle : _pendingRemovalEntities) {
        _pulledEntities.erase(handle);
    }
}

void TornadoVortex::OnUpdate(int gameTime) {
    if (_lifeSpan > 0 && gameTime - _createdTime > _lifeSpan)
        _despawnRequested = true;

    if (TornadoMenu::m_movementEnabled) {
        if ((_destination.x == 0 && _destination.y == 0) || MathEx::Distance(_position, _destination) < 15.0f)
            ChangeDestination(TornadoMenu::m_followPlayer);  // Follow based on setting, not distance

        // REMOVE distance check - let FollowPlayer setting control behavior
        // Tornado should either follow always or never follow, not just when far
        
        Vector3 vTarget = MathEx::MoveTowards(_position, _destination, TornadoMenu::m_moveSpeedScale * 0.287f);
        _position = MathEx::Lerp(_position, vTarget, GAMEPLAY::GET_FRAME_TIME() * 20.0f);
    }

    Position = _position;
    DespawnRequested = _despawnRequested;

    // Update sound position
    if (m_soundHandle != 0) {
        if (TornadoMenu::m_enableTornadoSound) {
            AudioManager::Get().Update3DSound(m_soundHandle, _position.x, _position.y, _position.z);
            AudioManager::Get().SetVolume(m_soundHandle, TornadoMenu::m_tornadoVolume);
        } else {
            AudioManager::Get().Stop(m_soundHandle);
            m_soundHandle = 0;
        }
    } else if (TornadoMenu::m_enableTornadoSound) {
        m_soundHandle = AudioManager::Get().Play3D("tornado_loop", _position.x, _position.y, _position.z, TornadoMenu::m_tornadoVolume, true);
    }

    CollectNearbyEntities(gameTime, MaxEntityDist);
    UpdatePulledEntities(gameTime, MaxEntityDist);

    // Update blip
    if (TornadoMenu::m_drawBlip) {
        if (m_blip == 0) {
            m_blip = UI::ADD_BLIP_FOR_COORD(_position.x, _position.y, _position.z);
            UI::SET_BLIP_SPRITE(m_blip, 458);
            UI::SET_BLIP_COLOUR(m_blip, 5);
            UI::SET_BLIP_SCALE(m_blip, 1.0f);
            UI::BEGIN_TEXT_COMMAND_SET_BLIP_NAME((char*)"STRING");
            UI::_ADD_TEXT_COMPONENT_STRING((char*)"Tornado");
            UI::END_TEXT_COMMAND_SET_BLIP_NAME(m_blip);
        } else {
            UI::SET_BLIP_COORDS(m_blip, _position.x, _position.y, _position.z);
        }
    } else {
        if (m_blip != 0) {
            UI::REMOVE_BLIP(&m_blip);
            m_blip = 0;
        }
    }

    // MATCH C# behavior: Update particles every frame (no skipping)
    for (auto& p : _particles) {
        p->OnUpdate(gameTime);
    }
}

void TornadoVortex::AddEntity(ActiveEntity entity) {
    if (ENTITY::DOES_ENTITY_EXIST(entity.entity)) {
        _pulledEntities[entity.entity] = entity;
    }
}

void TornadoVortex::ReleaseEntity(int handle) {
    if (std::find(_pendingRemovalEntities.begin(), _pendingRemovalEntities.end(), handle) == _pendingRemovalEntities.end())
        _pendingRemovalEntities.push_back(handle);
}

void TornadoVortex::Dispose() {
    if (m_soundHandle != 0) {
        AudioManager::Get().Stop(m_soundHandle);
        m_soundHandle = 0;
    }

    if (m_blip != 0) {
        UI::REMOVE_BLIP(&m_blip);
        m_blip = 0;
    }
    
    // Clear particles - the unique_ptr destructor will call ~TornadoParticle() -> Dispose()
    _particles.clear();
    
    _pulledEntities.clear();
    _pendingRemovalEntities.clear();
    _entitySnapshot.clear();
}
