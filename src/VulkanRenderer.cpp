#include "VulkanRenderer.h"

#pragma region CONSTRUCTORS_DESTRUCTORS
VulkanRenderer::VulkanRenderer(VulkanContext* context)
{
     //Retrieving important values and references from VulkanContext
    m_context = context;
    m_device = context->getDevice();
    m_allocator = context->getAllocator();
    m_msaaSampleCount = context->getMaxUsableSampleCount();
    

    //CAMERA
    m_camera = new Camera(m_context);
    createRenderPasses();

    for (auto& renderPass : m_renderPasses)
    {
        renderPass->createPushConstantsRanges();
        renderPass->createDescriptorSetLayout();
        renderPass->createDescriptorPool();
        renderPass->createPipelineRessources();

        renderPass->createPipelineLayout();
        renderPass->createDefaultPipeline();
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
    vk::CommandBuffer cmd = m_context->beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    m_context->endSingleTimeCommands(cmd);

    //clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

 
    registerEntity(m_camera);

}

VulkanRenderer::~VulkanRenderer()
{
    delete m_camera;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        

        m_device.destroySemaphore(m_renderFinishedSemaphores[i]);
        m_device.destroySemaphore(m_imageAvailableSemaphores[i]);
        m_device.destroyFence(m_inFlightFences[i]);
    }


    m_device.freeCommandBuffers(m_context->getCommandPool(), m_commandBuffers);

    
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
    while (m_context->isWindowOpen())
    {
        m_context->manageWindow();
        manageInput();
        updateEntities();
        for (auto& renderPass : m_renderPasses)
        {
            renderPass->updatePipelineRessources(m_currentFrame);
        }
        drawFrame();
    }
    m_device.waitIdle();
}

//Draws and presents a frame when a swapchain image is available
void VulkanRenderer::drawFrame() {

    //Wait and reset CPU semaphore
    m_device.waitForFences(1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);//Wait for one or all fences (VK_TRUE), uint64_max disables the timeout
    m_device.resetFences(m_inFlightFences[m_currentFrame]); //Always reset fences (after being sure we are going to submit work
    uint32_t imageIndex = m_context->acquireNextSwapchainImage(m_imageAvailableSemaphores[m_currentFrame]);
    m_commandBuffers[m_currentFrame].reset(); //Reset to record the command buffer
    

    recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);
  
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
    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };
    
    vk::Result result = m_context->getPresentQueue().presentKHR(presentInfo);
    
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
    vulkanScene->createBuffers();
    for (auto& renderPass : m_renderPasses)
    {
        renderPass->createDescriptorSet(vulkanScene);
    }
    
    m_scenes.push_back(vulkanScene);
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
#pragma endregion RENDER_PASSES

#pragma region INPUT
void VulkanRenderer::manageInput() {
    static bool rightPressed = false;
    static bool leftPressed = false;

    GLFWwindow* window = m_context->getWindowPtr();
    int key = glfwGetKey(window, GLFW_KEY_RIGHT);
    if (key == GLFW_PRESS && rightPressed == false) {
        //m_currentPipelineId++;
        rightPressed = true;
    }
    else if (rightPressed == true && key == GLFW_RELEASE)
    {
        rightPressed = false;
    }

    key = glfwGetKey(window, GLFW_KEY_LEFT);
    if (key == GLFW_PRESS && leftPressed == false) {
        //m_currentPipelineId--;
        leftPressed = true;
    }
    else if (leftPressed == true && key == GLFW_RELEASE)
    {
        leftPressed = false;
    }
    /*if (m_currentPipelineId == -1) m_currentPipelineId = static_cast<uint32_t>(m_pipelines.size()) - 1;
    m_currentPipelineId = m_currentPipelineId % static_cast<uint32_t>(m_pipelines.size());*/
}

#pragma endregion


#pragma region ENTITIES
void VulkanRenderer::updateEntities() {
    for (auto& entity : m_entities) {
        entity->update();
    }
}

void VulkanRenderer::registerEntity(Entity* entity) {
    m_entities.push_back(entity);
}
#pragma endregion ENTITIES