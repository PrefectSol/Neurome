#include "Messenger.h"

void Messenger::modalBox(const char *title, const char *content, const ImVec4 &color)
{
    bool open = true;
    ImGui::OpenPopup(title);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(title, &open, ImGuiWindowFlags_AlwaysAutoResize)) 
    {
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s", content);
        ImGui::PopStyleColor();

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) 
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Messenger::error(const char *content)
{
    const char *title = "Error";
    if (!ImGui::GetCurrentContext() || !glfwGetCurrentContext()) 
    {
        MessageBoxA(NULL, content, title, MB_ICONERROR | MB_OK);
        return;
    }

    modalBox(title, content, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
}

void Messenger::warning(const char *content)
{
    const char *title = "Warning";
    if (!ImGui::GetCurrentContext() || !glfwGetCurrentContext())
    {
        MessageBoxA(NULL, content, title, MB_ICONWARNING | MB_OK);
        return;
    }

    modalBox(title, content, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
}

void Messenger::info(const char *content)
{
    const char *title = "Info";
    if (!ImGui::GetCurrentContext() || !glfwGetCurrentContext()) 
    {
        MessageBoxA(NULL, content, title, MB_ICONINFORMATION | MB_OK);
        return;
    }

    modalBox(title, content, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
}