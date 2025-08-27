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

#include <vulkan/vulkan.h>
#ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#define VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME "VK_KHR_portability_enumeration"
#endif
#ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif
#ifndef VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
#define VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR 0x00000001
#endif

#include "engine.hpp"
#include "global.hpp"
#include "gui.hpp"
#include "io.hpp"
#include "util.hpp"
#include "vulkan_util.hpp"

using std::println, std::print;

using namespace DS;

using Vulkan::Extension;
using Vulkan::ValidationLayer;

constexpr bool print_version = true;

int main() {
    if (print_version) Util::print_versions();

    Engine::setup();

    bool show_demo_window = true;

    while (g_IsRunning) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            IO::handle_event(event);
        }
        if (SDL_GetWindowFlags(g_Window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }
        Engine::recreate_swapchains_if_necessary();

        // Reset Frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();

        // GUI
        ImGui::NewFrame();
        GUI::debug();
        ImGui::Render();

        ImDrawData *draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        if (!is_minimized) {
            g_MainWindowData.ClearValue.color.float32[0] = g_ClearColor.x * g_ClearColor.w;
            g_MainWindowData.ClearValue.color.float32[1] = g_ClearColor.y * g_ClearColor.w;
            g_MainWindowData.ClearValue.color.float32[2] = g_ClearColor.z * g_ClearColor.w;
            g_MainWindowData.ClearValue.color.float32[3] = g_ClearColor.w;
            Engine::FrameRender(&g_MainWindowData, draw_data);
            Engine::FramePresent(&g_MainWindowData);
        }
    }
    Engine::cleanup();
}