#include <iostream>
#include <vector>
#include <array>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>


#include <btBulletDynamicsCommon.h>

#include "jojo_utils.hpp"
#include "jojo_vulkan_data.hpp"
#include "jojo_vulkan.hpp"
#include "jojo_vulkan_utils.hpp"
#include "jojo_physics.hpp"
#include "jojo_swapchain.hpp"
#include "jojo_window.hpp"
#include "jojo_replay.hpp"
#include "Rendering/DescriptorSets.h"
#include "jojo_vulkan_textures.hpp"
#include "jojo_level.hpp"


void recordCommandBuffer (
    Config                      &config,
    JojoEngine                  *engine,
    const VkCommandBuffer        cmd,
    const VkFramebuffer          framebuffer,
    const VkRenderPass           renderPass,
    JojoVulkanMesh              *mesh,
    const JojoPipeline          *pipeline,
    const JojoPipeline          *textPipeline,
    const JojoPipeline          *levelPipeline,
    const Scene::SceneTemplates *scene,
    const Level::JojoLevel      *level
) {

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
    vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(config.width);
    viewport.height = static_cast<float>(config.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {config.width, config.height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // --------------------------------------------------------------
    // LEVEL DRAWING BEGIN
    // --------------------------------------------------------------

    {
        auto descriptor = engine->descriptors->set (Rendering::Set::Level);

        vkCmdBindPipeline (
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            levelPipeline->pipeline
        );
        vkCmdBindDescriptorSets (
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, levelPipeline->pipelineLayout,
            0, 1, &descriptor, 0, nullptr
        );

        VkBuffer vertexBuffers[] = { level->vertex };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers (cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer (cmd, level->index, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed (cmd, level->bsp->indexCount, 1, 0, 0, 0);
    }
    
    // --------------------------------------------------------------
    // LEVEL DRAWING END
    // --------------------------------------------------------------

    vkCmdBindPipeline (
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->pipeline
    );
    Scene::cmdDrawInstances (
        cmd, pipeline->pipelineLayout, mesh,
        scene->templates.data(), scene->instances.data(),
        scene->numInstances
    );

    vkCmdBindPipeline (
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        textPipeline->pipeline
    );
    auto textDescriptor = engine->descriptors->set (Rendering::Set::Text);
    vkCmdBindDescriptorSets (
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        textPipeline->pipelineLayout,
        0,
        1, &textDescriptor,
        0, nullptr
    );
    vkCmdDraw (cmd, 6, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
}


void drawFrame (
    Config                      &config,
    JojoEngine                  *engine,
    JojoWindow                  *window,
    JojoSwapchain               *swapchain,
    JojoVulkanMesh              *mesh,
    const JojoPipeline          *pipeline,
    const JojoPipeline          *textPipeline,
    const JojoPipeline          *levelPipeline,
    const Scene::SceneTemplates *scene,
    const Level::JojoLevel      *level
) {
    const auto allocator     = engine->allocator;
    const auto device        = engine->device;
    const auto drawQueue     = engine->queue;
    const auto transferQueue = engine->queue;

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device, swapchain->swapchain, std::numeric_limits<uint64_t>::max(),
        swapchain->semaphoreImageAvailable, VK_NULL_HANDLE, &imageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchain->recreateSwapchain(config, engine, window);
        return;
        // throw away this frame, because after recreating the swapchain, the vkAcquireNexImageKHR is
        // not signaling the semaphoreImageAvailable anymore
    }
    ASSERT_VULKAN (result);

    const auto drawCmd = swapchain->commandBuffers[imageIndex];
    const auto transferCmd = drawCmd;
    const auto fence = swapchain->commandBufferFences[imageIndex];

    // --------------------------------------------------------------
    // DATA STAGING BEGIN
    // --------------------------------------------------------------

    {
        // Wait for previous drawing to finish
        result = VK_TIMEOUT;
        while (result == VK_TIMEOUT)
            result = vkWaitForFences (device, 1, &fence, VK_TRUE, 100000000);
        ASSERT_VULKAN (result);
        result = vkResetFences (device, 1, &fence);
        ASSERT_VULKAN (result);
        
        result = beginCommandBuffer (transferCmd);
        ASSERT_VULKAN (result);

        // Perform data staging
        Level::cmdBuildAndStageIndicesNaively (allocator, device, level, transferCmd);

        {
            VkMemoryPropertyFlags memFlags   = {};
            VkBufferCopy          bufferCopy = {};
            VmaAllocationInfo     allocInfo;

            allocInfo = mesh->alli_modelTrans;
            vmaGetMemoryTypeProperties (
                allocator, allocInfo.memoryType,
                &memFlags
            );
            if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
                VkMappedMemoryRange memRange = {
                    VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
                };
                memRange.memory = allocInfo.deviceMemory;
                memRange.offset = allocInfo.offset;
                memRange.size   = allocInfo.size;
                vkFlushMappedMemoryRanges (device, 1, &memRange);
            }
            bufferCopy.size = allocInfo.size;
            vkCmdCopyBuffer (
                transferCmd, mesh->stage_modelTrans,
                mesh->modelTrans, 1, &bufferCopy
            );

            allocInfo = mesh->alli_globalTrans;
            vmaGetMemoryTypeProperties (
                allocator, allocInfo.memoryType,
                &memFlags
            );
            if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
                VkMappedMemoryRange memRange = {
                    VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
                };
                memRange.memory = allocInfo.deviceMemory;
                memRange.offset = allocInfo.offset;
                memRange.size = allocInfo.size;
                vkFlushMappedMemoryRanges (device, 1, &memRange);
            }
            bufferCopy.size = allocInfo.size;
            vkCmdCopyBuffer (
                transferCmd, mesh->stage_globalTrans,
                mesh->globalTrans, 1, &bufferCopy
            );

            allocInfo = mesh->alli_lightInfo;
            vmaGetMemoryTypeProperties (
                allocator, allocInfo.memoryType,
                &memFlags
            );
            if ((memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
                VkMappedMemoryRange memRange = {
                    VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
                };
                memRange.memory = allocInfo.deviceMemory;
                memRange.offset = allocInfo.offset;
                memRange.size = allocInfo.size;
                vkFlushMappedMemoryRanges (device, 1, &memRange);
            }
            bufferCopy.size = allocInfo.size;
            vkCmdCopyBuffer (
                transferCmd, mesh->stage_lightInfo,
                mesh->lightInfo, 1, &bufferCopy
            );
        }

        result = vkEndCommandBuffer (transferCmd);
        ASSERT_VULKAN (result);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &transferCmd;
        vkQueueSubmit (transferQueue, 1, &submitInfo, fence);
     }

    // --------------------------------------------------------------
    // DATA STAGING END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // DRAWING BEGIN
    // --------------------------------------------------------------

    {
        // Wait for data staging to be finished
        result = VK_TIMEOUT;
        while (result == VK_TIMEOUT)
            result = vkWaitForFences (device, 1, &fence, VK_TRUE, 100000000);
        ASSERT_VULKAN (result);
        result = vkResetFences (device, 1, &fence);
        ASSERT_VULKAN (result);

        result = beginCommandBuffer (drawCmd);
        ASSERT_VULKAN (result);

        recordCommandBuffer (
            config, engine,
            swapchain->commandBuffers[imageIndex],
            swapchain->framebuffers[imageIndex],
            swapchain->swapchainRenderPass,
            mesh,pipeline, textPipeline,
            levelPipeline, scene, level
        );

        result = vkEndCommandBuffer (drawCmd);
        ASSERT_VULKAN (result);

        VkPipelineStageFlags waitStageMask[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &swapchain->semaphoreImageAvailable;
        submitInfo.pWaitDstStageMask = waitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &drawCmd;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &swapchain->semaphoreRenderingDone;

        result = vkQueueSubmit (drawQueue, 1, &submitInfo, fence);
        ASSERT_VULKAN (result);

        VkPresentInfoKHR presentInfo;
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &swapchain->semaphoreRenderingDone;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain->swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR (drawQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            swapchain->recreateSwapchain (config, engine, window);
            return;
            // same as with vkAcquireNextImageKHR
        }
        ASSERT_VULKAN (result);
    }

    // --------------------------------------------------------------
    // DRAWING END
    // --------------------------------------------------------------
}


auto lastFrameTime = std::chrono::high_resolution_clock::now();


static void updateMvp (
    Config                      &config,
    JojoEngine                  *engine,
    Physics::Physics            *physics,
    JojoVulkanMesh              *mesh,
    const Scene::SceneTemplates *scene,
    const Level::JojoLevel      *level
) {
    auto world = physics->world;

    auto now = std::chrono::high_resolution_clock::now();
    float timeSinceLastFrame = std::chrono::duration_cast<std::chrono::milliseconds> (
        now - lastFrameTime
    ).count() / 1000.0f;
    lastFrameTime = now;

    if(config.isFrametimeOutputEnabled)
        std::cout << "frame time " << timeSinceLastFrame << std::endl;

    glm::mat4 projection = glm::perspective (
        glm::radians(60.0f),
        config.width / (float) config.height,
        0.001f, 3000.0f
    );
    projection[1][1] *= -1;  // openGL has the z dir flipped


    world->stepSimulation (timeSinceLastFrame);

    Scene::updateMatrices (
        scene->templates.data (), scene->instances.data (),
        scene->numInstances, mesh->alignModelTrans, true,
        (uint8_t *)mesh->alli_modelTrans.pMappedData
    );

    auto &player     = scene->templates[scene->instances[0].templateId].nodes[0];
    auto playerTrans = (JojoVulkanMesh::ModelTransformations *) (
        (uint8_t *)mesh->alli_modelTrans.pMappedData
        + mesh->alignModelTrans * player.dynamicTrans
    );
    glm::mat4 view = glm::inverse (playerTrans->model);

    Scene::updateNormalMatrices (
        scene->templates.data (), scene->instances.data (),
        scene->numInstances, view, mesh->alignModelTrans,
        (uint8_t *)mesh->alli_modelTrans.pMappedData
    );

    auto globalTrans = (JojoVulkanMesh::GlobalTransformations *)
        mesh->alli_globalTrans.pMappedData;
    globalTrans->projection = projection;
    globalTrans->view = view;

    const auto &lpos      = level->bsp->lightPos;
    const auto  numlights = lpos.size () + 1;
    auto lightInfo = (JojoVulkanMesh::LightBlock *)
        mesh->alli_lightInfo.pMappedData;
    lightInfo->parameters.x = config.gamma;   // Gamma
    lightInfo->parameters.y = config.hdrMode; // HDR enable
    lightInfo->parameters.z = 1.0f;           // HDR exposure
    lightInfo->parameters.w = (float)numlights;
    for (size_t i = 1; i < numlights; i++)
        lightInfo->sources[i].position = glm::vec3 (view * glm::vec4(lpos[i], 1.0f));
}


void gameloop (
    Config &config,
    JojoEngine *engine,
    JojoWindow *jojoWindow,
    JojoSwapchain *swapchain,
    Replay::Recorder            *jojoReplay,
    JojoVulkanMesh              *mesh,
    const JojoPipeline          *pipeline,
    const JojoPipeline          *textPipeline,
    const JojoPipeline          *levelPipeline,
    const Scene::SceneTemplates *scene,
    const Level::JojoLevel      *level,
    Physics::Physics            *physics
) {
    // TODO: extract a bunch of this to JojoWindow

    auto window = jojoWindow->window;
    jojoReplay->startRecording();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (jojoReplay->state() != Replay::RecorderState::Replaying) {
            auto state = jojoReplay->getKey(GLFW_KEY_SPACE);
            if (state == GLFW_PRESS)
                jojoReplay->startReplay();
        }

        btVector3 relativeForce(0, 0, 0);

        int state = jojoReplay->getKey(GLFW_KEY_W);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 0, -1);
        }
        state = jojoReplay->getKey(GLFW_KEY_S);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 0, 1);
        }
        state = jojoReplay->getKey(GLFW_KEY_A);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(-1, 0, 0);
        }
        state = jojoReplay->getKey(GLFW_KEY_D);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(1, 0, 0);
        }
        state = jojoReplay->getKey(GLFW_KEY_R);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, 1, 0);
        }
        state = jojoReplay->getKey(GLFW_KEY_F);
        if (state == GLFW_PRESS) {
            relativeForce = relativeForce + btVector3(0, -1, 0);
        }

        state = jojoReplay->getKey(GLFW_KEY_X);
        bool xPressed = state == GLFW_PRESS;

        state = jojoReplay->getKey(GLFW_KEY_Z);
        bool yPressed = state == GLFW_PRESS;


        btVector3 relativeTorque(0, 0, 0);

        state = jojoReplay->getKey(GLFW_KEY_Q);
        if (state == GLFW_PRESS) {
            relativeTorque = relativeTorque + btVector3(0, 1, 0);
        }
        state = jojoReplay->getKey(GLFW_KEY_E);
        if (state == GLFW_PRESS) {
            relativeTorque = relativeTorque + btVector3(0, -1, 0);
        }

        double xpos, ypos;
        jojoReplay->getCursorPos(&xpos, &ypos);

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

        if (jojoReplay->nextTickReady()) {
            jojoReplay->nextTick();

            if (!relativeForce.isZero () || !relativeTorque.isZero () || xPressed || yPressed) {
                auto body = scene->instances[0].body;

                btTransform trans;
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
            }

            updateMvp (
                config, engine,
                physics, mesh, scene, level
            );
        }

        drawFrame (
            config, engine, jojoWindow, swapchain,
            mesh, pipeline, textPipeline,
            levelPipeline, scene, level
        );
    }
}

void Rendering::DescriptorSets::createLayouts ()
{
    std::vector<VkDescriptorSetLayoutBinding> phong;
    addLayout (phong, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    addLayout (phong, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    addLayout (phong, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT);
    addLayout (phong, 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    addLayout (phong, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    layouts.push_back (createLayout (phong));

    std::vector<VkDescriptorSetLayoutBinding> text;
    addLayout (text, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    layouts.push_back (createLayout (text));

    std::vector<VkDescriptorSetLayoutBinding> level;
    addLayout (level, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    addLayout (level, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    addLayout (level, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    layouts.push_back (createLayout (level));
}

int main(int argc, char *argv[]) {
    Physics::Physics      physics   = {};
    Scene::SceneTemplates scene     = {};

    // --------------------------------------------------------------
    // ALLOCATE TEXTURE SPACE BEGIN
    // --------------------------------------------------------------

    {
        uint32_t *tex_ptr;

        scene.textures.resize (64);
        scene.textureCount = 2;

        tex_ptr = (uint32_t *)scene.textures[0].data ();
        for (int i = 0; i < 512; i++, tex_ptr += 512) {
            for (int j = 0; j < 512; j++) {
                tex_ptr[j] = -((i ^ j) >> 5 & 1) | 0xFFFF00FF;
            }
        }

        tex_ptr = (uint32_t *)scene.textures[1].data ();
        for (int i = 0; i < 512; i++, tex_ptr += 512) {
            for (int j = 0; j < 512; j++) {
                tex_ptr[j] = 0xFFFF0000;
            }
        }
    }

    // --------------------------------------------------------------
    // ALLOCATE TEXTURE SPACE END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // TEMPLATE LOADING BEGIN
    // --------------------------------------------------------------

    {
        const Scene::TemplateInfo templateFiles = {
            { "ship", { Object::Player }},
            { "roundcube", { Object::Box }},
            { "roundcube", { Object::Box }},
            { "roundcube", { Object::Box }},
            { "roundcube", { Object::Box }}
        };
        const uint32_t numTemplates = (uint32_t) templateFiles.size ();

        scene.templates.resize (numTemplates);
        for (uint32_t t = 0; t < numTemplates; ++t) {
            const auto &tfile = templateFiles[t];
            Scene::loadTemplate (
                tfile.first, tfile.second, t, &scene
            );
        }
    }

    // --------------------------------------------------------------
    // TEMPLATE LOADING END
    // --------------------------------------------------------------

    Config config = Config::readFromFile("config.ini");

    JojoWindow window;
    window.startGlfw(config);

    Replay::Recorder jojoReplay(window.window);
    jojoReplay.setResetFunc ([]() {});

    JojoEngine engine;
    engine.jojoWindow = &window;
    engine.startVulkan();
    engine.initializeDescriptorPool(100, 100, 100);

    JojoVulkanMesh mesh;
    mesh.scene = &scene;

    JojoSwapchain swapchain;
    swapchain.createCommandBuffers(&engine);
    swapchain.createSwapchainAndChildren(config, &engine);


    JojoPipeline pipeline;
    JojoPipeline levelPipeline;
    JojoPipeline textPipeline;

    // --------------------------------------------------------------
    // LEVEL LOADING START
    // --------------------------------------------------------------

    Level::JojoLevel *level = nullptr;

    {
        const auto allocator = engine.allocator;
        
        level = Level::alloc (allocator, "1");
        Level::loadRigidBodies (level);
    }

    // --------------------------------------------------------------
    // LEVEL LOADING END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // CREATE INSTANCES FOR CURRENT LEVEL START
    // --------------------------------------------------------------

    {
        using namespace glm;

        scene.instances.resize (3, {});
        scene.numInstances = 3;

        // Create player instance
        Scene::instantiate (
            translate(vec3(0.f, 2.5f, 0.f)), 2.0f,
            0, Scene::PlayerInstance,
            &scene, &scene.instances[0]
        );

        // Create a few boxes
        Scene::instantiate (
            translate (vec3 (0.f, 2.5f, -10.f)), 0.3f,
            1, Scene::NonLethalInstance,
            &scene, &scene.instances[1]
        );
        Scene::instantiate (
            translate (vec3 (-1.0f, 3.0f, -6.f)), 0.3f,
            2, Scene::NonLethalInstance,
            &scene, &scene.instances[2]
        );

        // Create physics world
        Physics::alloc (&physics);
        Physics::addInstancesToWorld (
            &physics, level,
            scene.instances.data (),
            scene.numInstances
        );
    }

    // --------------------------------------------------------------
    // CREATE INSTANCES FOR CURRENT LEVEL END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // PIPELINE CREATION START
    // --------------------------------------------------------------

    {
        pipeline.createPipelineHelper (
            config, &engine,
            swapchain.swapchainRenderPass, "phong",
            engine.descriptors->layout (Rendering::Set::Phong),
            { mesh.getVertexInputBindingDescription () },
            mesh.getVertexInputAttributeDescriptions ()
        );

        levelPipeline.createPipelineHelper (
            config, &engine,
            swapchain.swapchainRenderPass, "level",
            engine.descriptors->layout (Rendering::Set::Level),
            { Level::vertexInputBinding () },
            Level::vertexAttributes ()
        );

        textPipeline.createPipelineHelper (
            config, &engine,
            swapchain.swapchainRenderPass, "text",
            engine.descriptors->layout (Rendering::Set::Text)
        );
    }

    // --------------------------------------------------------------
    // PIPELINE CREATION END
    // --------------------------------------------------------------

    mesh.initializeBuffers(&engine, Rendering::Set::Phong);

    // initializeMaterialsAndLights (config, &engine, &scene, &mesh);

    Textures::Texture font;

    {
        Textures::fontTexture (
            engine.allocator, engine.device, 
            engine.commandPool, engine.queue,
            &font
        );

        auto descriptors = engine.descriptors;
        descriptors->update (Rendering::Set::Text, 0, Textures::descriptor(&font));
    }

    config.rebuildPipelines = ([&]() {
        vkDeviceWaitIdle(engine.device);
        std::cout << "Pipeline rebuilt" << std::endl;
        pipeline.destroyPipeline(&engine);
        pipeline.createPipelineHelper (
                config, &engine,
                swapchain.swapchainRenderPass, "phong",
                engine.descriptors->layout (Rendering::Set::Phong),
                { mesh.getVertexInputBindingDescription () },
                mesh.getVertexInputAttributeDescriptions()
        );

        levelPipeline.destroyPipeline (&engine);
        levelPipeline.createPipelineHelper (
            config, &engine,
            swapchain.swapchainRenderPass, "level",
            engine.descriptors->layout (Rendering::Set::Level),
            { Level::vertexInputBinding () },
            Level::vertexAttributes ()
        );
    });

    // --------------------------------------------------------------
    // INITIALIZE LIGHTSOURCES BEGIN
    // --------------------------------------------------------------

    {
        auto lblock = (JojoVulkanMesh::LightBlock *)
            mesh.alli_lightInfo.pMappedData;

        const auto &lpos      = level->bsp->lightPos;
        const auto  numlights = lpos.size () + 1;

        lblock->parameters.w           = (float)numlights;
        lblock->sources[0].color       = glm::vec3 (0.9f);
        lblock->sources[0].attenuation = glm::vec3 (0.3f, 0.05f, 0.01f);
        lblock->sources[0].position    = glm::vec3 (0.f, 0.f, -3.f);

        for (size_t i = 1; i < numlights; i++) {
            lblock->sources[i].color       = glm::vec3 (2.0, 0.0, 0.0);
            lblock->sources[i].attenuation = glm::vec3 (0.4f, 0.05f, 0.01f);
            lblock->sources[i].position    = lpos[i];
        }
    }
 
    // --------------------------------------------------------------
    // INITIALIZE LIGHTSOURCES END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // STAGING ONCE BEGIN
    // --------------------------------------------------------------

    {
        const auto cmd = swapchain.commandBuffers[0];
        const auto fence = swapchain.commandBufferFences[0];
        const auto allocator = engine.allocator;
        Level::CleanupQueue levelCleanupQueue;

        ASSERT_VULKAN (beginCommandBuffer (cmd));
        
        {
            levelCleanupQueue.reserve (10);
            Level::cmdStageVertexData (
                allocator, level, cmd, &levelCleanupQueue
            );
            Level::cmdLoadAndStageTextures (
                allocator, engine.device, cmd,
                level, &levelCleanupQueue
            );
            Level::cmdBuildAndStageIndicesNaively (
                allocator, engine.device, level, cmd
            );
        }

        {
            VkBuffer      staging;
            VmaAllocation stagingMem;

            Textures::cmdTextureArrayFromData (
                allocator, engine.device, cmd,
                scene.textures, scene.textureCount,
                512, 512, &mesh.textures,
                &staging, &stagingMem
            );

            auto tex = Textures::descriptor (&mesh.textures);
            engine.descriptors->update (Rendering::Set::Phong, 4, tex);

            levelCleanupQueue.emplace_back (staging, stagingMem);
        }

        {
            VkBufferCopy bufferCopy = {};
 
            bufferCopy.size = scene.vertices.size() * sizeof (Object::Vertex);
            vkCmdCopyBuffer (
                cmd, mesh.stage_vertex, mesh.vertex,
                1, &bufferCopy
            );
            bufferCopy.size = scene.indices.size () * sizeof (uint32_t);
            vkCmdCopyBuffer (
                cmd, mesh.stage_index, mesh.index,
                1, &bufferCopy
            );
            bufferCopy.size = scene.materials.size () * mesh.alignMaterialInfo;
            vkCmdCopyBuffer (
                cmd, mesh.stage_materialInfo, mesh.materialInfo,
                1, &bufferCopy
            );
        }

        ASSERT_VULKAN (vkEndCommandBuffer (cmd));

        {
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;

            ASSERT_VULKAN (vkResetFences (engine.device, 1, &fence));
            vkQueueSubmit (engine.queue, 1, &submitInfo, fence);

            // Wait for staging to complete
            // WARNING: do not reset fence here!
            //          apparently we get a deadlock otherwise
            auto result = VK_TIMEOUT;
            while (result == VK_TIMEOUT) {
                result = vkWaitForFences (
                    engine.device, 1, &fence,
                    VK_TRUE, 100000000
                );
            }
            ASSERT_VULKAN (result);
        }

        {
            // Staging cleanup
            for (const auto &pair : levelCleanupQueue)
                vmaDestroyBuffer (allocator, pair.first, pair.second);

            // Update descriptors
            Level::updateDescriptors (engine.descriptors, level);
        }
    }

    // --------------------------------------------------------------
    // STAGING ONCE END
    // --------------------------------------------------------------

    gameloop (
        config, &engine, &window, &swapchain, &jojoReplay,
        &mesh, &pipeline, &textPipeline, &levelPipeline,
        &scene, level, &physics
    );

    VkResult result = vkDeviceWaitIdle(engine.device);
    ASSERT_VULKAN (result);

    Textures::freeTexture (engine.allocator, engine.device, &font);

    mesh.destroyBuffers(&engine);

    // --------------------------------------------------------------
    // PHYSICS CLEANUP BEGIN
    // --------------------------------------------------------------

    {
        Physics::removeInstancesFromWorld (
            &physics, level,
            scene.instances.data (),
            scene.numInstances
        );
        Physics::free (&physics);
    }

    // --------------------------------------------------------------
    // PHYSICS CLEANUP END
    // --------------------------------------------------------------

    // --------------------------------------------------------------
    // LEVEL CLEANUP BEGIN
    // --------------------------------------------------------------

    {
        Level::free (engine.allocator, level);
    }
    
    // --------------------------------------------------------------
    // PHYSICS CLEANUP END
    // --------------------------------------------------------------

    swapchain.destroyCommandBuffers(&engine);

    pipeline.destroyPipeline(&engine);

    swapchain.destroySwapchainChildren(&engine);
    vkDestroySwapchainKHR(engine.device, swapchain.swapchain, nullptr);

    engine.shutdownVulkan();

    window.shutdownGlfw();

    return 0;
}


