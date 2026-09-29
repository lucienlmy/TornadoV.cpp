#pragma once
#include <vector>
#include <map>
#include <memory>
#include "types.h"
#include "MathEx.h"
#include "LoopedParticle.h"

class TornadoParticle;

struct ActiveEntity {
    Entity entity;
    float xBias;
    float yBias;
    bool isPlayer;

    ActiveEntity() : entity(0), xBias(0), yBias(0), isPlayer(false) {}
    ActiveEntity(Entity ent, float x, float y, bool player) 
        : entity(ent), xBias(x), yBias(y), isPlayer(player) {}
};

class TornadoVortex {
public:
    TornadoVortex(Vector3 initialPosition, bool neverDespawn);
    ~TornadoVortex();

    void Build();
    void OnUpdate(int gameTime);
    void Dispose();

    Vector3 Position;
    bool DespawnRequested;

    void ChangeDestination(bool trackToPlayer);
    Vector3 GetPosition() const { return Position; }
    void RefreshCachedVars();

private:
    void CollectNearbyEntities(int gameTime, float maxDistanceDelta);
    void UpdatePulledEntities(int gameTime, float maxDistanceDelta);
    void AddEntity(ActiveEntity entity);
    void ReleaseEntity(int entityHandle);

    std::vector<std::unique_ptr<TornadoParticle>> _particles;
    int _aliveTime;
    int _createdTime;
    int _nextUpdateTime;
    int _lastFullUpdateTime;
    int _lifeSpan;
    
    std::map<int, ActiveEntity> _pulledEntities;
    std::vector<int> _pendingRemovalEntities;
    std::vector<std::pair<int, ActiveEntity>> _entitySnapshot;

    Vector3 _position;
    Vector3 _destination;

    float ForceScale = 3.0f;
    float InternalForcesDist = 5.0f;
    float MaxEntityDist = 57.0f;

    int _lastPlayerShapeTestTime;
    bool _lastRaycastResultFailed;

    float _cachedVerticalForce;
    float _cachedHorizontalForce;
    float _cachedTopSpeed;
    int _lastVarCacheTime;

    int _updateFrameCounter;
    int _soundUpdateFrameCounter;
    static const int PARTICLE_UPDATE_INTERVAL = 2;
    static const int SOUND_UPDATE_INTERVAL = 5;
    int MaxEntityCount = 200;

    // Helper for blip (not in C# but needed for SHV)
    Blip m_blip;

    unsigned int m_soundHandle;
};
