//
// Created by benja on 4/28/2018.
//

#include "jojo_physics.hpp"


void myTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    JojoPhysics *w = static_cast<JojoPhysics *>(world->getWorldUserInfo());
    w->theTickCallback(world, timeStep);
}

JojoPhysics::JojoPhysics() {
    dynamicsWorld->setInternalTickCallback(myTickCallback, static_cast<void *>(this));
    dynamicsWorld->setGravity(btVector3(0, 0, 0));

}

JojoPhysics::~JojoPhysics() {
    // Cleanup.
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
        btCollisionObject *obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody *body = btRigidBody::upcast(obj);
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }

    for (int i = 0; i < collisionShapes.size(); ++i) {
        delete collisionShapes[i];
    }
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
    collisionShapes.clear();
}

void JojoPhysics::theTickCallback(btDynamicsWorld *dynamicsWorld, btScalar timeStep) {
    objectsCollisions.clear();

    int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold *contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);

        auto *objA = contactManifold->getBody0();
        auto *objB = contactManifold->getBody1();

        auto &collisionsA = objectsCollisions[objA];
        auto &collisionsB = objectsCollisions[objB];

        int numContacts = contactManifold->getNumContacts();
        for (int j = 0; j < numContacts; j++) {
            btManifoldPoint &pt = contactManifold->getContactPoint(j);

            collisionsA.push_back(&pt);
            collisionsB.push_back(&pt);
        }
    }
}

