#include "Peach.h"

Peach::Peach(VulkanContext* context, VulkanScene *scene, int parent /* = -1*/) : Entity(context)
{
    Transform peachTransform;
    peachTransform.translate = glm::vec3(-0.5f, 0.f, 0.6f);
    peachTransform.rotate = glm::vec3(0.f, 90.f, 0);

    m_node = scene->addMeshNode("assets/PeachHD/Peach.gltf", peachTransform, parent);
}

void Peach::update(VulkanScene *scene)
{
    float speed = 1.0f;

    static bool oPressed = false;
    static bool lPressed = false;
    static bool kPressed = false;
    static bool mPressed = false;

    GLFWwindow* window = m_context->getWindowPtr();
    float deltaTime = m_context->getTime().deltaTime;
    int key = glfwGetKey(window, GLFW_KEY_O);
    if (key == GLFW_PRESS) {
        scene->translateNode(m_node, glm::vec3(deltaTime * speed, 0.f, 0.f));
        oPressed = true;
    }
    else if (oPressed == true && key == GLFW_RELEASE)
    {
        oPressed = false;
    }

    key = glfwGetKey(window, GLFW_KEY_L);
    if (key == GLFW_PRESS) {
       scene->translateNode(m_node, glm::vec3(-deltaTime * speed, 0.f, 0.f));
        lPressed = true;
    }
    else if (lPressed == true && key == GLFW_RELEASE)
    {
        lPressed = false;
    }

    key = glfwGetKey(window, GLFW_KEY_K);
    if (key == GLFW_PRESS) {
        scene->translateNode(m_node, glm::vec3(0.f, 0.f, -deltaTime * speed));
        kPressed = true;
    }
    else if (kPressed == true && key == GLFW_RELEASE)
    {
        kPressed = false;
    }

    key = glfwGetKey(window, GLFW_KEY_SEMICOLON);
    if (key == GLFW_PRESS) {
        scene->translateNode(m_node, glm::vec3(0.f, 0.f, deltaTime * speed));
        mPressed = true;
    }
    else if (mPressed == true && key == GLFW_RELEASE)
    {
        mPressed = false;
    }

}
