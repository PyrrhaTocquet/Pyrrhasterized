#include "Camera.h"


Camera::Camera(VulkanContext* context) : Entity(context)
{
    Transform cameraTransform;
	cameraTransform.translate = m_cameraCoords.cameraPos;
	cameraTransform.rotate = m_cameraCoords.pitchYawRoll;

    m_model = new Model(context, "assets/Camera/camera.gltf", cameraTransform);
}

void Camera::update()
{
    //Time since rendering start
    static auto prevTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();
    prevTime = currentTime;

    static bool zPressed = false;

    GLFWwindow* window = m_context->getWindowPtr();
   
    float cameraSpeed = 10 * deltaTime;
    int key = glfwGetKey(window, GLFW_KEY_W);
    if (key == GLFW_PRESS && zPressed == false) {
        m_cameraCoords.cameraPos -= cameraSpeed * m_cameraCoords.getDirection(); //TODO better
        zPressed = true;
    }
    else if (zPressed == true && key == GLFW_RELEASE)
    {
        zPressed = false;
    }
    else if (zPressed == true)
    {
        m_cameraCoords.cameraPos -= cameraSpeed * m_cameraCoords.getDirection(); //TODO better
    }


    static double lastMousePosX = 500;
    static double lastMousePosY = 500;
    double posX, posY;
    glfwGetCursorPos(window, &posX, &posY);

    float deltaX = posX - lastMousePosX;
    float deltaY = lastMousePosY - posY;
    lastMousePosX = posX;
    lastMousePosY = posY;

    float sensitivity = 0.2f;
    deltaX *= sensitivity;
    deltaY *= sensitivity;

    m_cameraCoords.pitchYawRoll.y += deltaX;
    m_cameraCoords.pitchYawRoll.x += deltaY;

    if (m_cameraCoords.pitchYawRoll.x > 89.0f)
        m_cameraCoords.pitchYawRoll.x = 89.f;
    if (m_cameraCoords.pitchYawRoll.x < -89.0f)
        m_cameraCoords.pitchYawRoll.x = -89.0f;


}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(m_cameraCoords.cameraPos, m_cameraCoords.cameraPos - m_cameraCoords.getDirection(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::getProjMatrix(VulkanContext* context)
{
    vk::Extent2D extent = context->getSwapchainExtent();
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, nearPlane, farPlane); //45deg vertical field of view, aspect ratio, near and far view planes
    proj[1][1] *= -1; //Designed for openGL but the Y coordinate of the clip coordinates is inverted
    return proj;
}

glm::vec3 Camera::getCameraPos()
{
    return m_cameraCoords.cameraPos;
}

