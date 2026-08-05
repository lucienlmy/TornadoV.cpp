#pragma once
#include "natives.h"
#include "MathEx.h"
#include <vector>

class World {
public:
    // Gets all entities within a specified radius from a position
    // Returns a vector of entity handles
    static std::vector<Entity> GetNearbyEntities(Vector3 position, float radius);

private:
    // Helper functions to get all entities from each pool
    static std::vector<Entity> GetAllPedsFromPool();
    static std::vector<Entity> GetAllVehiclesFromPool();
    static std::vector<Entity> GetAllObjectsFromPool();
    static std::vector<Entity> GetAllPickupsFromPool();
    
    // Helper to collect entities from a specific pool
    static void CollectEntitiesFromPool(const std::vector<Entity>& entityList, Vector3 position, float radius, std::vector<Entity>& entities);
};
