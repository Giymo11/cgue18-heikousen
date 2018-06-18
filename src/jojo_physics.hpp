//
// Created by benja on 3/26/2018.
//

#pragma once
#include <vector>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include "jojo_level.hpp"
#include "jojo_scene.hpp"

namespace Level {
struct JojoLevel;
}

namespace Physics {

const float objectRestitution = 0.99f;

struct Collision {
    Scene::Instance *obj;

    bool active;
    bool sticky;
};

struct Physics {
    btDefaultCollisionConfiguration      *collisionConfig;
    btCollisionDispatcher                *dispatcher;
    btBroadphaseInterface                *overlappingPairCache;
    btSequentialImpulseConstraintSolver  *solver;
    btDiscreteDynamicsWorld              *world;

    btAlignedObjectArray<Collision>       collisionArray;
};

void alloc (
    Physics *physics
);

void free (
    Physics *physics
);

void addInstancesToWorld (
    Physics          *physics,
    Level::JojoLevel *level,
    Scene::Instance  *instances,
    size_t            numInstances
);

void removeInstancesFromWorld (
    Physics          *physics,
    Level::JojoLevel *level,
    Scene::Instance  *instances,
    size_t            numInstances
);

}

