#include "NeuromeGUI.h"

NeuromeGUI::NeuromeGUI() noexcept
	: m_windowTitle("Neurome!"),
	m_windowWidth(1280), m_windowHeight(720),
	m_initalized(false), m_window(nullptr),
    m_clearColor(0.12f, 0.15f, 0.18f, 1.00f)
{
    setlocale(LC_ALL, "");

	if (!glfwInit()) 
    {
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, m_windowTitle.c_str(), nullptr, nullptr);
    if (!m_window) 
    {
        return;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    glfwSetErrorCallback(glfwErrorCallback);

    m_initalized = true;
}

NeuromeGUI::~NeuromeGUI()
{
    if (m_initalized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}

void NeuromeGUI::render()
{
    if (!m_initalized)
    {
        throw std::runtime_error("Failed to initialize the graphical user interface");
    }

    while (!glfwWindowShouldClose(m_window)) 
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Region 1");
        ImGui::Text("2t");
        ImGui::End();

        ImGui::Begin("Region 2");
        ImGui::Text("1t");
        ImGui::Button("test");
        ImGui::End();

        update();
    }
}

void NeuromeGUI::glfwErrorCallback(int error, const char *description)
{
    char buffer[MAX_STR];
    snprintf(buffer, sizeof(buffer), "GLFW Error %d: %s", error, description);

    throw std::runtime_error(buffer);
}

void NeuromeGUI::update() 
{
    ImGui::Render();

    int displayW, displayH;
    glfwGetFramebufferSize(m_window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);

    glClearColor(m_clearColor.x * m_clearColor.w,
                 m_clearColor.y * m_clearColor.w,
                 m_clearColor.z * m_clearColor.w,
                 m_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}