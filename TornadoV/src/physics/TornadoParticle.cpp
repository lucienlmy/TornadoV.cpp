#include "TornadoParticle.h"
#include "TornadoVortex.h"
#include "TornadoMenu.h"
#include "natives.h"
#include "IniHelper.h"
#include "MathEx.h"
#include "Logger.h"
#include "main.h"
#include <cmath>
#include <random>

TornadoParticle::TornadoParticle(TornadoVortex* vortex, Vector3 position, Vector3 angle, 
                               const std::string& fxAsset, const std::string& fxName, 
                               float radius, int layerIdx, bool isCloud)
{
    Ref = SafeSetup(position);
    LayerIndex = layerIdx;
    float layerSep = IniHelper::GetValue("VortexAdvanced", "LayerSeparationAmount", 22.0f);
    _offset.x = 0;
    _offset.y = 0;
    _offset.z = layerSep * layerIdx;
    _rotation = MathEx::Euler(angle.x, angle.y, angle.z);
    _radius = radius;
    _angle = 0.0f;
    Parent = vortex;
    _centerPos = position;
    IsCloud = isCloud;
    _ptfx = std::make_unique<LoopedParticle>(fxAsset, fxName);
    _updateSkipCounter = layerIdx % UPDATE_SKIP_FREQUENCY;

    PostSetup();
}

TornadoParticle::~TornadoParticle() {
    Dispose();
}

Entity TornadoParticle::SafeSetup(Vector3 position) {
    Hash model = GAMEPLAY::GET_HASH_KEY(const_cast<char*>("prop_beach_volball02"));
    
    if (!STREAMING::HAS_MODEL_LOADED(model)) {
        STREAMING::REQUEST_MODEL(model);
        
        int timeout = 0;
        while (!STREAMING::HAS_MODEL_LOADED(model) && timeout < 50) {
            WAIT(0);
            timeout++;
        }

        if (!STREAMING::HAS_MODEL_LOADED(model)) {
            Logger::Error("TornadoParticle: Failed to load prop model in SafeSetup!");
            return 0;
        }
    }
    
    Entity prop = OBJECT::CREATE_OBJECT(model, position.x, position.y, position.z, false, false, false);
    if (!ENTITY::DOES_ENTITY_EXIST(prop)) {
        Logger::Error("TornadoParticle: Failed to create object!");
        return 0;
    }

    ENTITY::SET_ENTITY_COLLISION(prop, false, false);
    ENTITY::SET_ENTITY_VISIBLE(prop, false, false);
    ENTITY::FREEZE_ENTITY_POSITION(prop, false); 
    ENTITY::_0x3910051CCECDB00C(prop, false);
    
    // CRITICAL FIX: Prevent entity distance culling
    // This keeps particles active even when far from player or behind objects
    ENTITY::SET_ENTITY_AS_MISSION_ENTITY(prop, true, true);
    ENTITY::SET_ENTITY_ALPHA(prop, 0, false);
    
    return prop;
}

void TornadoParticle::PostSetup() {
    int maxLayers = IniHelper::GetValue("VortexAdvanced", "MaxParticleLayers", 48);
    if (maxLayers < 1) maxLayers = 1; // Prevent division by zero
    
    _layerMask = 1.0f - (float)LayerIndex / (maxLayers * 4);
    _layerMask *= 0.1f * LayerIndex;
    _layerMask = 1.0f - _layerMask;
    if (_layerMask <= 0.3f) _layerMask = 0.3f;

    RefreshCache();
}

void TornadoParticle::RefreshCache() {
    _cachedRotationSpeed = TornadoMenu::m_rotationSpeed;
    _cachedLayerSeparation = TornadoMenu::m_layerSeparation;
    _lastCacheTime = GAMEPLAY::GET_GAME_TIMER();
}

void TornadoParticle::OnUpdate(int gameTime) {
    // OPTIMIZATION: Frame skipping - only update every Nth frame based on layer
    // _updateSkipCounter++;
    // if (_updateSkipCounter < UPDATE_SKIP_FREQUENCY)
    //     return;
    // _updateSkipCounter = 0;

    if (gameTime - _lastCacheTime > 10000)
    { 
        RefreshCache();
    }

    if (!ENTITY::DOES_ENTITY_EXIST(Ref))
    { 
        RemoveFx();
        return;
    }

    _centerPos = MathEx::Add(Parent->GetPosition(), _offset);

    // Normalize angle before calculations
    if (_angle > 6.28318f)
        _angle -= 6.28318f;
    else if (_angle < -6.28318f)
        _angle += 6.28318f;

    float cosAngle = std::cos(_angle);
    float sinAngle = std::sin(_angle);

    // MATCH C# TParticle.cs: new Vector3(_radius * cosAngle, _radius * sinAngle, 0)
    // Note: C# Vector3 uses X, Y, Z. In TParticle.cs it's (X, Y, 0) relative to rotation.
    Vector3 relativePos = { _radius * cosAngle, 0, _radius * sinAngle, 0, 0.0f, 0 };
    Vector3 finalPos = MathEx::Add(_centerPos, MathEx::MultiplyVector(relativePos, _rotation));

    ENTITY::SET_ENTITY_COORDS(Ref, finalPos.x, finalPos.y, finalPos.z, false, false, false, false);

    // MATCH C# TParticle.cs: Use Game.LastFrameTime for rotation
    // Game.LastFrameTime in SHVDN is the time between the current and previous frame.
    // In SHV, GAMEPLAY::GET_FRAME_TIME() returns this value.
    float lastFrameTime = GAMEPLAY::GET_FRAME_TIME();
    float rotationSpeed = _cachedRotationSpeed;
    
    if (TornadoMenu::m_reverseRotation) {
        rotationSpeed = -rotationSpeed;
    }

    if (IsCloud)
        _angle -= rotationSpeed * 0.16f * lastFrameTime;
    else
        _angle -= rotationSpeed * _layerMask * lastFrameTime;
}

void TornadoParticle::StartFx(float scale) {
    if (!ENTITY::DOES_ENTITY_EXIST(Ref)) return;

    if (!_ptfx->IsLoaded()) {
        _ptfx->Load();
        
        int timeout = 0;
        while (!_ptfx->IsLoaded() && timeout < 50) {
            WAIT(0);
            timeout++;
        }

        if (!_ptfx->IsLoaded()) {
            Logger::Error("TornadoParticle: PTFX asset " + _ptfx->GetAssetName() + " failed to load!");
            return;
        }
    }

    _ptfx->Start(Ref, scale);
}

void TornadoParticle::RemoveFx() {
    if (_ptfx) {
        _ptfx->Remove();
    }
}

void TornadoParticle::Dispose() {
    RemoveFx();

    if (ENTITY::DOES_ENTITY_EXIST(Ref)) {
        ENTITY::DELETE_ENTITY(&Ref);
    }
}
