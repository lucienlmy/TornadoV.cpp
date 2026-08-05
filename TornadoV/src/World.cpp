#include "World.h"
#include "main.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_set>

std::vector<Entity> World::GetNearbyEntities(Vector3 position, float radius) {
    std::vector<Entity> nearbyEntities;
    nearbyEntities.reserve(512);

    std::vector<Entity> allPeds = GetAllPedsFromPool();
    std::vector<Entity> allVehicles = GetAllVehiclesFromPool();
    std::vector<Entity> allObjects = GetAllObjectsFromPool();
    std::vector<Entity> allPickups = GetAllPickupsFromPool();

    CollectEntitiesFromPool(allPeds, position, radius, nearbyEntities);
    CollectEntitiesFromPool(allVehicles, position, radius, nearbyEntities);
    CollectEntitiesFromPool(allObjects, position, radius, nearbyEntities);
    CollectEntitiesFromPool(allPickups, position, radius, nearbyEntities);

    return nearbyEntities;
}

std::vector<Entity> World::GetAllPedsFromPool() {
    const int POOL_SIZE = 1024;
    int peds[POOL_SIZE];
    int count = worldGetAllPeds(peds, POOL_SIZE);

    std::vector<Entity> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        if (ENTITY::DOES_ENTITY_EXIST(peds[i]))
            result.push_back(peds[i]);
    }
    return result;
}

std::vector<Entity> World::GetAllVehiclesFromPool() {
    const int POOL_SIZE = 1024;
    int vehicles[POOL_SIZE];
    int count = worldGetAllVehicles(vehicles, POOL_SIZE);

    std::vector<Entity> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        if (ENTITY::DOES_ENTITY_EXIST(vehicles[i]))
            result.push_back(vehicles[i]);
    }
    return result;
}

std::vector<Entity> World::GetAllObjectsFromPool() {
    const int POOL_SIZE = 1024;
    int objects[POOL_SIZE];
    int count = worldGetAllObjects(objects, POOL_SIZE);

    std::vector<Entity> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        if (ENTITY::DOES_ENTITY_EXIST(objects[i]))
            result.push_back(objects[i]);
    }
    return result;
}

std::vector<Entity> World::GetAllPickupsFromPool() {
    const int POOL_SIZE = 512;
    int pickups[POOL_SIZE];
    int count = worldGetAllPickups(pickups, POOL_SIZE);

    std::vector<Entity> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        if (ENTITY::DOES_ENTITY_EXIST(pickups[i]))
            result.push_back(pickups[i]);
    }
    return result;
}

void World::CollectEntitiesFromPool(const std::vector<Entity>& entityList, Vector3 position, float radius, std::vector<Entity>& entities) {
    float radiusSquared = radius * radius;

    for (Entity entity : entityList) {
        Vector3 entityPos = ENTITY::GET_ENTITY_COORDS(entity, true);

        float dx = entityPos.x - position.x;
        float dy = entityPos.y - position.y;

        if ((dx * dx) + (dy * dy) <= radiusSquared)
            entities.push_back(entity);
    }
}