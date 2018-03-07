#include <btBulletDynamicsCommon.h>
#include <iostream>

int main () {
    std::cout << "Hello World!" << std::endl;

    // Build the broadphase
    auto broadphase = new btDbvtBroadphase();

    // Set up the collision configuration and dispatcher
    auto collisionConfiguration = new btDefaultCollisionConfiguration();
    auto dispatcher = new btCollisionDispatcher(collisionConfiguration);

    // The actual physics solver
    auto solver = new btSequentialImpulseConstraintSolver;

    // The world.
    auto dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));

    // Do_everything_else_here


    // Clean up behind ourselves like good little programmers
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;

    return 0;
}