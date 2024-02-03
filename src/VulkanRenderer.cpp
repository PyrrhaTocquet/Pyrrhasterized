#include "VulkanRenderer.h"

#pragma region CONSTRUCTORS_DESTRUCTORS
static void initRenderPass(VulkanRenderPass* renderPass, vk::DescriptorSetLayout geometryDescriptorSetLayout) {
    renderPass->createPushConstantsRanges();
    renderPass->createDescriptorSetLayout();
    renderPass->createDescriptorPool();
    renderPass->createPipelineRessources();

    renderPass->createPipelineLayout(geometryDescriptorSetLayout);
    renderPass->createDefaultPipeline();
}

VulkanRenderer::VulkanRenderer(VulkanContext* context)
{
     //Retrieving important values and references from VulkanContext
    m_context = context;
    m_device = context->getDevice();
    m_allocator = context->getAllocator();
    m_msaaSampleCount = context->getMaxUsableSampleCount();
    
    Material::createSamplers(m_context);

    //CAMERA
    m_camera = new Camera(m_context);
    
    createGeometryDescriptorSetLayout();
    createRenderPasses();

    { 
    std::vector<std::jthread> renderPassCreationThreads(m_renderPasses.size());
        for (size_t i = 0; i < m_renderPasses.size(); i++)
        {
            renderPassCreationThreads[i] = std::jthread(initRenderPass, m_renderPasses[i], m_geometryDescriptorSetLayout);
        }
    }
    //Rendering pipeline creation
    createFramebuffers();
   
    //Command execution related objects
    createCommandBuffers();
    createSyncObjects();
    
    //IMGUI
    ImGui_ImplVulkan_InitInfo initInfo = m_context->getImGuiInitInfo();
    initInfo.MSAASamples = static_cast<VkSampleCountFlagBits>(m_msaaSampleCount);
    ImGui_ImplVulkan_Init(&initInfo, m_renderPasses[RenderPassesId::MainRenderPassId]->getRenderPass());
    m_device.waitIdle();

    //execute a gpu command to upload imgui font textures
    vk::CommandBuffer cmd = m_context->beginSingleTimeCommands(m_context->getCommandPool());
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    m_context->endSingleTimeCommands(cmd, m_context->getCommandPool());

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();
    
    registerEntity(m_camera);
}

VulkanRenderer::~VulkanRenderer()
{

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        

        m_device.destroySemaphore(m_renderFinishedSemaphores[i]);
        m_device.destroySemaphore(m_imageAvailableSemaphores[i]);
        m_device.destroyFence(m_inFlightFences[i]);
    }

    Material::cleanSamplers(m_context);
    m_device.freeCommandBuffers(m_context->getCommandPool(), m_commandBuffers);
    delete m_camera;
    
    for (auto& renderPass : m_renderPasses)
    {
        delete renderPass;
    }

}
#pragma endregion

#pragma region PIPELINE
//adds a pipeline to the pipeline list
/*void VulkanRenderer::addPipeline(VulkanPipeline* pipeline) {
    m_pipelines.push_back(pipeline);
}*/

#pragma endregion



#pragma region FRAMEBUFFERS


//Recreate objects that depend on the swapchain image size to handle window resizing
void VulkanRenderer::recreateSwapchainSizedObjects() {
    cleanSwapchainSizedObjects();
    m_context->recreateSwapchain();
    for (const auto& renderPass : m_renderPasses)
    {
        renderPass->recreateRenderPass();
    }

    //createPipelineLayout();
    /*for (int i = 0; i < m_pipelines.size(); i++)
    {
        m_pipelines[i]->recreatePipeline();
    }*/
    
}

//Destroys  objects that depend on the swapchain image size to handle window resizing
void VulkanRenderer::cleanSwapchainSizedObjects() {
    vkDeviceWaitIdle(m_device);

    /*for (const auto& pipeline : m_pipelines) {
        pipeline->cleanPipeline();
    }*/
   // m_device.destroyPipelineLayout(m_pipelineLayout);
    
    m_context->cleanupSwapchain();

}

//creates framebuffer and attachments for each render passes
void VulkanRenderer::createFramebuffers() {
    for (auto& renderPass : m_renderPasses) {
        renderPass->createAttachments();
        renderPass->createFramebuffer();
    }
}
#pragma endregion

#pragma region COMMAND_BUFFERS
//creates the main command buffers
void VulkanRenderer::createCommandBuffers() {
    m_commandBuffers.resize(m_context->getSwapchainImagesCount());

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_context->getCommandPool(),
        .level = vk::CommandBufferLevel::ePrimary, //Primary : Can be submitted to a queue for execution, Secondary : Can be called from primary command buffers
        .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size()),
    };

    try {
        m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not allocate command buffers");
    }
}

//Records the main command buffer for frame generation
void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t swapchainImageIndex) 
{
    vk::CommandBufferBeginInfo beginInfo{};
    commandBuffer.begin(beginInfo); //TODO Revirtualise it well
    m_renderPasses[0]->drawRenderPass(commandBuffer, swapchainImageIndex, m_currentFrame, m_scenes);
    m_renderPasses[1]->drawRenderPass(commandBuffer, swapchainImageIndex, m_currentFrame, m_scenes);
    commandBuffer.end();
}
#pragma endregion
     

#pragma region SYNCHRONISATION
//create Synchronisation objects to manage frame generation execution
void VulkanRenderer::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreInfo{};

    vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};//Signaled by default so that the mainloop doesn't block at the first frame

    try {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            m_imageAvailableSemaphores[i] =  m_device.createSemaphore(semaphoreInfo);
            m_renderFinishedSemaphores[i] = m_device.createSemaphore(semaphoreInfo);
            m_inFlightFences[i] = m_device.createFence(fenceInfo);
        }
    }catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create synchronisation objects");
    }

}
#pragma endregion

#pragma region EXECUTION_FLOW
//Starts the window management and frame drawing loop
void VulkanRenderer::mainloop() {
    while (m_context->isWindowOpen() && !m_shouldStopRendering)
    {
        m_context->manageWindow();
        m_context->updateTime();
        manageInput();
        updateEntities();
        drawFrame();
    }
    m_device.waitIdle();
}

//Draws and presents a frame when a swapchain image is available
void VulkanRenderer::drawFrame() {

    //Wait and reset CPU semaphore
    m_device.waitForFences(1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);//Wait for one or all fences (VK_TRUE), uint64_max disables the timeout
    uint32_t imageIndex = m_context->acquireNextSwapchainImage(m_imageAvailableSemaphores[m_currentFrame]);
     m_device.resetFences(m_inFlightFences[m_currentFrame]); //Always reset fences (after being sure we are going to submit work
    m_commandBuffers[imageIndex].reset(); //Reset to record the command buffer
    
    for (auto& renderPass : m_renderPasses)
    {
        renderPass->updatePipelineRessources(m_currentFrame, m_scenes);
    }
    recordCommandBuffer(m_commandBuffers[imageIndex], imageIndex);
  
    //Submitting the command buffer
    vk::Semaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1, //Specifies which semaphores to wait on before executions begin and on which stage to wait.
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages, //Stages can be ran while the image is not yet available. (Stages are linked to waitSemaphores indexes)
        .commandBufferCount = 1, //Command buffers to submit for execution
        .pCommandBuffers = &m_commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,  //Semaphores to signal once the buffers have finished execution
        .pSignalSemaphores = signalSemaphores,
    };

    //inFlightFence will be signaled when the command buffers finished executing
    try {
        m_context->getGraphicsQueue().submit(submitInfo, m_inFlightFences[m_currentFrame]);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }


    if (!present(signalSemaphores, imageIndex)) {
        recreateSwapchainSizedObjects();
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

/// <summary>
/// Presents the swapchain image of index imageIndex
/// </summary>
/// <param name="signalSemaphores"></param>
/// <param name="imageIndex"></param>
/// <returns>false if the swapchain is no longer valid. It means the swapchain and swapchain size related objects have to be recreated</returns>
bool VulkanRenderer::present(vk::Semaphore *signalSemaphores, uint32_t imageIndex) {

    vk::SwapchainKHR swapchains[] = { m_context->getSwapchain()};
    vk::Result cResult;
    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex,
        .pResults = &cResult,
    };
    //Vulkan Hpp crashes for eErrorOutOfDateKHR
    vkQueuePresentKHR(static_cast<VkQueue>(m_context->getPresentQueue()), reinterpret_cast<VkPresentInfoKHR*>(&presentInfo));
    
    vk::Result result = reinterpret_cast<vk::Result>(cResult);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_context->isFramebufferResized())
    {
        m_context->clearFramebufferResized();
        return false;
    }
    else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    return true;
}
#pragma endregion

#pragma region SCENES
//Adds a scene to the scenes to be renderer. Generates the descriptor sets with the textures before adding.
void VulkanRenderer::addScene(VulkanScene* vulkanScene) {
    //TODO MAKE SURE THERE IS A UNIQUE SCENE !!!!!
    vulkanScene->loadModels();
    vulkanScene->createGeometryBuffers();
    vulkanScene->createGeometryDescriptorSet(m_geometryDescriptorSetLayout);
    for (auto& renderPass : m_renderPasses)
    {
        renderPass->createDescriptorSets(vulkanScene);
    }
    
    m_scenes.push_back(vulkanScene);
    for (auto& light : vulkanScene->getLights())
    {
        light->setCamera(m_camera);
        registerEntity(light);
    }
}
#pragma endregion

#pragma region RENDER_PASSES
void VulkanRenderer::createRenderPasses() {
    //Render pass 1: Shadow Render Pass
    ShadowCascadeRenderPass* shadowRenderPass = new ShadowCascadeRenderPass(m_context, m_camera);
    m_shadowPass = shadowRenderPass;
    shadowRenderPass->createRenderPass();
    m_renderPasses.push_back(shadowRenderPass);

    //Render pass 2: Main Render Pass
    MainRenderPass* mainRenderPass = new MainRenderPass(m_context, m_camera, shadowRenderPass);
    mainRenderPass->createRenderPass();
    m_mainPass = mainRenderPass;
    m_renderPasses.push_back(mainRenderPass);

}

void VulkanRenderer::createGeometryDescriptorSetLayout()
{
    //Creates the VkDescriptorSetLayout for the mesh shader's geometry buffers
	vk::Device device = m_context->getDevice();

    /*
	0: Meshlet Info
	1: Primitives
	2: Indices
	3: Vertices
	*/
    vk::DescriptorSetLayoutBinding meshletInfoBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
    };

	vk::DescriptorSetLayoutBinding primitivesBinding = meshletInfoBinding;
	vk::DescriptorSetLayoutBinding indicesBinding = meshletInfoBinding;
	vk::DescriptorSetLayoutBinding verticesBinding = meshletInfoBinding;

	primitivesBinding.binding = 1;
	indicesBinding.binding = 2;
	verticesBinding.binding = 3;

    vk::DescriptorSetLayoutBinding bindings[4] = { meshletInfoBinding, primitivesBinding, indicesBinding, verticesBinding };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 4,
        .pBindings = bindings,
    };

    try {
        m_geometryDescriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
    }
    catch (vk::SystemError err)
    {
        throw std::runtime_error("could not create descriptor set layout");
    }

}
#pragma endregion RENDER_PASSES

#pragma region INPUT
//Function that manages keyboard input behavior
void VulkanRenderer::manageInput() {

    GLFWwindow* window = m_context->getWindowPtr();
    int key = glfwGetKey(window, GLFW_KEY_ESCAPE);
    if (key == GLFW_PRESS) {
        m_shouldStopRendering = true;
    } 
}

#pragma endregion


#pragma region ENTITIES
//Iterates the entity list to call their update function
void VulkanRenderer::updateEntities() {
    for (auto& entity : m_entities) {
        entity->update();
    }
    for (auto& scene : m_scenes) {
        scene->updateLights();
    }
}

//Adds the entity pointer to the entity list
void VulkanRenderer::registerEntity(Entity* entity) {
    m_entities.push_back(entity);
}
#pragma endregion ENTITIES