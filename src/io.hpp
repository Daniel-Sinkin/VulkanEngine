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

#include <vulkan/vulkan.h>

namespace DS::IO {
void handle_event(SDL_Event &event) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_QUIT) {
        println("[   SDL] Info: Got SDL_EVENT_QUIT event");
        g_IsRunning = false;
    }
    SDL_WindowID window_id = SDL_GetWindowID(g_Window);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == window_id) {
        println(
            "[   SDL] Info: Get SDL_EVENT_WINDOW_CLOSE_REQUESTED on current window ({})",
            window_id);
        g_IsRunning = false;
    }

    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
        case SDLK_ESCAPE:
            println("[   SDL] Info: ESC pressed, closing window");
            g_IsRunning = false;
            break;
        default:
            // println("[   SDL] Info: Unknown key (keycode={}) pressed", event.key.key);
            break;
        }
    }
}
} // namespace DS::IO