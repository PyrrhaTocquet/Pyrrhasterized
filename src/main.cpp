/*  Vulkan Base Project
    Goal: Making a best practice Vulkan project to be used as a starting point for rasterized computer graphics projects
    Author: Pyrrha Tocquet
    Date: 24/03/2023
*/


#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.hpp"


#include <iostream>
#include "VulkanContext.h"
#include "VulkanRenderer.h"
#include "VulkanScene.h"
#include "Peach.h"

int main() {

    VulkanContext context;
    VulkanRenderer renderer(&context);
    VulkanScene scene(&context);
    
    /* Ganon */
    Transform ganonTransform;
    ganonTransform.translate = glm::vec3(-0.f, 0.f, -.5f);
    ganonTransform.rotate = glm::vec3(0.f, 90.f, 0.f);

    /* Pikachu */
    Transform pikachuTransform;
    pikachuTransform.rotate = glm::vec3(0.f, 90.f, 0.f);

    /* Mario */
    

    /* Sponza */
    Transform sponzaTransform;
    sponzaTransform.translate = glm::vec3(0.f, -2.f, 0.0f);
    sponzaTransform.scale = glm::vec3(0.005f, 0.005f, 0.005f);

    /* Link */
    Transform linkTransform;
    linkTransform.translate = glm::vec3(3.f, 0.f, -2.f);
    linkTransform.scale = glm::vec3(0.05f, 0.05f, 0.05f);
    linkTransform.rotate = glm::vec3(0.f, -115.f, 0.f);

    /* Chest */
    Transform chestTransform;
    chestTransform.translate = glm::vec3(0.f, 0.f, 2.f);

    scene.addModel("assets/TreasureChest/model.gltf", chestTransform);
    scene.addModel("assets/SponzaGltf/model.glb", sponzaTransform);
    scene.addModel("assets/Ganon/ganon.gltf", ganonTransform);
    scene.addModel("assets/PikachuObj/model.obj", pikachuTransform);
    Peach* peachEntity = new Peach(&context);
    scene.addEntity(peachEntity);
    renderer.registerEntity(peachEntity);
    scene.addModel("assets/Link/model.obj", linkTransform);

    renderer.addScene(&scene);
    
    PipelineInfo pipelineInfo;

    pipelineInfo.vertPath = "shaders/vertexTextureNoLight.spv";
    pipelineInfo.fragPath = "shaders/fragmentTextureNoLight.spv";
    renderer.addPipeline(renderer.createPipeline(pipelineInfo));

    pipelineInfo.vertPath = "shaders/vertexDebugNormals.spv";
    pipelineInfo.fragPath = "shaders/fragmentDebugNormals.spv";
    renderer.addPipeline(renderer.createPipeline(pipelineInfo));

    pipelineInfo.vertPath = "shaders/vertexDebugTextureCoords.spv";
    pipelineInfo.fragPath = "shaders/fragmentDebugTextureCoords.spv";
    renderer.addPipeline(renderer.createPipeline(pipelineInfo));



    pipelineInfo.vertPath = "shaders/vertexTextureNoLight.spv";
    pipelineInfo.fragPath = "shaders/fragmentDepth.spv";
    renderer.addPipeline(renderer.createPipeline(pipelineInfo));

    pipelineInfo.vertPath = "shaders/vertexTangents.spv";
    pipelineInfo.fragPath = "shaders/fragmentTangents.spv";

    renderer.addPipeline(renderer.createPipeline(pipelineInfo));

    pipelineInfo.vertPath = "shaders/vertexTextureNoLight.spv";
    pipelineInfo.fragPath = "shaders/fragmentTextureNoLight.spv";
    pipelineInfo.polygonMode = vk::PolygonMode::eLine;

    renderer.addPipeline(renderer.createPipeline(pipelineInfo));


    

    renderer.mainloop();

    return 0;
}