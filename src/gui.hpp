#pragma once
#include <cstring>
#include <format>
#include <print>
#include <vector>

#include <glm/glm.hpp>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_vulkan.h>

namespace DS::GUI {
void debug() {
    ImGui::Begin("Hello, Window!");
    ImGui::ColorEdit3("clear color", (float *)&g_ClearColor);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / g_IO->Framerate, g_IO->Framerate);
    ImGui::End();
}
} // namespace DS::GUI