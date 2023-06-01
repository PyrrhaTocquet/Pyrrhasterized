#include "Peach.h"

Peach::Peach(VulkanContext* context) : Entity(context)
{
    Transform peachTransform;
    peachTransform.translate = glm::vec3(-0.5f, 0.f, 0.5f);
    peachTransform.rotate = glm::vec3(0.f, 90.f, 0);
    peachTransform.scale = glm::vec3(0.005f, 0.005f, 0.005f);

	m_model = new Model(context, "assets/Peach/model.obj", peachTransform);
}

void Peach::update()
{
    float speed = 1.0f;

    //Time since rendering start
    static auto prevTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - prevTime).count();
    prevTime = currentTime;

    static bool oPressed = false;
    static bool lPressed = false;
    static bool kPressed = false;
    static bool mPressed = false;

    GLFWwindow* window = m_context->getWindowPtr();

    int key = glfwGetKey(window, GLFW_KEY_O);
    if (key == GLFW_PRESS) {
        m_model->translateBy(glm::vec3(deltaTime * speed, 0.f, 0.f));
        oPressed = true;
    }
    else if (oPressed == true && key == GLFW_RELEASE)
    {
        oPressed = false;
    }

    key = glfwGetKey(window, GLFW_KEY_L);
    if (key == GLFW_PRESS) {
        m_model->translateBy(glm::vec3(-deltaTime * speed, 0.f, 0.f));
        lPressed = true;
    }
    else if (lPressed == true && key == GLFW_RELEASE)
    {
        lPressed = false;
    }

    key = glfwGetKey(window, GLFW_KEY_K);
    if (key == GLFW_PRESS) {
        m_model->translateBy(glm::vec3(0.f, 0.f, -deltaTime * speed));
        kPressed = true;
    }
    else if (kPressed == true && key == GLFW_RELEASE)
    {
        kPressed = false;
    }

    key = glfwGetKey(window, GLFW_KEY_SEMICOLON);
    if (key == GLFW_PRESS) {
        m_model->translateBy(glm::vec3(0.f, 0.f, deltaTime * speed));
        mPressed = true;
    }
    else if (mPressed == true && key == GLFW_RELEASE)
    {
        mPressed = false;
    }




}
