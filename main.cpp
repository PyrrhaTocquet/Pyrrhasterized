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

int main() {

    VulkanContext context;
    VulkanRenderer renderer(&context);
    VulkanScene scene(&context);
    
    /* Ganon */
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-.5f, -.5f, -0.5f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(.015, .015, .015));
    glm::mat4 ganonTransform = translation * scale;

    /* Pikachu */
    translation = glm::translate(glm::mat4(1.0f), glm::vec3(-.5f, .5f, 0.f));
    translation = glm::rotate(translation, glm::radians(-90.f), glm::vec3(1.0f, 0.f, .0f));
    glm::mat4 pikachuTransform = translation;

    /* Mario */
    translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.0, 0.f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.005f, 0.005f, 0.005f));
    glm::mat4 peachTransform = translation * scale;

    /* Sponza */
    translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.f, 0.0, -2.f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.005f, 0.005f, 0.005f));
    glm::mat4 sponzaTransform = translation * scale;

    /* Link */
    translation = glm::translate(glm::mat4(1.0f), glm::vec3(3.f, 0.0, -2.f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(.05f, .05f, .05f));
    glm::mat4 linkTransform = translation * scale;
    linkTransform = glm::rotate(linkTransform, glm::radians(-115.f), glm::vec3(0.f, 0.f, 1.f));


    ganonTransform = glm::rotate(ganonTransform, glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
    pikachuTransform = glm::rotate(pikachuTransform, glm::radians(90.f), glm::vec3(0.0f, 0.f, 1.f));
    peachTransform = glm::rotate(peachTransform, glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
    
    scene.addObjModel("F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Ressources/Sponza/", sponzaTransform);
    scene.addObjModel("F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Ressources/Ganon/", ganonTransform);
    scene.addObjModel("F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Ressources/PikachuObj/", pikachuTransform);
    scene.addObjModel("F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Ressources/peach/", peachTransform);
    scene.addObjModel("F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Ressources/Link/", linkTransform);

    renderer.addScene(&scene);

    PipelineInfo pipelineInfo;

    pipelineInfo.vertPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/vertexTextureNoLight.spv";
    pipelineInfo.fragPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/fragmentTextureNoLight.spv";
    renderer.addPipeline(renderer.createPipeline(pipelineInfo));

    pipelineInfo.vertPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/vertexDebugNormals.spv";
    pipelineInfo.fragPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/fragmentDebugNormals.spv";
    renderer.addPipeline(renderer.createPipeline(pipelineInfo));

    pipelineInfo.vertPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/vertexDebugTextureCoords.spv";
    pipelineInfo.fragPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/fragmentDebugTextureCoords.spv";
    renderer.addPipeline(renderer.createPipeline(pipelineInfo));



    pipelineInfo.vertPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/vertexTextureNoLight.spv";
    pipelineInfo.fragPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/fragmentDepth.spv";
    renderer.addPipeline(renderer.createPipeline(pipelineInfo));

    pipelineInfo.vertPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/vertexTextureNoLight.spv";
    pipelineInfo.fragPath = "F:/Programmation/C++/Vulkan Projects/Vulkan_Base_Project/Shaders/fragmentTextureNoLight.spv";
    pipelineInfo.polygonMode = vk::PolygonMode::eLine;

    renderer.addPipeline(renderer.createPipeline(pipelineInfo));


    renderer.mainloop();

    return 0;
}