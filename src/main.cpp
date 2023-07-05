/*  Pyrrhasterized
    Goal: Making a porte-folio + learning project 3D rasterized renderer
    Author: Pyrrha Tocquet
    Date: 22/05/2023

    Forked from Vulkan Base Project (Private repo) (original file date: 24/03/2023)
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
#include "DirectionalLight.h"
#include "PointLight.h"
#include "Spotlight.h"
#include "Peach.h"

int main() {

    VulkanContext context;
    VulkanRenderer renderer(&context);

    DirectionalLight sun(&context, -glm::vec4(5.f, 50.f, 0.f, 0.f), glm::vec4(1.f, 1.f, .95f, 1.f));
    sun.setIntensity(5.0f);
    VulkanScene scene(&context, &sun);
    
    /* Ganon */
    Transform ganonTransform;
    ganonTransform.translate = glm::vec3(-0.f, 0.f, -.5f);
    ganonTransform.rotate = glm::vec3(0.f, 90.f, 0.f);

    /* Helmet */
    Transform helmetTransform;
    helmetTransform.translate = glm::vec3(0.f, 1.12f, -0.5f);
    helmetTransform.rotate = glm::vec3(90.f, 0.f, -90.f);
    helmetTransform.scale = glm::vec3(0.15f, 0.15f, 0.15f);

  
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
    scene.addModel("assets/SponzaGltf/sponza.glb", sponzaTransform);
    scene.addModel("assets/Ganon/ganon.gltf", ganonTransform);
    scene.addModel("assets/Helmet/DamagedHelmet.gltf", helmetTransform);
    Peach* peachEntity = new Peach(&context);
    scene.addEntity(peachEntity);
    renderer.registerEntity(peachEntity);

    Transform sphereMaterialsTransform;
    sphereMaterialsTransform.translate.y = 10;
    sphereMaterialsTransform.translate.x = 10;
    sphereMaterialsTransform.scale *= 0.1;
    scene.addModel("assets/SphereMaterials/spherematerials.glb", sphereMaterialsTransform);
    
    /*
    Transform transform;
    scene.addModel("assets/Splatoon/splatoon.gltf", transform);
    */
    /*
    Transform transform;
    scene.addModel("assets/CubeScene/cubeScene.gltf", transform);
    */
    PointLight* pointLight = new PointLight(&context, glm::vec4(6.0f, -1.1f, -2.2f, 1.f), 2.5f, glm::vec4(.1f, .1f, 1.f, 1.0f));
    pointLight->setIntensity(5.0f);
    scene.addLight(pointLight);

    PointLight* pointLightDos = new PointLight(&context, glm::vec4(6.0f, -1.1f, 2.2f, 1.f), 2.5f, glm::vec4(.1f, 1.f, .1f, 1.0f));
    pointLightDos->setIntensity(5.0f);
    scene.addLight(pointLightDos);
    
    Spotlight* spotlight = new Spotlight(&context, glm::vec4(-7.1f, -1.1f, 2.6f, 1.f), glm::vec4(-7.1f, -1.1f, -2.6f, 1.f) - glm::vec4(-7.1f, -1.1f, 2.6f, 1.f), 10.f, 8.f, glm::vec4(1.f, .2f, .2f, 1.f));
    spotlight->setIntensity(3.0f);
    scene.addLight(spotlight);

    renderer.addScene(&scene);

    renderer.mainloop();

    return 0;
}
