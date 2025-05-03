#include "NeuromeGUI.h"

int main(int argc, const char *argv[])
{
    if (argc != 1)
    {
        Messenger::error("Command line arguments are not supported");
        return 1;
    }

    if (ProcessHandler::requestAdmin() == 1)
    {
        return 0;
    }

    NeuromeGUI app;
    try
    {
        app.render();
    }
    catch (const std::exception &exp)
    {
        Messenger::error(exp.what());
        return 1;
    }

    return 0;
}



//#include <GLFW/glfw3.h>
//#include <iostream>
//#include <algorithm>
//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
//
//class Window {
//public:
//    enum class ResizeDirection {
//        NONE,
//        LEFT, RIGHT, TOP, BOTTOM,
//        TOP_LEFT, TOP_RIGHT,
//        BOTTOM_LEFT, BOTTOM_RIGHT
//    };
//
//private:
//    GLFWwindow *window;
//    const int VISUAL_BORDER = 2;
//    const int RESIZE_HITBOX = 8;
//    const int MIN_SIZE = 50;
//
//    ResizeDirection resizeDir = ResizeDirection::NONE;
//    bool isResizing = false;
//    int windowStartX, windowStartY;
//    int windowStartWidth, windowStartHeight;
//    double dragStartScreenX, dragStartScreenY;
//
//    // Курсоры
//    GLFWcursor *hResizeCursor;
//    GLFWcursor *vResizeCursor;
//    GLFWcursor *nwseResizeCursor;
//    GLFWcursor *neswResizeCursor;
//
//    void drawBorder(int width, int height) {
//        glColor3f(0.5f, 0.5f, 0.5f);
//        glBegin(GL_QUADS);
//        // Top border
//        glVertex2i(0, 0);
//        glVertex2i(width, 0);
//        glVertex2i(width, VISUAL_BORDER);
//        glVertex2i(0, VISUAL_BORDER);
//        // Bottom border
//        glVertex2i(0, height - VISUAL_BORDER);
//        glVertex2i(width, height - VISUAL_BORDER);
//        glVertex2i(width, height);
//        glVertex2i(0, height);
//        // Left border
//        glVertex2i(0, 0);
//        glVertex2i(VISUAL_BORDER, 0);
//        glVertex2i(VISUAL_BORDER, height);
//        glVertex2i(0, height);
//        // Right border
//        glVertex2i(width - VISUAL_BORDER, 0);
//        glVertex2i(width, 0);
//        glVertex2i(width, height);
//        glVertex2i(width - VISUAL_BORDER, height);
//        glEnd();
//    }
//
//    void handleResize() {
//        if (!isResizing) {
//            updateResizeDirection();
//            return;
//        }
//
//        updateCursor();
//
//        double currentScreenX, currentScreenY;
//        glfwGetCursorPos(window, &currentScreenX, &currentScreenY);
//        int winX, winY;
//        glfwGetWindowPos(window, &winX, &winY);
//        currentScreenX += winX;
//        currentScreenY += winY;
//
//        int newWidth = windowStartWidth;
//        int newHeight = windowStartHeight;
//        int newX = windowStartX;
//        int newY = windowStartY;
//
//        calculateNewDimensions(currentScreenX, currentScreenY, newWidth, newHeight, newX, newY);
//        applyWindowConstraints(newWidth, newHeight, newX, newY);
//    }
//
//    void updateResizeDirection() {
//        double mouseX, mouseY;
//        glfwGetCursorPos(window, &mouseX, &mouseY);
//        int width, height;
//        glfwGetWindowSize(window, &width, &height);
//
//        resizeDir = ResizeDirection::NONE;
//
//        if (mouseX < RESIZE_HITBOX) {
//            if (mouseY < RESIZE_HITBOX) resizeDir = ResizeDirection::TOP_LEFT;
//            else if (mouseY > height - RESIZE_HITBOX) resizeDir = ResizeDirection::BOTTOM_LEFT;
//            else resizeDir = ResizeDirection::LEFT;
//        }
//        else if (mouseX > width - RESIZE_HITBOX) {
//            if (mouseY < RESIZE_HITBOX) resizeDir = ResizeDirection::TOP_RIGHT;
//            else if (mouseY > height - RESIZE_HITBOX) resizeDir = ResizeDirection::BOTTOM_RIGHT;
//            else resizeDir = ResizeDirection::RIGHT;
//        }
//        else if (mouseY < RESIZE_HITBOX) {
//            resizeDir = ResizeDirection::TOP;
//        }
//        else if (mouseY > height - RESIZE_HITBOX) {
//            resizeDir = ResizeDirection::BOTTOM;
//        }
//
//        updateCursor();
//    }
//
//    void updateCursor() {
//        GLFWcursor *cursor = nullptr;
//        switch (resizeDir) {
//        case ResizeDirection::LEFT:
//        case ResizeDirection::RIGHT:
//            cursor = hResizeCursor;
//            break;
//        case ResizeDirection::TOP:
//        case ResizeDirection::BOTTOM:
//            cursor = vResizeCursor;
//            break;
//        case ResizeDirection::TOP_LEFT:
//        case ResizeDirection::BOTTOM_RIGHT:
//            cursor = nwseResizeCursor;
//            break;
//        case ResizeDirection::TOP_RIGHT:
//        case ResizeDirection::BOTTOM_LEFT:
//            cursor = neswResizeCursor;
//            break;
//        default:
//            cursor = nullptr;
//        }
//        glfwSetCursor(window, cursor);
//    }
//
//    void calculateNewDimensions(double currentScreenX, double currentScreenY,
//        int &newWidth, int &newHeight, int &newX, int &newY) {
//        switch (resizeDir) {
//        case ResizeDirection::LEFT:
//            newWidth = windowStartX + windowStartWidth - currentScreenX;
//            newX = currentScreenX;
//            break;
//        case ResizeDirection::RIGHT:
//            newWidth = currentScreenX - windowStartX;
//            break;
//        case ResizeDirection::TOP:
//            newHeight = windowStartY + windowStartHeight - currentScreenY;
//            newY = currentScreenY;
//            break;
//        case ResizeDirection::BOTTOM:
//            newHeight = currentScreenY - windowStartY;
//            break;
//        case ResizeDirection::TOP_LEFT:
//            newWidth = windowStartX + windowStartWidth - currentScreenX;
//            newHeight = windowStartY + windowStartHeight - currentScreenY;
//            newX = currentScreenX;
//            newY = currentScreenY;
//            break;
//        case ResizeDirection::TOP_RIGHT:
//            newWidth = currentScreenX - windowStartX;
//            newHeight = windowStartY + windowStartHeight - currentScreenY;
//            newY = currentScreenY;
//            break;
//        case ResizeDirection::BOTTOM_LEFT:
//            newWidth = windowStartX + windowStartWidth - currentScreenX;
//            newHeight = currentScreenY - windowStartY;
//            newX = currentScreenX;
//            break;
//        case ResizeDirection::BOTTOM_RIGHT:
//            newWidth = currentScreenX - windowStartX;
//            newHeight = currentScreenY - windowStartY;
//            break;
//        default:
//            break;
//        }
//    }
//
//    void applyWindowConstraints(int &newWidth, int &newHeight, int &newX, int &newY) {
//        newWidth = std::max(newWidth, MIN_SIZE);
//        newHeight = std::max(newHeight, MIN_SIZE);
//
//        switch (resizeDir) {
//        case ResizeDirection::LEFT:
//        case ResizeDirection::TOP_LEFT:
//        case ResizeDirection::BOTTOM_LEFT:
//            newX = windowStartX + windowStartWidth - newWidth;
//            break;
//        default:
//            break;
//        }
//
//        switch (resizeDir) {
//        case ResizeDirection::TOP:
//        case ResizeDirection::TOP_RIGHT:
//        case ResizeDirection::TOP_LEFT:
//            newY = windowStartY + windowStartHeight - newHeight;
//            break;
//        default:
//            break;
//        }
//
//        glfwSetWindowSize(window, newWidth, newHeight);
//        glfwSetWindowPos(window, newX, newY);
//    }
//
//public:
//    Window(int width = 800, int height = 600, const char *title = "Resizable Window") {
//        if (!glfwInit()) {
//            throw std::runtime_error("Failed to initialize GLFW");
//        }
//
//        // Создаем курсоры
//        hResizeCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
//        vResizeCursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
//        nwseResizeCursor = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
//        neswResizeCursor = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
//
//        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
//        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
//        if (!window) {
//            glfwTerminate();
//            throw std::runtime_error("Failed to create GLFW window");
//        }
//
//        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
//        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
//        glfwSetWindowPos(window, (mode->width - width) / 2, (mode->height - height) / 2);
//        glfwSetWindowSizeLimits(window, MIN_SIZE, MIN_SIZE, GLFW_DONT_CARE, GLFW_DONT_CARE);
//
//        glfwMakeContextCurrent(window);
//        glfwSwapInterval(1);
//
//        // Setup Dear ImGui
//        IMGUI_CHECKVERSION();
//        ImGui::CreateContext();
//        ImGui_ImplGlfw_InitForOpenGL(window, true);
//        ImGui_ImplOpenGL3_Init("#version 130");
//        ImGui::StyleColorsDark();
//
//        glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int32_t width, int32_t height)
//            {
//            glViewport(0, 0, width, height);
//            });
//    }
//
//    ~Window() {
//        ImGui_ImplOpenGL3_Shutdown();
//        ImGui_ImplGlfw_Shutdown();
//        ImGui::DestroyContext();
//
//        // Уничтожаем курсоры
//        glfwDestroyCursor(hResizeCursor);
//        glfwDestroyCursor(vResizeCursor);
//        glfwDestroyCursor(nwseResizeCursor);
//        glfwDestroyCursor(neswResizeCursor);
//
//        glfwDestroyWindow(window);
//        glfwTerminate();
//    }
//
//    bool shouldClose() const {
//        return glfwWindowShouldClose(window);
//    }
//
//    void beginFrame() {
//        glfwPollEvents();
//
//        ImGui_ImplOpenGL3_NewFrame();
//        ImGui_ImplGlfw_NewFrame();
//        ImGui::NewFrame();
//
//        bool isMouseOverImGui = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
//
//        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
//            if (!isResizing && resizeDir != ResizeDirection::NONE && !isMouseOverImGui) {
//                isResizing = true;
//                glfwGetCursorPos(window, &dragStartScreenX, &dragStartScreenY);
//                int winX, winY;
//                glfwGetWindowPos(window, &winX, &winY);
//                dragStartScreenX += winX;
//                dragStartScreenY += winY;
//                glfwGetWindowPos(window, &windowStartX, &windowStartY);
//                glfwGetWindowSize(window, &windowStartWidth, &windowStartHeight);
//            }
//        }
//        else {
//            isResizing = false;
//        }
//
//        handleResize();
//    }
//
//    void endFrame() {
//        ImGui::Render();
//
//        int width, height;
//        glfwGetFramebufferSize(window, &width, &height);
//        glViewport(0, 0, width, height);
//        glClear(GL_COLOR_BUFFER_BIT);
//
//        glMatrixMode(GL_PROJECTION);
//        glLoadIdentity();
//        glOrtho(0, width, height, 0, -1, 1);
//
//        drawBorder(width, height);
//
//        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//
//        // Обновляем курсор после ImGui
//        updateCursor();
//
//        glfwSwapBuffers(window);
//    }
//
//    GLFWwindow *getWindow() const {
//        return window;
//    }
//};
//
//int main() {
//    try {
//        Window window(800, 600, "ImGui Resizable Window");
//
//        while (!window.shouldClose()) {
//            window.beginFrame();
//
//            ImGui::Begin("Demo Window");
//            ImGui::Text("Hello, ImGui!");
//            if (ImGui::Button("Close Window")) {
//                glfwSetWindowShouldClose(window.getWindow(), GLFW_TRUE);
//            }
//            ImGui::End();
//
//            window.endFrame();
//        }
//    }
//    catch (const std::exception &e) {
//        std::cerr << "Error: " << e.what() << std::endl;
//        return -1;
//    }
//
//    return 0;
//}