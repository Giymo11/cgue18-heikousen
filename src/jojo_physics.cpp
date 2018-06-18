//
// Created by benja on 4/28/2018.
//
#include "jojo_physics.hpp"
#include "jojo_level.hpp"

namespace Physics {

static void compactCollisionArray (
    btAlignedObjectArray<Collision> &collisions
) {
    const auto num      = collisions.size ();
    int        lastFree = -1;

    for (int i = 0; i < num; i++) {
        auto &coll = collisions[i];

        if (!coll.sticky) {
            coll.active = false;
            continue;
        }

        if (lastFree < 0)
            continue;

        auto &last = collisions[lastFree];
        last.active = true;
        last.sticky = true;
        last.obj    = coll.obj;
        coll.active = false;
        coll.sticky = false;
        coll.obj    = nullptr;
    }
}

static void collisionCallback (
    btDynamicsWorld                 *world,
    btScalar                      /* timeStep */
) {
    const auto dp = world->getDispatcher ();
    const auto numManifold = dp->getNumManifolds ();
    auto       physics = (Physics *)(world->getWorldUserInfo ());
    auto      &collisions = physics->collisionArray;
    int        nextFreeCollision = 0;
    compactCollisionArray (collisions);

    for (int i = 0; i < numManifold; ++i) {
        const auto manif = dp->getManifoldByIndexInternal (i);
        const auto body0 = btRigidBody::upcast (manif->getBody0 ());
        const auto body1 = btRigidBody::upcast (manif->getBody1 ());

        if (body0->getUserPointer () == nullptr)
            continue;
        if (body1->getUserPointer () == nullptr)
            continue;

        const auto inst0 = (Scene::Instance *)(body0->getUserPointer ());
        const auto inst1 = (Scene::Instance *)(body1->getUserPointer ());
        const auto type0 = inst0->type;
        const auto type1 = inst1->type;

        if (type0 != Scene::PlayerInstance && type1 != Scene::PlayerInstance)
            continue;

        // --------------------------------------------------------------
        // STORE COLLISION START
        // --------------------------------------------------------------

        const auto collNum = collisions.size ();
        Collision *coll = nullptr;

        for (; nextFreeCollision < collNum; nextFreeCollision++) {
            if (!collisions[nextFreeCollision].active) {
                coll = &collisions[nextFreeCollision];
                break;
            }
        }

        // No collision space left anymore
        if (coll == nullptr)
            break;

        coll->active = true;
        coll->obj = type0 != Scene::PlayerInstance ? inst0 : inst1;

        switch (coll->obj->type) {
        case Scene::PortalInstance:
        case Scene::LethalInstance:
            coll->sticky = true;
            break;
        case Scene::NonLethalInstance:
            break;
        case Scene::PlayerInstance:
        default:
            CHECK (false);
            break;
        }

        // --------------------------------------------------------------
        // STORE COLLISION END
        // --------------------------------------------------------------
    }
}

void alloc (
    Physics *physics
) {
    physics->collisionConfig      = new btDefaultCollisionConfiguration();
    physics->dispatcher           = new btCollisionDispatcher (physics->collisionConfig);
    physics->overlappingPairCache = new btDbvtBroadphase ();
    physics->solver               = new btSequentialImpulseConstraintSolver ();
    physics->solver->setRandSeed (0xDEADBEEF);
    physics->world = new btDiscreteDynamicsWorld (
        physics->dispatcher, physics->overlappingPairCache,
        physics->solver, physics->collisionConfig
    );

    physics->world->setInternalTickCallback (collisionCallback, (void *)physics);
    physics->world->setGravity (btVector3 (0, 0, 0));

    physics->collisionArray.clear ();
    physics->collisionArray.resize (64, { nullptr, false, false });
}

void free (
    Physics *physics
) {
    physics->collisionArray.clear ();
    delete physics->world;
    delete physics->solver;
    delete physics->overlappingPairCache;
    delete physics->dispatcher;
    delete physics->collisionConfig;
}

void addInstancesToWorld (
    Physics          *physics,
    Level::JojoLevel *level,
    Scene::Instance  *instances,
    size_t            numInstances
) {
    auto world = physics->world;

    Level::addRigidBodies (level, physics->world);
    for (auto i = 0; i < numInstances; i++)
        world->addRigidBody (instances[i].body);
}

void removeInstancesFromWorld (
    Physics          *physics,
    Level::JojoLevel *level,
    Scene::Instance  *instances,
    size_t            numInstances
) {
    const btVector3 zeroVector (0, 0, 0);
    auto            world = physics->world;

    for (auto i = 0; i < numInstances; i++) {
        auto body = instances[i].body;

        world->removeRigidBody (body);
        body->clearForces ();
        body->setLinearVelocity (zeroVector);
        body->setAngularVelocity (zeroVector);
    }

    Level::removeRigidBodies (level, world);
}

}