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
  
    static bool zPressed = false;

    GLFWwindow* window = m_context->getWindowPtr();
   
    //Camrea Movement
    float cameraMovement = c_cameraSpeed * m_context->getTime().deltaTime;
    int key = glfwGetKey(window, GLFW_KEY_W);
    if (key == GLFW_PRESS && zPressed == false) {
        m_cameraCoords.cameraPos -= cameraMovement * m_cameraCoords.getDirection(); //TODO better
        zPressed = true;
    }
    else if (zPressed == true && key == GLFW_RELEASE)
    {
        zPressed = false;
    }
    else if (zPressed == true)
    {
        m_cameraCoords.cameraPos -= cameraMovement * m_cameraCoords.getDirection(); //TODO better
    }

    //Camera Rotation
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

//returns the view matrix of the camera
glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(m_cameraCoords.cameraPos, m_cameraCoords.cameraPos - m_cameraCoords.getDirection(), glm::vec3(0.0f, 1.0f, 0.0f));
}

//retrieves the projection matrix of the camera
glm::mat4 Camera::getProjMatrix(VulkanContext* context)
{
    vk::Extent2D extent = context->getSwapchainExtent();
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, nearPlane, farPlane); //45deg vertical field of view, aspect ratio, near and far view planes
    proj[1][1] *= -1; //Designed for openGL but the Y coordinate of the clip coordinates is inverted
    return proj;
}

//return the camera position
glm::vec3 Camera::getCameraPos()
{
    return m_cameraCoords.cameraPos;
}

