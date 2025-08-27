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
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.0f);

    bool is_running = true;
    while (is_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                println("[   SDL] Info: Got SDL_EVENT_QUIT event");
                is_running = false;
            }
            SDL_WindowID window_id = SDL_GetWindowID(g_Window);
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == window_id) {
                println(
                    "[   SDL] Info: Get SDL_EVENT_WINDOW_CLOSE_REQUESTED on current window ({})",
                    window_id);
                is_running = false;
            }
        }
        if (SDL_GetWindowFlags(g_Window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }
        int fb_width, fb_height;
        SDL_GetWindowSize(g_Window, &fb_width, &fb_height);
        bool positive_size = (fb_width > 0) && (fb_height > 0);
        bool window_wrong_size = g_WD->Width != fb_width || g_WD->Height != fb_height;
        if (positive_size && (g_SwapChainRebuild || window_wrong_size)) {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(
                g_Instance,
                g_PhysicalDevice,
                g_Device,
                &g_MainWindowData,
                g_QueueFamily,
                g_Allocator,
                fb_width,
                fb_height,
                g_MinImageCount);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, Window!");

            ImGui::Text("This is some useful text.");
            ImGui::Checkbox("Demo Window", &show_demo_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float *)&clear_color);

            if (ImGui::Button("Button")) counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / g_IO.Framerate, g_IO.Framerate);
            ImGui::End();
        }

        ImGui::Render();
        ImDrawData *draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        if (!is_minimized) {
            g_MainWindowData.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            g_MainWindowData.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            g_MainWindowData.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            g_MainWindowData.ClearValue.color.float32[3] = clear_color.w;
            Engine::FrameRender(&g_MainWindowData, draw_data);
            Engine::FramePresent(&g_MainWindowData);
        }
    }
    Engine::cleanup();
}