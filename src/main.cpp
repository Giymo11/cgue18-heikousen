#include <iostream>
#include <vector>
#include <array>
#include <chrono>


#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING

#include <tiny_gltf.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <btBulletDynamicsCommon.h>

#include "jojo_utils.hpp"
// #include "jojo_script.hpp"

#include "jojo_vulkan_data.hpp"
#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_physics.hpp"
#include "jojo_engine.hpp"
#include "jojo_swapchain.hpp"
#include "jojo_window.hpp"
#include "jojo_pipeline.hpp"





void recordCommandBuffer(Config &config,
                         JojoPipeline *pipeline,
                         VkCommandBuffer commandBuffer,
                         VkFramebuffer framebuffer,
                         VkRenderPass renderPass,
                         JojoVulkanMesh *mesh) {

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = {config.width, config.height};
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    // _INLINE means to only use primary command buffers
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(config.width);
    viewport.height = static_cast<float>(config.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {config.width, config.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    mesh->goDrawYourself(commandBuffer, pipeline->pipelineLayout);

    vkCmdEndRenderPass(commandBuffer);
}


void drawFrame(Config &config,
               JojoEngine *engine,
               JojoWindow *window,
               JojoSwapchain *swapchain,
               JojoPipeline *pipeline,
               JojoVulkanMesh *mesh) {

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(engine->device,
                                            swapchain->swapchain,
                                            std::numeric_limits<uint64_t>::max(),
                                            swapchain->semaphoreImageAvailable,
                                            VK_NULL_HANDLE,
                                            &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchain->recreateSwapchain(config, engine, window);
        return;
        // throw away this frame, because after recreating the swapchain, the vkAcquireNexImageKHR is
        // not signaling the semaphoreImageAvailable anymore
    }
    ASSERT_VULKAN(result)


    VkPipelineStageFlags waitStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};


    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &swapchain->semaphoreImageAvailable;
    submitInfo.pWaitDstStageMask = waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(swapchain->commandBuffers[imageIndex]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &swapchain->semaphoreRenderingDone;

    result = vkWaitForFences(engine->device, 1, &swapchain->commandBufferFences[imageIndex], VK_TRUE, UINT64_MAX);
    ASSERT_VULKAN(result)
    result = vkResetFences(engine->device, 1, &swapchain->commandBufferFences[imageIndex]);
    ASSERT_VULKAN(result)


    result = beginCommandBuffer(swapchain->commandBuffers[imageIndex]);
    ASSERT_VULKAN(result)

    recordCommandBuffer(config,
                        pipeline,
                        swapchain->commandBuffers[imageIndex],
                        swapchain->framebuffers[imageIndex],
                        swapchain->swapchainRenderPass,
                        mesh);

    result = vkEndCommandBuffer(swapchain->commandBuffers[imageIndex]);
    ASSERT_VULKAN(result)

    result = vkQueueSubmit(engine->queue, 1, &submitInfo, swapchain->commandBufferFences[imageIndex]);
    ASSERT_VULKAN(result)

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &swapchain->semaphoreRenderingDone;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain->swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(engine->queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        swapchain->recreateSwapchain(config, engine, window);
        return;
        // same as with vkAcquireNextImageKHR
    }
    ASSERT_VULKAN(result)
}


auto lastFrameTime = std::chrono::high_resolution_clock::now();

void updateMvp(Config &config, JojoEngine *engine, JojoPhysics &physics, JojoVulkanMesh *mesh) {
    auto now = std::chrono::high_resolution_clock::now();
    float timeSinceLastFrame =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrameTime).count() / 1000.0f;
    lastFrameTime = now;

    std::cout << "frame time " << timeSinceLastFrame << std::endl;

    glm::mat4 view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -5.0f));
    //view[1][1] *= -1;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), config.width / (float) config.height, 0.001f, 100.0f);
    // openGL has the z dir flipped
    projection[1][1] *= -1;


    physics.dynamicsWorld->stepSimulation(timeSinceLastFrame);

    // TODO: have a mapping from physics to node
    JojoNode &node = mesh->scene->children[0];

    // TODO: maybe move this over to the physics class as well
    btCollisionObject *obj = physics.dynamicsWorld->getCollisionObjectArray()[0];
    btRigidBody *body = btRigidBody::upcast(obj);

    btTransform trans;
    if (body && body->getMotionState()) {
        body->getMotionState()->getWorldTransform(trans);
    } else {
        trans = obj->getWorldTransform();
    }
    btVector3 origin = trans.getOrigin();

    auto &manifoldPoints = physics.objectsCollisions[body];

    glm::mat4 matrix;
    trans.getOpenGLMatrix(glm::value_ptr(matrix));
    node.setRelativeMatrix(matrix);

    //int direction = (i % 2 * 2 - 1);
    //mesh.modelMatrix = glm::rotate(mesh.modelMatrix, timeSinceLastFrame * glm::radians(30.0f) * direction, glm::vec3(0, 0, 1));

    glm::mat4 proj_view = projection * view;

    mesh->updateAlignedUniforms(proj_view);
    auto bufferSize = mesh->dynamicAlignment * mesh->scene->mvps.size();

    // TODO: copy the model matrices to GPU memory via some kind of flushing / not via coherent memory
    void *rawData;
    VkResult result = vkMapMemory(engine->device,
                                  mesh->uniformBufferDeviceMemory,
                                  0,
                                  bufferSize,
                                  0,
                                  &rawData);
    ASSERT_VULKAN(result)
    memcpy(rawData, mesh->alignedMvps, bufferSize);
    vkUnmapMemory(engine->device, mesh->uniformBufferDeviceMemory);

}


void gameloop(Config &config,
              JojoEngine *engine,
              JojoWindow *jojoWindow,
              JojoSwapchain *swapchain,
              JojoPipeline *pipeline,
              JojoPhysics &physics,
              JojoVulkanMesh *mesh) {
    // TODO: extract a bunch of this to JojoWindow

    auto window = jojoWindow->window;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        btVector3 relativeForce(0, 0, 0);

        int state = glfwGetKey(window, GLFW_KEY_W);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 0, -1);
        }
        state = glfwGetKey(window, GLFW_KEY_S);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 0, 1);
        }
        state = glfwGetKey(window, GLFW_KEY_A);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(-1, 0, 0);
        }
        state = glfwGetKey(window, GLFW_KEY_D);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(1, 0, 0);
        }
        state = glfwGetKey(window, GLFW_KEY_R);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 1, 0);
        }
        state = glfwGetKey(window, GLFW_KEY_F);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, -1, 0);
        }

        state = glfwGetKey(window, GLFW_KEY_X);
        bool xPressed = state == GLFW_PRESS;

        state = glfwGetKey(window, GLFW_KEY_Z);
        bool yPressed = state == GLFW_PRESS;


        btVector3 relativeTorque(0, 0, 0);

        state = glfwGetKey(window, GLFW_KEY_Q);
        if (state == GLFW_PRESS) {
            relativeTorque = relativeTorque + btVector3(0, 1, 0);
        }
        state = glfwGetKey(window, GLFW_KEY_E);
        if (state == GLFW_PRESS) {
            relativeTorque = relativeTorque + btVector3(0, -1, 0);
        }

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        double relXpos = xpos - config.width / 2.0f;
        double relYpos = ypos - config.height / 2.0f;

        double minX = config.width * config.deadzoneScreenPercentage / 100.0f;
        double minY = config.height * config.deadzoneScreenPercentage / 100.0f;

        double maxX = config.width * config.navigationScreenPercentage / 100.0f;
        double maxY = config.height * config.navigationScreenPercentage / 100.0f;

        double absXpos = glm::abs(relXpos);
        double absYpos = glm::abs(relYpos);

        if (absXpos > minX) {
            if (absXpos > maxX) {
                absXpos = maxX;
            }
            double torque = (absXpos - minX) / (maxX - minX) * glm::sign(relXpos) * -1;
            relativeTorque = relativeTorque + btVector3(0, 0, static_cast<float>(torque));
        }

        if (absYpos > minY) {
            if (absYpos > maxY) {
                absYpos = maxY;
            }
            double torque = (absYpos - minY) / (maxY - minY) * glm::sign(relYpos) * -1;
            relativeTorque = relativeTorque + btVector3(static_cast<float>(torque), 0, 0);
        }

        double newXpos = absXpos * glm::sign(relXpos) + config.width / 2.0f;
        double newYpos = absYpos * glm::sign(relYpos) + config.height / 2.0f;
        glfwSetCursorPos(window, newXpos, newYpos);
        // TODO: fix dis


        if (!relativeForce.isZero() || !relativeTorque.isZero() || xPressed || yPressed) {
            btCollisionObject *obj = physics.dynamicsWorld->getCollisionObjectArray()[0];
            btRigidBody *body = btRigidBody::upcast(obj);

            btTransform trans;
            if (body && body->getMotionState()) {
                body->getMotionState()->getWorldTransform(trans);

                btMatrix3x3 &boxRot = trans.getBasis();

                if (xPressed) {
                    if (body->getLinearVelocity().norm() < 0.01) {
                        // stop the jiggling around
                        body->setLinearVelocity(btVector3(0, 0, 0));
                    } else {
                        // counteract the current inertia
                        // TODO: think about maybe making halting easier than accelerating.
                        btVector3 correctedForce = (body->getLinearVelocity() * -1).normalized();
                        body->applyCentralForce(correctedForce);
                    }
                }
                if (yPressed) {
                    if (body->getAngularVelocity().norm() < 0.01) {
                        body->setAngularVelocity(btVector3(0, 0, 0));
                    } else {
                        btVector3 correctedTorque = (body->getAngularVelocity() * -1).normalized();
                        body->applyTorque(correctedTorque);
                    }
                }

                if (!xPressed) {
                    if (!relativeForce.isZero()) {
                        // TODO: decide about maybe normalizing
                        btVector3 correctedForce = boxRot * relativeForce;
                        body->applyCentralForce(correctedForce);
                    }
                }
                if (!yPressed) {
                    if (!relativeTorque.isZero()) {
                        btVector3 correctedTorque = boxRot * relativeTorque;
                        body->applyTorque(correctedTorque);
                    }
                }

            } else {
                // for later use
                trans = obj->getWorldTransform();
            }
        }

        updateMvp(config, engine, physics, mesh);

        drawFrame(config, engine, jojoWindow, swapchain, pipeline, mesh);
    }
}







void loadFromGlb(tinygltf::Model *modelDst, std::string relPath) {
    tinygltf::TinyGLTF loader;
    std::string err;

    bool loaded = loader.LoadBinaryFromFile(modelDst, &err, relPath);
    if (!err.empty()) {
        std::cout << "Err: " << err << std::endl;
    }
    if (!loaded) {
        std::cout << "Failed to parse glTF: " << std::endl;
        psnip_trap();
    }
}


int main(int argc, char *argv[]) {
    //Scripting::Engine jojoScript;

    //Scripting::GameObject helloObj;
    //jojoScript.hookScript(helloObj, "../scripts/hello.js");
    //helloObj.updateLogic();



    JojoPhysics physics;


    tinygltf::Model gltfModel;
    loadFromGlb(&gltfModel, "../models/duck.glb");

    JojoScene scene;
/*
    JojoNode duck;
    duck.loadFromGltf(gltfModel, &scene);
    scene.children.push_back(duck);
*/

    loadFromGlb(&gltfModel, "../models/icosphere.glb");
    JojoNode icosphere;
    icosphere.loadFromGltf(gltfModel, &scene);
    icosphere.setRelativeMatrix(glm::mat4());
    scene.children.push_back(icosphere);


    auto collisionShapeIndex = physics.collisionShapes.size();
    auto collisionObjectArrayIndex = physics.dynamicsWorld->getCollisionObjectArray().size();

    std::cout << "shapeIndex " << collisionShapeIndex << ", objectIndex " << collisionObjectArrayIndex << std::endl;

    btCollisionShape *colShape = new btSphereShape(1);
    physics.collisionShapes.push_back(colShape);

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(0, 0, 0));

    btVector3 localInertia(0, 1, 0);
    btScalar mass(1.0f);
    colShape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);
    auto rigidBodyConstuctionInfo = btRigidBody::btRigidBodyConstructionInfo(mass, myMotionState, colShape, localInertia);
    btRigidBody *body = new btRigidBody(rigidBodyConstuctionInfo);
    body->setRestitution(objectRestitution);
    body->forceActivationState(DISABLE_DEACTIVATION);

    physics.dynamicsWorld->addRigidBody(body);



    auto translation = glm::vec3(1, 1, 1);
    auto scale = glm::vec3(1, 1, 1);

    JojoNode icosphere2;
    icosphere2.loadFromGltf(gltfModel, &scene);
    icosphere2.setRelativeMatrix(glm::translate(glm::scale(icosphere.getRelativeMatrix(), scale), translation));
    scene.children.push_back(icosphere2);





    Config config = Config::readFromFile("../config.ini");

    JojoWindow window;
    window.startGlfw(config);

    JojoEngine engine;
    engine.jojoWindow = &window;
    engine.startVulkan();
    engine.initialieDescriptorPool(0, 1, 0);


    JojoVulkanMesh mesh;
    mesh.scene = &scene;


    JojoSwapchain swapchain;
    swapchain.createCommandBuffers(&engine);
    swapchain.createSwapchainAndChildren(config, &engine);


    JojoPipeline pipeline;
    pipeline.initializeDescriptorSetLayout(&engine);


    mesh.initializeBuffers(&engine, &pipeline);


    pipeline.createPipelineHelper(config, &engine, swapchain.swapchainRenderPass);

    gameloop(config, &engine, &window, &swapchain, &pipeline, physics, &mesh);


    VkResult result = vkDeviceWaitIdle(engine.device);
    ASSERT_VULKAN(result)


    pipeline.destroyDescriptorSetLayout(&engine);
    engine.destroyDescriptorPool();

    mesh.destroyBuffers(&engine);

    swapchain.destroyCommandBuffers(&engine);

    pipeline.destroyPipeline(&engine);

    swapchain.destroySwapchainChildren(&engine);
    vkDestroySwapchainKHR(engine.device, swapchain.swapchain, nullptr);

    engine.shutdownVulkan();

    window.shutdownGlfw();

    return 0;
}
