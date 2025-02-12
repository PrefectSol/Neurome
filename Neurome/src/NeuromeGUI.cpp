#include "NeuromeGUI.h"

NeuromeGUI::NeuromeGUI() noexcept
	: m_windowTitle("Better than a command line!"),
	m_windowWidth(1280), m_windowHeight(720),
	m_initalized(false), m_window(nullptr),
    m_settingsWidth(0.25f), m_canvasHeight(0.7f), m_manageWidth(0.5f),
    m_isDragging(false), m_dragStartX(0.0), m_dragStartY(0.0)
{
    setlocale(LC_ALL, "");

	if (!glfwInit()) 
    {
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

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
    io.Fonts->AddFontFromFileTTF("data/fonts/Cousine-Regular.ttf", 15.0f);

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    glfwSetErrorCallback(glfwErrorCallback);

    setInitialWindowPos();

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

    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoCollapse |      
                                         ImGuiWindowFlags_NoBringToFrontOnFocus;

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

        const float menuBarHeight = beginMenuBar();

        beginSettings(menuBarHeight, windowFlags);
        beginCanvas(menuBarHeight, windowFlags);
        beginManage(menuBarHeight, windowFlags);
        beginInfo(menuBarHeight, windowFlags);

        checkResize();
        resizeWindow();
        update();
    }
}

void NeuromeGUI::update()
{
    const ImVec4 clearColor(0.12f, 0.15f, 0.18f, 1.00f);

    ImGui::Render();

    glfwGetFramebufferSize(m_window, &m_windowWidth, &m_windowHeight);
    glViewport(0, 0, m_windowWidth, m_windowHeight);

    glClearColor(clearColor.x * clearColor.w,
                 clearColor.y * clearColor.w,
                 clearColor.z * clearColor.w,
                 clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}

float NeuromeGUI::beginMenuBar()
{
    const float framePadding = 4.0f;
    const float buttonPadding = 20.0f;

    const ImVec4 buttonStyle(0.2f, 0.2f, 0.2f, 0.6f);
    const ImVec4 buttonHoveredStyle(0.3f, 0.3f, 0.3f, 0.8f);
    const ImVec4 buttonActiveStyle(0.4f, 0.4f, 0.4f, 1.0f);

    const char *minButtonName = "min";
    const char *maxButtonName = "max";
    const char *closeButtonName = "close";

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, framePadding));
    if (ImGui::BeginMainMenuBar())
    {
        checkDragging();

        const float titleWidth = ImGui::CalcTextSize(m_windowTitle.c_str()).x;

        ImGui::SetCursorPosX((m_windowWidth - titleWidth) * 0.5f);
        ImGui::Text(m_windowTitle.c_str());

        const float minButtonWidth = ImGui::CalcTextSize(minButtonName).x + buttonPadding;
        const float maxButtonWidth = ImGui::CalcTextSize(maxButtonName).x + buttonPadding;
        const float closeButtonWidth = ImGui::CalcTextSize(closeButtonName).x + buttonPadding;
        const float totalButtonsWidth = minButtonWidth + maxButtonWidth + closeButtonWidth;

        ImGui::SetCursorPosX(m_windowWidth - totalButtonsWidth);

        ImGui::PushStyleColor(ImGuiCol_Button, buttonStyle);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonHoveredStyle);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActiveStyle);

        if (ImGui::Button(minButtonName, ImVec2(minButtonWidth, 0)))
        {
            glfwIconifyWindow(m_window);
        }

        ImGui::SameLine(0, 1.0f);

        if (ImGui::Button(maxButtonName, ImVec2(maxButtonWidth, 0))) 
        {
            if (glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED)) 
            {
                glfwRestoreWindow(m_window);
            }
            else 
            {
                glfwMaximizeWindow(m_window);
            }
        }

        ImGui::SameLine(0, 1.0f);

        if (ImGui::Button(closeButtonName, ImVec2(closeButtonWidth, 0))) 
        {
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        }

        ImGui::PopStyleColor(3);

        ImGui::EndMainMenuBar();
    }

    ImGui::PopStyleVar();

    return ImGui::GetFrameHeight();
}

void NeuromeGUI::checkDragging()
{
    const ImVec2 menuBarMin = ImGui::GetCursorScreenPos();
    const ImVec2 menuBarMax(menuBarMin.x + m_windowWidth, menuBarMin.y + ImGui::GetFrameHeight());

    if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(menuBarMin, menuBarMax))
    {
        m_isDragging = true;
        glfwGetCursorPos(m_window, &m_dragStartX, &m_dragStartY);
    }

    if (m_isDragging)
    {
        double currentX, currentY;
        glfwGetCursorPos(m_window, &currentX, &currentY);

        if (ImGui::IsMouseDown(0))
        {
            int windowX, windowY;
            glfwGetWindowPos(m_window, &windowX, &windowY);

            const int newX = windowX + (currentX - m_dragStartX);
            const int newY = windowY + (currentY - m_dragStartY);

            glfwSetWindowPos(m_window, newX, newY);
        }
        else
        {
            m_isDragging = false;
        }
    }
}

void NeuromeGUI::checkResize()
{
    GLFWcursor *standardCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    GLFWcursor *horizontalResizeCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    GLFWcursor *verticalResizeCursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    GLFWcursor *diagonalResizeCursor1 = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    GLFWcursor *diagonalResizeCursor2 = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);

    double mouseX, mouseY;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);

    int windowX, windowY, windowWidth, windowHeight;
    glfwGetWindowPos(m_window, &windowX, &windowY);
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);

    const int RESIZE_BORDER = 5; // Размер области для захвата

    bool onLeft = mouseX >= windowX && mouseX <= windowX + RESIZE_BORDER;
    bool onRight = mouseX >= windowX + windowWidth - RESIZE_BORDER && mouseX <= windowX + windowWidth;
    bool onTop = mouseY >= windowY && mouseY <= windowY + RESIZE_BORDER;
    bool onBottom = mouseY >= windowY + windowHeight - RESIZE_BORDER && mouseY <= windowY + windowHeight;

    // Определяем край для изменения размера
    if (onLeft && onTop) m_resizeEdge = 5;
    else if (onRight && onTop) m_resizeEdge = 6;
    else if (onLeft && onBottom) m_resizeEdge = 7;
    else if (onRight && onBottom) m_resizeEdge = 8;
    else if (onLeft) m_resizeEdge = 1;
    else if (onRight) m_resizeEdge = 2;
    else if (onTop) m_resizeEdge = 3;
    else if (onBottom) m_resizeEdge = 4;
    else m_resizeEdge = 0;

    // Меняем курсор в зависимости от края
    if (m_resizeEdge == 1 || m_resizeEdge == 2)
        glfwSetCursor(m_window, horizontalResizeCursor);
    else if (m_resizeEdge == 3 || m_resizeEdge == 4)
        glfwSetCursor(m_window, verticalResizeCursor);
    else if (m_resizeEdge == 5 || m_resizeEdge == 8)
        glfwSetCursor(m_window, diagonalResizeCursor1);
    else if (m_resizeEdge == 6 || m_resizeEdge == 7)
        glfwSetCursor(m_window, diagonalResizeCursor2);
    else
        glfwSetCursor(m_window, standardCursor);
}

void NeuromeGUI::resizeWindow()
{
    if (ImGui::IsMouseClicked(0) && m_resizeEdge != 0)
    {
        m_isResizing = true;
        glfwGetCursorPos(m_window, &m_dragStartX, &m_dragStartY);
    }

    if (m_isResizing)
    {
        double currentX, currentY;
        glfwGetCursorPos(m_window, &currentX, &currentY);

        if (ImGui::IsMouseDown(0))
        {
            int windowX, windowY, windowWidth, windowHeight;
            glfwGetWindowPos(m_window, &windowX, &windowY);
            glfwGetWindowSize(m_window, &windowWidth, &windowHeight);

            int deltaX = currentX - m_dragStartX;
            int deltaY = currentY - m_dragStartY;

            // Обновляем размеры и позицию в зависимости от края
            switch (m_resizeEdge)
            {
            case 1: // Левый край
                glfwSetWindowPos(m_window, windowX + deltaX, windowY);
                glfwSetWindowSize(m_window, windowWidth - deltaX, windowHeight);
                break;
            case 2: // Правый край
                glfwSetWindowSize(m_window, windowWidth + deltaX, windowHeight);
                break;
            case 3: // Верхний край
                glfwSetWindowPos(m_window, windowX, windowY + deltaY);
                glfwSetWindowSize(m_window, windowWidth, windowHeight - deltaY);
                break;
            case 4: // Нижний край
                glfwSetWindowSize(m_window, windowWidth, windowHeight + deltaY);
                break;
            case 5: // Левый-верхний угол
                glfwSetWindowPos(m_window, windowX + deltaX, windowY + deltaY);
                glfwSetWindowSize(m_window, windowWidth - deltaX, windowHeight - deltaY);
                break;
            case 6: // Правый-верхний угол
                glfwSetWindowPos(m_window, windowX, windowY + deltaY);
                glfwSetWindowSize(m_window, windowWidth + deltaX, windowHeight - deltaY);
                break;
            case 7: // Левый-нижний угол
                glfwSetWindowPos(m_window, windowX + deltaX, windowY);
                glfwSetWindowSize(m_window, windowWidth - deltaX, windowHeight + deltaY);
                break;
            case 8: // Правый-нижний угол
                glfwSetWindowSize(m_window, windowWidth + deltaX, windowHeight + deltaY);
                break;
            }

            m_dragStartX = currentX;
            m_dragStartY = currentY;
        }
        else
        {
            m_isResizing = false;
        }
    }
}

void NeuromeGUI::beginSettings(float menuBarHeight, ImGuiWindowFlags windowFlags)
{
    ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(m_windowWidth * m_settingsWidth, m_windowHeight - menuBarHeight));

    ImGui::Begin("Region 1", nullptr, windowFlags);
    ImGui::Text("Region 1 Content");

    const ImVec2 region1Size = ImGui::GetWindowSize();
    m_settingsWidth = region1Size.x / m_windowWidth;
    ImGui::End();
}

void NeuromeGUI::beginCanvas(float menuBarHeight, ImGuiWindowFlags windowFlags)
{
    ImGui::SetNextWindowPos(ImVec2(m_windowWidth * m_settingsWidth, menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(m_windowWidth * (1.0f - m_settingsWidth), (m_windowHeight - menuBarHeight) * m_canvasHeight));
    
    ImGui::Begin("Region 2", nullptr, windowFlags);
    ImGui::Text("Region 2 Content");

    const ImVec2 region2Size = ImGui::GetWindowSize();
    m_canvasHeight = region2Size.y / (m_windowHeight - menuBarHeight);
    ImGui::End();
}

void NeuromeGUI::beginManage(float menuBarHeight, ImGuiWindowFlags windowFlags)
{
    const float remainingWidth = 1.0f - m_settingsWidth;
    const float remainingHeight = 1.0f - m_canvasHeight;

    ImGui::SetNextWindowPos(ImVec2(m_windowWidth * m_settingsWidth, menuBarHeight + (m_windowHeight - menuBarHeight) * m_canvasHeight));
    ImGui::SetNextWindowSize(ImVec2(m_windowWidth * remainingWidth * m_manageWidth, (m_windowHeight - menuBarHeight) * remainingHeight));
    
    ImGui::Begin("Region 3", nullptr, windowFlags);
    ImGui::Text("Region 3 Content");

    const ImVec2 region3Size = ImGui::GetWindowSize();
    m_manageWidth = (region3Size.x / m_windowWidth) / remainingWidth;
    ImGui::End();

}

void NeuromeGUI::beginInfo(float menuBarHeight, ImGuiWindowFlags windowFlags)
{
    const float remainingWidth = 1.0f - m_settingsWidth;
    const float remainingHeight = 1.0f - m_canvasHeight;

    ImGui::SetNextWindowPos(ImVec2(m_windowWidth * (m_settingsWidth + remainingWidth * m_manageWidth), menuBarHeight + (m_windowHeight - menuBarHeight) * m_canvasHeight));
    ImGui::SetNextWindowSize(ImVec2(m_windowWidth * remainingWidth * (1.0f - m_manageWidth), (m_windowHeight - menuBarHeight) * remainingHeight));
    
    ImGui::Begin("Region 4", nullptr, windowFlags);
    ImGui::Text("Region 4 Content");
    ImGui::End();
}

void NeuromeGUI::setInitialWindowPos() const
{
    const float offset = 0.1f;

    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);

    const int x = static_cast<int>(mode->width * offset);
    const int y = static_cast<int>(mode->height * offset);

    glfwSetWindowPos(m_window, x, y);
}

void NeuromeGUI::glfwErrorCallback(int error, const char *description)
{
    char buffer[MAX_STR];
    snprintf(buffer, sizeof(buffer), "GLFW Error %d: %s", error, description);

    throw std::runtime_error(buffer);
}
