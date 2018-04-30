//
// Created by benja on 3/26/2018.
//

#pragma once

#include <vector>
#include <map>

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include "jojo_physics_node.hpp"

#define PRINTF_FLOAT "%7.3f"

constexpr float objectRestitution = 0.99f;

#define PRINTF_FLOAT "%7.3f"

static void myTickCallback(btDynamicsWorld *world, btScalar timeStep);

class JojoPhysics {
public:

    btDefaultCollisionConfiguration *collisionConfiguration = new btDefaultCollisionConfiguration();

    btCollisionDispatcher *dispatcher = new btCollisionDispatcher(collisionConfiguration);

    btBroadphaseInterface *overlappingPairCache = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver *solver = new btSequentialImpulseConstraintSolver;

    btDiscreteDynamicsWorld *dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,
                                                                         overlappingPairCache,
                                                                         solver,
                                                                         collisionConfiguration);

    btAlignedObjectArray<btCollisionShape *> collisionShapes;

    std::map<const btCollisionObject *, std::vector<btManifoldPoint *>> objectsCollisions;



    JojoPhysicsNode *player;
    JojoPhysicsNode *winner;
    JojoPhysicsNode *loser;

    std::vector<JojoPhysicsNode*> dynamicNodes;

    void theTickCallback(btDynamicsWorld *dynamicsWorld, btScalar timeStep);

    JojoPhysics();

    ~JojoPhysics();

};


