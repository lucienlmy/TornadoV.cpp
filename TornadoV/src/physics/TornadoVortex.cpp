#include "TornadoVortex.h"
#include "TornadoParticle.h"
#include "TornadoMenu.h"
#include "IniHelper.h"
#include "Logger.h"
#include "natives.h"
#include "MathEx.h"
#include "AudioManager.h"
#include "World.h"
#include <algorithm>
#include <cmath>
#include <random>

TornadoVortex::TornadoVortex(Vector3 initialPosition, bool neverDespawn)
    : _position(initialPosition), _destination({ 0.0f, 0, 0.0f, 0, 0.0f, 0 }), m_blip(0), _updateFrameCounter(0), _soundUpdateFrameCounter(0), m_soundHandle(0)
{
    Position = initialPosition;
    DespawnRequested = false;
    _createdTime = GAMEPLAY::GET_GAME_TIMER();

    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(160000, 600000);
    _lifeSpan = neverDespawn ? -1 : dis(gen);

    RefreshCachedVars();

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
        }
        else {
            float angle = angleDis(gen);
            float dist = distDis100(gen);
            _destination.x = _destination.x + std::cos(angle) * dist;
            _destination.y = _destination.y + std::sin(angle) * dist;
        }

        float groundZ;
        bool groundFound = GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(_destination.x, _destination.y, 1000.0f, &groundZ, false);
        if (groundFound && groundZ > -1000.0f && !std::isnan(groundZ))
            _destination.z = groundZ - 10.0f;

        Vector3 outPos;
        if (PATHFIND::GET_CLOSEST_VEHICLE_NODE(_destination.x, _destination.y, _destination.z, &outPos, 1, 3.0f, 0)) {
            if (MathEx::Distance(_destination, outPos) < 40.0f && std::abs(outPos.z - _destination.z) < 10.0f)
                return;
        }

        if (i > 10 && i % 5 == 0)
            WAIT(0);
    }

    if (trackToPlayer)
        _destination = playerPos;
}

void TornadoVortex::Build() {
    Logger::Log("Vortex: Build starting...");

    float radius = TornadoMenu::m_vortexRadius;
    int   particleCount = IniHelper::GetValue("VortexAdvanced", "ParticlesPerLayer", 9);
    int   maxLayers = IniHelper::GetValue("VortexAdvanced", "MaxParticleLayers", 48);
    std::string particleAsset = IniHelper::GetValue("VortexAdvanced", "ParticleAsset", std::string("core"));
    std::string particleName = IniHelper::GetValue("VortexAdvanced", "ParticleName", std::string("ent_amb_smoke_foundry"));
    bool enableClouds = TornadoMenu::m_cloudTopEnabled;

    // Match original C# caps exactly
    maxLayers = (std::min)(maxLayers, 36);
    particleCount = (std::min)(particleCount, 6);
    if (particleCount < 1) particleCount = 1;

    int   multiplier = 360 / particleCount;
    float particleSize = 3.0685f;
    int   layers = enableClouds ? 8 : maxLayers;

    float layerSepScale = IniHelper::GetValue("VortexAdvanced", "LayerSeparationAmount", 22.0f);
    if (layerSepScale < 1.0f) layerSepScale = 22.0f;

    bool isCore = (particleAsset == "core");
    if (!isCore)
        STREAMING::REQUEST_NAMED_PTFX_ASSET(const_cast<char*>(particleAsset.c_str()));
    STREAMING::REQUEST_NAMED_PTFX_ASSET(const_cast<char*>("scr_agencyheistb"));

    Hash model = GAMEPLAY::GET_HASH_KEY(const_cast<char*>("prop_beach_volball02"));
    STREAMING::REQUEST_MODEL(model);

    int timeout = 0;
    while (timeout < 300) {
        bool ptfx1Loaded = isCore || STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(const_cast<char*>(particleAsset.c_str()));
        bool ptfx2Loaded = STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(const_cast<char*>("scr_agencyheistb"));
        bool modelLoaded = STREAMING::HAS_MODEL_LOADED(model);
        if (ptfx1Loaded && ptfx2Loaded && modelLoaded) break;
        WAIT(0);
        timeout++;
    }

    Logger::Log("Vortex: Building " + std::to_string(layers) + " layers...");

    for (int layerIdx = 0; layerIdx < layers; layerIdx++) {
        int particlesThisLayer = (layerIdx > layers - 4) ? particleCount + 2 : particleCount;

        for (int angle = 0; angle < particlesThisLayer; angle++) {
            Vector3 pos = _position;
            pos.z += layerSepScale * layerIdx;
            Vector3 rot = { (float)(angle * multiplier), 0, 0.0f, 0, 0.0f, 0 };

            if (TornadoMenu::m_particleMod && layerIdx < 2 && angle % 2 == 0) {
                auto extra = std::make_unique<TornadoParticle>(this, pos, rot, "scr_agencyheistb", "scr_env_agency3b_smoke", radius, layerIdx);
                extra->StartFx(4.7f);
                if (ENTITY::DOES_ENTITY_EXIST(extra->Ref))
                    DECISIONEVENT::ADD_SHOCKING_EVENT_FOR_ENTITY(86, extra->Ref, 0.0f);
                _particles.push_back(std::move(extra));
            }

            bool isTop = false;
            if (enableClouds && layerIdx > layers - 3) {
                pos.z += 12.0f;
                particleSize += 6.0f;
                radius += 7.0f;
                isTop = true;
            }

            auto main = std::make_unique<TornadoParticle>(this, pos, rot, particleAsset, particleName, radius, layerIdx, isTop);
            main->StartFx(particleSize);
            if (ENTITY::DOES_ENTITY_EXIST(main->Ref))
                DECISIONEVENT::ADD_SHOCKING_EVENT_FOR_ENTITY(86, main->Ref, 0.0f);

            radius += 0.08f * (0.72f * layerIdx);
            particleSize += 0.01f * (0.12f * layerIdx);
            _particles.push_back(std::move(main));

            if (_particles.size() % 10 == 0)
                WAIT(0);
        }
    }

    Logger::Log("Vortex: Build complete. Total particles: " + std::to_string(_particles.size()));
}

void TornadoVortex::AddEntity(ActiveEntity entity) {
    if (ENTITY::DOES_ENTITY_EXIST(entity.entity))
        _pulledEntities[entity.entity] = entity;
}

void TornadoVortex::ReleaseEntity(int handle) {
    if (std::find(_pendingRemovalEntities.begin(), _pendingRemovalEntities.end(), handle) == _pendingRemovalEntities.end())
        _pendingRemovalEntities.push_back(handle);
}

// ---------------------------------------------------------------------------
// CollectNearbyEntities — 200ms scan interval, no per-cycle add cap.
// MaxEntityCount is driven by TornadoMenu (default raised to 500).
// ---------------------------------------------------------------------------
void TornadoVortex::CollectNearbyEntities(int gameTime, float maxDistanceDelta) {
    if (gameTime < _nextUpdateTime) return;

    if ((int)_pulledEntities.size() >= MaxEntityCount) {
        _nextUpdateTime = gameTime + 20;
        return;
    }

    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> scalarDis(-1.0f, 1.0f);

    std::vector<Entity> allEntities = World::GetNearbyEntities(_position, maxDistanceDelta + 10.0f);

    for (Entity ent : allEntities) {
        if ((int)_pulledEntities.size() >= MaxEntityCount) break;
        if (!ENTITY::DOES_ENTITY_EXIST(ent)) continue;
        if (_pulledEntities.count(ent)) continue;

        Vector3 pos = ENTITY::GET_ENTITY_COORDS(ent, true);
        float dist2d = MathEx::Distance2D(pos, _position);

        if (dist2d > maxDistanceDelta + 4.0f) continue;
        if (ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(ent) > 300.0f) continue;

        if (ENTITY::IS_ENTITY_A_PED(ent) && !PED::IS_PED_RAGDOLL(ent))
            PED::SET_PED_TO_RAGDOLL(ent, 800, 1500, 2, 1, 1, 0);

        bool isPlayerEntity = (ent == PLAYER::PLAYER_PED_ID());
        if (!isPlayerEntity && ENTITY::IS_ENTITY_A_VEHICLE(ent)) {
            Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);
            if (playerVehicle == ent)
                isPlayerEntity = true;
        }

        AddEntity(ActiveEntity(ent, 3.0f * scalarDis(gen), 3.0f * scalarDis(gen), isPlayerEntity));
    }

    _nextUpdateTime = gameTime + 200;
}

// ---------------------------------------------------------------------------
// UpdatePulledEntities — process ALL entities every frame.
// Existence/range checks come first (matching original C# order) so
// out-of-range entities are always released even at high entity counts.
// ---------------------------------------------------------------------------
void TornadoVortex::UpdatePulledEntities(int gameTime, float maxDistanceDelta) {
    if (gameTime - _lastVarCacheTime > 5000)
        RefreshCachedVars();

    _pendingRemovalEntities.clear();
    _entitySnapshot.clear();
    for (auto const& [handle, activeEnt] : _pulledEntities)
        _entitySnapshot.push_back({ handle, activeEnt });

    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> floatDis(0.0f, 1.0f);
    std::uniform_real_distribution<float> scalarDis(-1.0f, 1.0f);

    for (auto const& kvp : _entitySnapshot) {
        int key = kvp.first;
        ActiveEntity value = kvp.second;
        Entity entity = value.entity;

        if (!ENTITY::DOES_ENTITY_EXIST(entity)) {
            ReleaseEntity(key);
            continue;
        }

        Vector3 pos = ENTITY::GET_ENTITY_COORDS(entity, true);
        float dist = MathEx::Distance2D(pos, _position);

        if (dist > maxDistanceDelta - 13.0f || ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(entity) > 300.0f) {
            ReleaseEntity(key);
            continue;
        }
        
        // Prevent negative release distance
        if (maxDistanceDelta < 13.0f) {
            ReleaseEntity(key);
            continue;
        }

        Vector3 targetPos = { _position.x + value.xBias, 0, _position.y + value.yBias, 0, pos.z, 0 };
        Vector3 dirVec = MathEx::Subtract(targetPos, pos);
        if (MathEx::Length(dirVec) < 0.0001f) continue;

        Vector3 direction = MathEx::Normalize(dirVec);
        float forceBias = floatDis(gen);
        float force = ForceScale * (forceBias + forceBias / (std::max)(dist, 1.0f));

        float verticalForce = _cachedVerticalForce;
        float horizontalForce = _cachedHorizontalForce;

        if (value.isPlayer && !TornadoMenu::m_affectPlayer) continue;

        if (value.isPlayer) {
            verticalForce *= 1.62f;
            horizontalForce *= 1.2f;

            if (gameTime - _lastPlayerShapeTestTime > 1000) {
                int ray = WORLDPROBE::_CAST_RAY_POINT_TO_POINT(
                    pos.x, pos.y, pos.z,
                    targetPos.x, targetPos.y, targetPos.z,
                    1, entity, 7);
                BOOL hit; Vector3 endCoords, surfaceNormal; Entity entHit;
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

        ENTITY::APPLY_FORCE_TO_ENTITY(
            entity, 3,
            direction.x * horizontalForce, direction.y * horizontalForce, direction.z * horizontalForce,
            floatDis(gen), 0.0f, scalarDis(gen), 0, false, true, true, false, true);

        Vector3 upTarget = { _position.x, 0, _position.y, 0, _position.z + 1000.0f, 0 };
        Vector3 upDir = MathEx::Normalize(MathEx::Subtract(upTarget, pos));
        ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
            entity, 1,
            upDir.x * verticalForce, upDir.y * verticalForce, upDir.z * verticalForce,
            0, 0, 1, 1);

        Vector3 worldUp = { 0.0f, 0, 0.0f, 0, 1.0f, 0 };
        Vector3 normCross = MathEx::Normalize(MathEx::Cross(direction, worldUp));
        ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
            entity, 1,
            normCross.x * force * horizontalForce,
            normCross.y * force * horizontalForce,
            normCross.z * force * horizontalForce,
            0, 0, 1, 1);

        if (value.isPlayer && TornadoMenu::m_enableTornadoSound) {
            CAM::SHAKE_GAMEPLAY_CAM(
                const_cast<char*>("LARGE_EXPLOSION_SHAKE"),
                0.012f * (std::max)(1.0f, 30.0f / (std::max)(dist, 1.0f)));
            CONTROLS::_SET_CONTROL_NORMAL(0, 214, 0.1f);
        }

        if (ENTITY::IS_ENTITY_A_PED(entity) && !PED::IS_PED_RAGDOLL(entity))
            PED::SET_PED_TO_RAGDOLL(entity, 800, 1500, 2, 1, 1, 0);

        ENTITY::SET_ENTITY_MAX_SPEED(entity, _cachedTopSpeed);
    }

    for (int handle : _pendingRemovalEntities) {
        _pulledEntities.erase(handle);
        auto it = std::remove_if(_entitySnapshot.begin(), _entitySnapshot.end(),
            [handle](const std::pair<int, ActiveEntity>& p) { return p.first == handle; });
        _entitySnapshot.erase(it, _entitySnapshot.end());
    }
}

void TornadoVortex::OnUpdate(int gameTime) {
    if (_lifeSpan > 0 && gameTime - _createdTime > _lifeSpan) {
        Logger::Log("Vortex: Despawn requested (lifespan expired)");
        DespawnRequested = true;
    }

    if (TornadoMenu::m_movementEnabled) {
        Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true);
        
        // Change destination if tornado is too far from player
        if (MathEx::Distance(_position, playerPos) > TornadoMenu::m_tornadoMaxDistance * 0.8f) {
            ChangeDestination(true);
        }
        
        if ((_destination.x == 0 && _destination.y == 0) || MathEx::Distance(_position, _destination) < 15.0f)
            ChangeDestination(TornadoMenu::m_followPlayer);

        Vector3 vTarget = MathEx::MoveTowards(_position, _destination, TornadoMenu::m_moveSpeedScale * 0.287f);
        _position = MathEx::Lerp(_position, vTarget, GAMEPLAY::GET_FRAME_TIME() * 20.0f);
    }

    Position = _position;

    _soundUpdateFrameCounter++;
    if (m_soundHandle != 0) {
        if (TornadoMenu::m_enableTornadoSound) {
            if (_soundUpdateFrameCounter >= SOUND_UPDATE_INTERVAL) {
                AudioManager::Get().Update3DSound(m_soundHandle, _position.x, _position.y, _position.z);
                AudioManager::Get().SetVolume(m_soundHandle, TornadoMenu::m_tornadoVolume);
                _soundUpdateFrameCounter = 0;
            }
        }
        else {
            AudioManager::Get().Stop(m_soundHandle);
            m_soundHandle = 0;
        }
    }
    else if (TornadoMenu::m_enableTornadoSound) {
        m_soundHandle = AudioManager::Get().Play3D("tornado_loop", _position.x, _position.y, _position.z, TornadoMenu::m_tornadoVolume, true);
    }

    CollectNearbyEntities(gameTime, MaxEntityDist);
    UpdatePulledEntities(gameTime, MaxEntityDist);

    if (TornadoMenu::m_drawBlip) {
        if (m_blip == 0) {
            m_blip = UI::ADD_BLIP_FOR_COORD(_position.x, _position.y, _position.z);
            UI::SET_BLIP_SPRITE(m_blip, 458);
            UI::SET_BLIP_COLOUR(m_blip, 5);
            UI::SET_BLIP_SCALE(m_blip, 1.0f);
            UI::BEGIN_TEXT_COMMAND_SET_BLIP_NAME((char*)"STRING");
            UI::_ADD_TEXT_COMPONENT_STRING((char*)"Tornado");
            UI::END_TEXT_COMMAND_SET_BLIP_NAME(m_blip);
        }
        else {
            UI::SET_BLIP_COORDS(m_blip, _position.x, _position.y, _position.z);
        }
    }
    else {
        if (m_blip != 0) {
            UI::REMOVE_BLIP(&m_blip);
            m_blip = 0;
        }
    }

    for (auto& p : _particles)
        p->OnUpdate(gameTime);
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

    _particles.clear();
    _pulledEntities.clear();
    _pendingRemovalEntities.clear();
    _entitySnapshot.clear();
}