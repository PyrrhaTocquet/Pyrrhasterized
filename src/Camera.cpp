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
    static bool sPressed = false;

    GLFWwindow* window = m_context->getWindowPtr();
   
    //Camrea Movement
    float cameraMovement = (cameraSpeed + glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) * fastCameraSpeed) * m_context->getTime().deltaTime;
    int zKey = glfwGetKey(window, GLFW_KEY_W);
    int sKey = glfwGetKey(window, GLFW_KEY_S);
    if (zKey == GLFW_PRESS && zPressed == false) {
        m_cameraCoords.cameraPos -= cameraMovement * m_cameraCoords.getDirection(); //TODO better
        zPressed = true;
    }
    else if (zPressed == true && zKey == GLFW_RELEASE)
    {
        zPressed = false;
    }
    else if (zPressed == true)
    {
        m_cameraCoords.cameraPos -= cameraMovement * m_cameraCoords.getDirection(); //TODO better
    }

     if (sKey == GLFW_PRESS && sPressed == false) {
        m_cameraCoords.cameraPos += cameraMovement * m_cameraCoords.getDirection(); //TODO better
        sPressed = true;
    }
    else if (sPressed == true && sKey == GLFW_RELEASE)
    {
        sPressed = false;
    }
    else if (sPressed == true)
    {
        m_cameraCoords.cameraPos += cameraMovement * m_cameraCoords.getDirection(); //TODO better
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

    float sensitivity = 0.4f;
    deltaX *= sensitivity;
    deltaY *= sensitivity;
    
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
    {
        m_cameraCoords.pitchYawRoll.y += deltaX;
        m_cameraCoords.pitchYawRoll.x += deltaY;
    }
   

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

