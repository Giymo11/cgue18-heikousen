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
#include "jojo_replay.hpp"

void bindBufferToDescriptorSet(VkDevice device,
                               VkBuffer uniformBuffer,
                               VkDescriptorSet descriptorSet) {

    VkDescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo.buffer = uniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(glm::mat4);

    VkWriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.pNext = nullptr;
    writeDescriptorSet.dstSet = descriptorSet;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pImageInfo = nullptr;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
    writeDescriptorSet.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}


void recordCommandBuffer(Config &config,
                         JojoPipeline *pipeline,
                         VkCommandBuffer commandBuffer,
                         VkFramebuffer framebuffer,
                         VkRenderPass renderPass,
                         std::vector<JojoVulkanMesh> &meshes) {

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

    VkDeviceSize offsets[] = {0};

    for (JojoVulkanMesh &mesh : meshes) {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(mesh.vertexBuffer), offsets);
        vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline->pipelineLayout,
                                0,
                                1,
                                &(mesh.uniformDescriptorSet),
                                0,
                                nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
}


void drawFrame(Config &config,
               JojoEngine *engine,
               JojoWindow *window,
               JojoSwapchain *swapchain,
               JojoPipeline *pipeline,
               std::vector<JojoVulkanMesh> &meshes) {

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
                        meshes);

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

void updateMvp(Config &config, JojoEngine *engine, JojoPhysics &physics, std::vector<JojoVulkanMesh> &meshes) {
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


    void *rawData;
    for (int i = 0; i < meshes.size(); ++i) {
        JojoVulkanMesh &mesh = meshes[i];

        // TODO: maybe move this over to the physics class as well
        btCollisionObject *obj = physics.dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody *body = btRigidBody::upcast(obj);

        btTransform trans;
        if (body && body->getMotionState()) {
            body->getMotionState()->getWorldTransform(trans);
        } else {
            trans = obj->getWorldTransform();
        }
        btVector3 origin = trans.getOrigin();

        auto &manifoldPoints = physics.objectsCollisions[body];

        trans.getOpenGLMatrix(glm::value_ptr(mesh.modelMatrix));

        //int direction = (i % 2 * 2 - 1);
        //mesh.modelMatrix = glm::rotate(mesh.modelMatrix, timeSinceLastFrame * glm::radians(30.0f) * direction, glm::vec3(0, 0, 1));

        glm::mat4 mvp = projection * view * mesh.modelMatrix;
        VkResult result = vkMapMemory(engine->device, mesh.uniformBufferDeviceMemory, 0, sizeof(mvp), 0, &rawData);
        ASSERT_VULKAN(result)
        memcpy(rawData, &mvp, sizeof(mvp));
        vkUnmapMemory(engine->device, mesh.uniformBufferDeviceMemory);
    }


}


void gameloop(Config &config,
              JojoEngine *engine,
              JojoWindow *jojoWindow,
              JojoSwapchain *swapchain,
              JojoPipeline *pipeline,
              Replay::Recorder *jojoReplay,
              JojoPhysics &physics,
              std::vector<JojoVulkanMesh> &meshes) {
    // TODO: extract a bunch of this to JojoWindow
   
    auto window = jojoWindow->window;
    jojoReplay->startRecording ();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (jojoReplay->state() != Replay::RecorderState::Replaying) {
            auto state = jojoReplay->getKey (GLFW_KEY_SPACE);
            if (state == GLFW_PRESS)
                jojoReplay->startReplay ();
        }

        btVector3 relativeForce(0, 0, 0);

        int state = jojoReplay->getKey (GLFW_KEY_W);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 0, -1);
        }
        state = jojoReplay->getKey (GLFW_KEY_S);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 0, 1);
        }
        state = jojoReplay->getKey (GLFW_KEY_A);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(-1, 0, 0);
        }
        state = jojoReplay->getKey (GLFW_KEY_D);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(1, 0, 0);
        }
        state = jojoReplay->getKey (GLFW_KEY_R);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 1, 0);
        }
        state = jojoReplay->getKey (GLFW_KEY_F);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, -1, 0);
        }
       
        state = jojoReplay->getKey (GLFW_KEY_X);
        bool xPressed = state == GLFW_PRESS;

        state = jojoReplay->getKey (GLFW_KEY_Z);
        bool yPressed = state == GLFW_PRESS;


        btVector3 relativeTorque(0, 0, 0);

        state = jojoReplay->getKey (GLFW_KEY_Q);
        if (state == GLFW_PRESS) {
            relativeTorque = relativeTorque + btVector3(0, 1, 0);
        }
        state = jojoReplay->getKey (GLFW_KEY_E);
        if (state == GLFW_PRESS) {
            relativeTorque = relativeTorque + btVector3(0, -1, 0);
        }

        double xpos, ypos;
        jojoReplay->getCursorPos (&xpos, &ypos);

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

        if (jojoReplay->nextTickReady()) {
            jojoReplay->nextTick ();

            if (!relativeForce.isZero () || !relativeTorque.isZero () || xPressed || yPressed) {
                btCollisionObject *obj = physics.dynamicsWorld->getCollisionObjectArray ()[0];
                btRigidBody *body = btRigidBody::upcast (obj);

                btTransform trans;
                if (body && body->getMotionState ()) {
                    body->getMotionState ()->getWorldTransform (trans);

                    btMatrix3x3 &boxRot = trans.getBasis ();

                    if (xPressed) {
                        if (body->getLinearVelocity ().norm () < 0.01) {
                            // stop the jiggling around
                            body->setLinearVelocity (btVector3 (0, 0, 0));
                        } else {
                            // counteract the current inertia
                            // TODO: think about maybe making halting easier than accelerating.
                            btVector3 correctedForce = (body->getLinearVelocity () * -1).normalized ();
                            body->applyCentralForce (correctedForce);
                        }
                    }
                    if (yPressed) {
                        if (body->getAngularVelocity ().norm () < 0.01) {
                            body->setAngularVelocity (btVector3 (0, 0, 0));
                        } else {
                            btVector3 correctedTorque = (body->getAngularVelocity () * -1).normalized ();
                            body->applyTorque (correctedTorque);
                        }
                    }

                    if (!xPressed) {
                        if (!relativeForce.isZero ()) {
                            // TODO: decide about maybe normalizing
                            btVector3 correctedForce = boxRot * relativeForce;
                            body->applyCentralForce (correctedForce);
                        }
                    }
                    if (!yPressed) {
                        if (!relativeTorque.isZero ()) {
                            btVector3 correctedTorque = boxRot * relativeTorque;
                            body->applyTorque (correctedTorque);
                        }
                    }

                } else {
                    // for later use
                    trans = obj->getWorldTransform ();
                }
            }

            updateMvp (config, engine, physics, meshes);
        }

        drawFrame(config, engine, jojoWindow, swapchain, pipeline, meshes);
    }
}


void destroyMeshStuff(JojoEngine *engine, std::vector<JojoVulkanMesh> &meshes) {

    for (JojoVulkanMesh &mesh : meshes) {
        vkFreeMemory(engine->device, mesh.uniformBufferDeviceMemory, nullptr);
        vkDestroyBuffer(engine->device, mesh.uniformBuffer, nullptr);

        vkFreeMemory(engine->device, mesh.indexBufferDeviceMemory, nullptr);
        vkDestroyBuffer(engine->device, mesh.indexBuffer, nullptr);

        vkFreeMemory(engine->device, mesh.vertexBufferDeviceMemory, nullptr);
        vkDestroyBuffer(engine->device, mesh.vertexBuffer, nullptr);
    }
}


void initializeBuffers(JojoEngine *engine, JojoPipeline *pipeline, std::vector<JojoVulkanMesh> &meshes) {

    for (JojoVulkanMesh &mesh : meshes) {
        createAndUploadBuffer(engine->device, engine->chosenDevice, engine->commandPool, engine->queue, mesh.vertices,
                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              &(mesh.vertexBuffer), &(mesh.vertexBufferDeviceMemory));
        createAndUploadBuffer(engine->device, engine->chosenDevice, engine->commandPool, engine->queue, mesh.indices,
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              &(mesh.indexBuffer), &(mesh.indexBufferDeviceMemory));

        VkDeviceSize bufferSize = sizeof(glm::mat4);
        createBuffer(engine->device, engine->chosenDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     &(mesh.uniformBuffer),
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &(mesh.uniformBufferDeviceMemory));


        VkResult result = allocateDescriptorSet(engine->device, engine->descriptorPool, pipeline->descriptorSetLayout,
                                                &(mesh.uniformDescriptorSet));
        ASSERT_VULKAN(result)
        bindBufferToDescriptorSet(engine->device, mesh.uniformBuffer, mesh.uniformDescriptorSet);
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

    Config config = Config::readFromFile("../config.ini");

    JojoWindow window;
    window.startGlfw(config);


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
    scene.children.push_back(icosphere);


    // bullet part
    btCollisionShape *colShape = new btBoxShape(btVector3(1, 1, 1));

    physics.collisionShapes.push_back(colShape);

    btTransform startTransform;
    startTransform.setIdentity();


    startTransform.setOrigin(btVector3(0, 0, 0));
    //meshes->modelMatrix = translate(glm::mat4(), glm::vec3(x, y, z));

    btVector3 localInertia(0, 1, 0);
    btScalar mass(1.0f);

    colShape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);

    btRigidBody *body = new btRigidBody(
            btRigidBody::btRigidBodyConstructionInfo(mass, myMotionState, colShape, localInertia));
    body->setRestitution(objectRestitution);
    body->forceActivationState(DISABLE_DEACTIVATION);


    physics.dynamicsWorld->addRigidBody(body);


    float width = 0.5f, height = 0.5f, depth = 0.5f;
    auto offset = -2.0f;

    /*
     *
    int cubesAmount = 3;

    std::vector<JojoVulkanMesh> meshes;
    meshes.resize(cubesAmount + 1);

     for (int i = 0; i < meshes.size(); ++i) {
        createCube(&meshes[i], physics, width, height, depth, 0.0f, offset * i, 0.0f);
    }*/

    std::vector<JojoVulkanMesh> meshes;
    meshes.resize(1);


    meshes[0].vertices = scene.vertexBuffer;
    meshes[0].indices = scene.indexBuffer;
    meshes[0].modelMatrix = glm::mat4();

    Replay::Recorder jojoReplay(window.window);
    jojoReplay.setResetFunc ([body, &startTransform]() {
        // TODO: Reset state of the world here
        btVector3 zeroVector (0, 0, 0);

        body->clearForces ();
        body->setLinearVelocity (zeroVector);
        body->setAngularVelocity (zeroVector);
        body->setWorldTransform (startTransform);
    });


    JojoEngine engine;
    engine.jojoWindow = &window;
    engine.startVulkan();
    engine.initialieDescriptorPool(meshes.size());


    JojoSwapchain swapchain;
    swapchain.createCommandBuffers(&engine);
    swapchain.createSwapchainAndChildren(config, &engine);


    JojoPipeline pipeline;
    pipeline.initializeDescriptorSetLayout(&engine);

    initializeBuffers(&engine, &pipeline, meshes);

    pipeline.createPipelineHelper(config, &engine, swapchain.swapchainRenderPass);

    gameloop(config, &engine, &window, &swapchain, &pipeline, &jojoReplay, physics, meshes);


    VkResult result = vkDeviceWaitIdle(engine.device);
    ASSERT_VULKAN(result)


    pipeline.destroyDescriptorSetLayout(&engine);
    engine.destroyDescriptorPool();

    destroyMeshStuff(&engine, meshes);

    swapchain.destroyCommandBuffers(&engine);

    pipeline.destroyPipeline(&engine);

    swapchain.destroySwapchainChildren(&engine);
    vkDestroySwapchainKHR(engine.device, swapchain.swapchain, nullptr);

    engine.shutdownVulkan();

    window.shutdownGlfw();

    return 0;
}
