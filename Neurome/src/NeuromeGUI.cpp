#include "NeuromeGUI.h"

NeuromeGUI::NeuromeGUI() noexcept
    : m_windowWidth(1280), m_windowHeight(720),
    m_window(nullptr),
    m_ewResizeCursor(nullptr), m_nsResizeCursor(nullptr),
    m_nwseResizeCursor(nullptr), m_neswResizeCursor(nullptr),
    m_isInitialized(false),
    m_isResizing(false), m_resizeDirection(Direction::None),
    m_windowStartX(0), m_windowStartY(0),
    m_windowStartWidth(0), m_windowStartHeight(0),
    m_isDragging(false),
    m_windowX(0), m_windowY(0),
    m_dragStartX(0.0), m_dragStartY(0.0)

    //m_settingsWidth(0.25f), m_canvasHeight(0.7f), m_manageWidth(0.5f),
    //m_isChangedSettings(false), m_isAttachProcess(false), m_isCaptureWindow(false)
{
    setlocale(LC_ALL, "");

    if (!glfwInit())
    {
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    m_window.reset(glfwCreateWindow(m_windowWidth, m_windowHeight, env::windowTitle, nullptr, nullptr));
    if (!m_window)
    {
        return;
    }

    m_ewResizeCursor.reset(glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR));
    m_nsResizeCursor.reset(glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
    m_nwseResizeCursor.reset(glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR));
    m_neswResizeCursor.reset(glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR));

    glfwMakeContextCurrent(m_window.get());
    glfwSwapInterval(env::vsync);

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(m_window.get(), true);
    ImGui_ImplOpenGL3_Init(env::glslVersion);

    glfwSetErrorCallback(glfwErrorCallback);
    glfwSetFramebufferSizeCallback(m_window.get(), glfwFramebufferSizeCallback);

    setColorStyle();
    setInitialWindowPos();

    m_isInitialized = true;
}

NeuromeGUI::~NeuromeGUI()
{
    if (m_isInitialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        m_ewResizeCursor.reset();
        m_nsResizeCursor.reset();
        m_nwseResizeCursor.reset();
        m_neswResizeCursor.reset();

        m_window.reset();

        glfwTerminate();
    }
}

void NeuromeGUI::render()
{
    if (!m_isInitialized)
    {
        throw std::runtime_error("Failed to initialize the graphical user interface");
    }

    const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoCollapse |      
                                         ImGuiWindowFlags_NoBringToFrontOnFocus;

    while (!glfwWindowShouldClose(m_window.get()))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(m_window.get(), GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(env::glfwSleepMs);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        handleResize();

        beginMenuBar();

        //beginSettings(menuBarHeight, windowFlags);
        //beginCanvas(menuBarHeight, windowFlags);
        //beginManage(menuBarHeight, windowFlags);
        //beginInfo(menuBarHeight, windowFlags);

        update();
    }
}

void NeuromeGUI::update()
{
    ImGui::Render();

    glfwGetFramebufferSize(m_window.get(), &m_windowWidth, &m_windowHeight);
    glViewport(0, 0, m_windowWidth, m_windowHeight);

    glClearColor(env::clearColorR * env::clearColorA,
                 env::clearColorG * env::clearColorA,
                 env::clearColorB * env::clearColorA,
                 env::clearColorA);

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_windowWidth, m_windowHeight, 0, -1, 1);

    drawBorder();
    updateCursor();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window.get());
}

void NeuromeGUI::setInitialWindowPos() const
{
    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);

    const int32_t x = static_cast<int32_t>(mode->width * env::initialWindowOffset);
    const int32_t y = static_cast<int32_t>(mode->height * env::initialWindowOffset);

    glfwSetWindowPos(m_window.get(), x, y);
}

void NeuromeGUI::setColorStyle() const
{
    ImFontConfig config;
    config.OversampleH = env::oversampleH;
    config.OversampleV = env::oversampleV;
    config.PixelSnapH = env::pixelSnapH;

    const ImWchar ranges[] = {
        GlyphRange::BasicLatinStart, GlyphRange::Latin1SupplementEnd,
        GlyphRange::CyrillicStart, GlyphRange::CyrillicSupplementEnd,
        GlyphRange::None
    };

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(env::fontFile, env::pixelsSize, &config, ranges);
    io.Fonts->Build();

    ImGui::StyleColorsDark();
}

void NeuromeGUI::drawBorder() const
{
    glColor4f(env::borderColorR, env::borderColorG, env::borderColorB, env::borderColorA);
    glBegin(GL_QUADS);

    glVertex2i(0, 0);
    glVertex2i(m_windowWidth, 0);
    glVertex2i(m_windowWidth, env::visualBorderSize);
    glVertex2i(0, env::visualBorderSize);

    glVertex2i(0, m_windowHeight - env::visualBorderSize);
    glVertex2i(m_windowWidth, m_windowHeight - env::visualBorderSize);
    glVertex2i(m_windowWidth, m_windowHeight);
    glVertex2i(0, m_windowHeight);

    glVertex2i(0, 0);
    glVertex2i(env::visualBorderSize, 0);
    glVertex2i(env::visualBorderSize, m_windowHeight);
    glVertex2i(0, m_windowHeight);

    glVertex2i(m_windowWidth - env::visualBorderSize, 0);
    glVertex2i(m_windowWidth, 0);
    glVertex2i(m_windowWidth, m_windowHeight);
    glVertex2i(m_windowWidth - env::visualBorderSize, m_windowHeight);
    glEnd();
}

void NeuromeGUI::handleResize()
{
    if (glfwGetMouseButton(m_window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        const bool isOverGui = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
        if (!m_isResizing && m_resizeDirection != Direction::None && !isOverGui) 
        {
            m_isResizing = true;

            glfwGetWindowPos(m_window.get(), &m_windowStartX, &m_windowStartY);
            glfwGetWindowSize(m_window.get(), &m_windowStartWidth, &m_windowStartHeight);
        }
    }
    else 
    {
        m_isResizing = false;
    }

    if (!m_isResizing) 
    {
        updateResizeDirection();
        return;
    }

    updateCursor();

    double cxpos, cypos;
    glfwGetCursorPos(m_window.get(), &cxpos, &cypos);

    int32_t wxpos, wypos;
    glfwGetWindowPos(m_window.get(), &wxpos, &wypos);

    cxpos += wxpos;
    cypos += wypos;

    int32_t newX = m_windowStartX;
    int32_t newY = m_windowStartY;
    int32_t newWidth = m_windowStartWidth;
    int32_t newHeight = m_windowStartHeight;

    if (getWindowConstraints(cxpos, cypos, &newX, &newY, &newWidth, &newHeight))
    {
        setWindowConstraints(newX, newY, newWidth, newHeight);
    }
}

void NeuromeGUI::updateResizeDirection()
{
    double mx, my;
    glfwGetCursorPos(m_window.get(), &mx, &my);

    m_resizeDirection = Direction::None;
    if (mx < env::resizeHitbox) 
    {
        if (my < env::resizeHitbox)
        {
            m_resizeDirection = Direction::TopLeft;
        }
        else if (my > m_windowHeight - env::resizeHitbox)
        {
            m_resizeDirection = Direction::BottomLeft;
        }
        else
        {
            m_resizeDirection = Direction::Left;
        }
    }
    else if (mx > m_windowWidth - env::resizeHitbox) 
    {
        if (my < env::resizeHitbox)
        {
            m_resizeDirection = Direction::TopRight;
        }
        else if (my > m_windowHeight - env::resizeHitbox)
        {
            m_resizeDirection = Direction::BottomRight;
        }
        else
        {
            m_resizeDirection = Direction::Right;
        }
    }
    else if (my < env::resizeHitbox) 
    {
        m_resizeDirection = Direction::Top;
    }
    else if (my > m_windowHeight - env::resizeHitbox)
    {
        m_resizeDirection = Direction::Bottom;
    }

    updateCursor();
}

void NeuromeGUI::updateCursor()
{
    GLFWcursor *cursor = nullptr;
    switch (m_resizeDirection) 
    {
        case Direction::Left:
        case Direction::Right:
            cursor = m_ewResizeCursor.get();
            break;
        case Direction::Top:
        case Direction::Bottom:
            cursor = m_nsResizeCursor.get();
            break;
        case Direction::TopLeft:
        case Direction::BottomRight:
            cursor = m_nwseResizeCursor.get();
            break;
        case Direction::TopRight:
        case Direction::BottomLeft:
            cursor = m_neswResizeCursor.get();
            break;
        default:
            cursor = nullptr;
    }

    glfwSetCursor(m_window.get(), cursor);
}

bool NeuromeGUI::getWindowConstraints(float x, float y, int32_t *newX, int32_t *newY, int32_t *newWidth, int32_t *newHeight) const
{
    if (!newX || !newY || !newWidth || !newHeight)
    {
        return false;
    }

    switch (m_resizeDirection) 
    {
        case Direction::Left:
            *newWidth = m_windowStartX + m_windowStartWidth - x;
            *newX = x;
            break;
        case Direction::Right:
            *newWidth = x - m_windowStartX;
            break;
        case Direction::Top:
            *newHeight = m_windowStartY + m_windowStartHeight - y;
            *newY = y;
            break;
        case Direction::Bottom:
            *newHeight = y - m_windowStartY;
            break;
        case Direction::TopLeft:
            *newWidth = m_windowStartX + m_windowStartWidth - x;
            *newHeight = m_windowStartY + m_windowStartHeight - y;
            *newX = x;
            *newY = y;
            break;
        case Direction::TopRight:
            *newWidth = x - m_windowStartX;
            *newHeight = m_windowStartY + m_windowStartHeight - y;
            *newY = y;
            break;
        case Direction::BottomLeft:
            *newWidth = m_windowStartX + m_windowStartWidth - x;
            *newHeight = y - m_windowStartY;
            *newX = x;
            break;
        case Direction::BottomRight:
            *newWidth = x - m_windowStartX;
            *newHeight = y - m_windowStartY;
            break;
        default:
            break;
    }

    return true;
}

void NeuromeGUI::setWindowConstraints(int32_t newX, int32_t newY, int32_t newWidth, int32_t newHeight)
{
    m_windowWidth = std::max(newWidth, (int32_t)env::minWindowWidth);
    m_windowHeight = std::max(newHeight, (int32_t)env::minWindowHeight);

    switch (m_resizeDirection)
    {
        case Direction::Left:
        case Direction::TopLeft:
        case Direction::BottomLeft:
            newX = m_windowStartX + m_windowStartWidth - m_windowWidth;
            break;
        default:
            break;
    }

    switch (m_resizeDirection) 
    {
        case Direction::Top:
        case Direction::TopRight:
        case Direction::TopLeft:
            newY = m_windowStartY + m_windowStartHeight - m_windowHeight;
            break;
        default:
            break;
    }

    glfwSetWindowSize(m_window.get(), m_windowWidth, m_windowHeight);
    glfwSetWindowPos(m_window.get(), newX, newY);
}

void NeuromeGUI::handleDragging()
{
    const ImVec2 menuBarMin = ImGui::GetCursorScreenPos();
    const ImVec2 menuBarMax(menuBarMin.x + m_windowWidth, menuBarMin.y + ImGui::GetFrameHeight());

    const bool is_hovered = ImGui::IsWindowHovered();
    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_isDragging = true;

        glfwGetWindowPos(m_window.get(), &m_windowX, &m_windowY);

        double mx, my;
        glfwGetCursorPos(m_window.get(), &mx, &my);

        m_dragStartX = m_windowX + mx;
        m_dragStartY = m_windowY + my;
    }
    else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        m_isDragging = false;
    }

    if (m_isDragging)
    {
        double mx, my;
        glfwGetCursorPos(m_window.get(), &mx, &my);

        int wx, wy;
        glfwGetWindowPos(m_window.get(), &wx, &wy);

        const int32_t dx = wx + mx - m_dragStartX;
        const int32_t dy = wy + my - m_dragStartY;

        glfwSetWindowPos(m_window.get(), m_windowX + dx, m_windowY + dy);
    }
}

float NeuromeGUI::beginMenuBar()
{
    static const ImVec4 windowBgStyle(env::borderColorR, env::borderColorG, env::borderColorB, env::borderColorA);
    static const ImVec4 buttonStyle(env::buttonStyleR, env::buttonStyleG, env::buttonStyleB, env::buttonStyleA);
    static const ImVec4 buttonHoveredStyle(env::buttonHoveredStyleR, env::buttonHoveredStyleG, env::buttonHoveredStyleB, env::buttonHoveredStyleA);
    static const ImVec4 buttonActiveStyle(env::buttonActiveStyleR, env::buttonActiveStyleG, env::buttonActiveStyleB, env::buttonActiveStyleA);

    static const float menuBarHeight = ImGui::GetFrameHeight() + env::resizeHitbox;

    static const float titleWidth = ImGui::CalcTextSize(env::windowTitle).x;

    static const float minButtonWidth = ImGui::CalcTextSize(env::minButtonTitle).x + env::buttonSpacing;
    static const float maxButtonWidth = ImGui::CalcTextSize(env::maxButtonTitle).x + env::buttonSpacing;
    static const float closeButtonWidth = ImGui::CalcTextSize(env::closeButtonTitle).x + env::buttonSpacing;
    static const float totalButtonsWidth = minButtonWidth + maxButtonWidth + closeButtonWidth;

    ImGui::SetNextWindowPos(ImVec2(env::resizeHitbox, env::resizeHitbox));
    ImGui::SetNextWindowSize(ImVec2(m_windowWidth - 2 * env::resizeHitbox, menuBarHeight));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBgStyle);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); 
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("##MenuBar", nullptr, ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove))
    {
        handleDragging();

        ImGui::SetCursorPosY(ImGui::GetFrameHeight() * 0.5f);

        ImGui::SetCursorPosX((m_windowWidth - titleWidth) * 0.5f);
        ImGui::TextUnformatted(env::windowTitle);

        ImGui::PushStyleColor(ImGuiCol_Button, buttonStyle);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonHoveredStyle);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActiveStyle);

        ImGui::SetCursorPos(ImVec2(m_windowWidth - totalButtonsWidth, (menuBarHeight - ImGui::GetFrameHeight()) * 0.5f));

        if (ImGui::Button(env::minButtonTitle, ImVec2(minButtonWidth, menuBarHeight))) 
        {
            glfwIconifyWindow(m_window.get());
        }

        ImGui::SameLine(0, 1.0f);
        if (ImGui::Button(env::maxButtonTitle, ImVec2(maxButtonWidth, menuBarHeight))) 
        {
            if (glfwGetWindowAttrib(m_window.get(), GLFW_MAXIMIZED))
            {
                glfwRestoreWindow(m_window.get());
            }
            else
            {
                glfwMaximizeWindow(m_window.get());
            }
        }

        ImGui::SameLine(0, 1.0f);
        if (ImGui::Button(env::closeButtonTitle, ImVec2(closeButtonWidth, menuBarHeight)))
        {
            glfwSetWindowShouldClose(m_window.get(), GLFW_TRUE);
        }

        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);

    return menuBarHeight + env::resizeHitbox;
}

//void NeuromeGUI::beginSettings(float menuBarHeight, ImGuiWindowFlags windowFlags)
//{
//    ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
//    ImGui::SetNextWindowSize(ImVec2(m_windowWidth * m_settingsWidth, m_windowHeight - menuBarHeight));
//
//    ImGui::Begin("Settings", nullptr, windowFlags);
//
//    const float windowWidth = ImGui::GetWindowWidth();
//    const float buttonsWidth = 100.0f;
//    const float spacing = 10.0f;
//    const float totalWidth = buttonsWidth * 2 + spacing;
//
//    ImGui::SetCursorPosX((windowWidth - totalWidth) * 0.5f);
//
//    if (ImGui::Button("Reset", ImVec2(buttonsWidth, 0.0f)))
//    {
//        initSettings();
//        m_isChangedSettings = true;
//    }
//
//    ImGui::SameLine(0, spacing);
//
//    if (ImGui::Button(m_isChangedSettings ? "Save*" : "Save", ImVec2(buttonsWidth, 0)))
//    {
//        m_settings.save(m_settingsPath);
//        m_isChangedSettings = false;
//    }
//
//    char clientNameBuffer[MAX_STR];
//    strcpy_s(clientNameBuffer, m_settings.clientName.c_str());
//
//    ImGui::Spacing();
//    ImGui::AlignTextToFramePadding();
//    ImGui::Text("client name:");
//    ImGui::SameLine();
//    ImGui::SetNextItemWidth(-1);
//    if (ImGui::InputText("##clientname", clientNameBuffer, sizeof(clientNameBuffer)))
//    {
//        m_settings.clientName = clientNameBuffer;
//        m_isChangedSettings = true;
//    }
//
//    const ImVec2 region1Size = ImGui::GetWindowSize();
//    m_settingsWidth = region1Size.x / m_windowWidth;
//    ImGui::End();
//}
//
//void NeuromeGUI::beginCanvas(float menuBarHeight, ImGuiWindowFlags windowFlags)
//{
//    ImGui::SetNextWindowPos(ImVec2(m_windowWidth * m_settingsWidth, menuBarHeight));
//    ImGui::SetNextWindowSize(ImVec2(m_windowWidth * (1.0f - m_settingsWidth), (m_windowHeight - menuBarHeight) * m_canvasHeight));
//
//    ImGui::Begin("Display", nullptr, windowFlags);
//
//    const ImVec2 region2Size = ImGui::GetWindowSize();
//    m_canvasHeight = region2Size.y / (m_windowHeight - menuBarHeight);
//    ImGui::End();
//}
//
//void NeuromeGUI::beginManage(float menuBarHeight, ImGuiWindowFlags windowFlags)
//{
//    const float remainingWidth = 1.0f - m_settingsWidth;
//    const float remainingHeight = 1.0f - m_canvasHeight;
//
//    ImGui::SetNextWindowPos(ImVec2(m_windowWidth * m_settingsWidth, menuBarHeight + (m_windowHeight - menuBarHeight) * m_canvasHeight));
//    ImGui::SetNextWindowSize(ImVec2(m_windowWidth * remainingWidth * m_manageWidth, (m_windowHeight - menuBarHeight) * remainingHeight));
//
//    ImGui::Begin("Manage", nullptr, windowFlags);
//
//    if (ImGui::Button(m_isAttachProcess ? "Detach" : "Attach"))
//    {
//        m_isAttachProcess != m_isAttachProcess;
//    }
//
//    const ImVec2 region3Size = ImGui::GetWindowSize();
//    m_manageWidth = (region3Size.x / m_windowWidth) / remainingWidth;
//    ImGui::End();
//}
//
//void NeuromeGUI::beginInfo(float menuBarHeight, ImGuiWindowFlags windowFlags)
//{
//    const float remainingWidth = 1.0f - m_settingsWidth;
//    const float remainingHeight = 1.0f - m_canvasHeight;
//
//    ImGui::SetNextWindowPos(ImVec2(m_windowWidth * (m_settingsWidth + remainingWidth * m_manageWidth), menuBarHeight + (m_windowHeight - menuBarHeight) * m_canvasHeight));
//    ImGui::SetNextWindowSize(ImVec2(m_windowWidth * remainingWidth * (1.0f - m_manageWidth), (m_windowHeight - menuBarHeight) * remainingHeight));
//
//    ImGui::Begin("Info", nullptr, windowFlags);
//
//    ImGui::Text("Process: %s", getProcessPath());
//
//    ImGui::End();
//}


//void NeuromeGUI::toggleProcess()
//{
//    if (m_isAttachProcess)
//    {
//        detachProcess();
//
//        m_isAttachProcess = false;
//        return;
//    }
//
//    const int32_t result = attachProcess();
//    if (result == NONE)
//    {
//        Messenger::error("Failed to open process");
//        return;
//    }
//
//    if (result & READ_USER_CONFIG_MASK)
//    {
//        Messenger::error("Failed to read user config");
//        return;
//    }
//
//    if (result & RESTART_MASK)
//    {
//        Messenger::warning("The process is restarting. Wait and try again");
//        return;
//    }
//
//    if (result & BLOCK_TRAFFIC_MASK)
//    {
//        Messenger::warning("Network traffic could not be blocked. Increased risk of threats");
//    }
//
//    m_isAttachProcess = true;
//}

void NeuromeGUI::glfwErrorCallback(int32_t error, const char *description)
{
    char buffer[MAX_STR];
    snprintf(buffer, sizeof(buffer), "GLFW Error %d: %s", error, description);

    throw std::runtime_error(buffer);
}

void NeuromeGUI::glfwFramebufferSizeCallback(GLFWwindow *window, int32_t width, int32_t height)
{
    glViewport(0, 0, width, height);
}

void NeuromeGUI::WindowDeleter_t::operator()(GLFWwindow *window)
{
    if (window)
    {
        glfwDestroyWindow(window);
    }
}

void NeuromeGUI::CursorDeleter_t::operator()(GLFWcursor *cursor)
{
    if (cursor)
    {
        glfwDestroyCursor(cursor);
    }
}