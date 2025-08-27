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

#include "util.hpp"

using namespace DS;

VkAllocationCallbacks *g_Allocator = nullptr;
VkInstance g_Instance = VK_NULL_HANDLE;
VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
VkDevice g_Device = VK_NULL_HANDLE;
uint32_t g_QueueFamily = Constants::queue_familily_not_init;
VkQueue g_Queue = VK_NULL_HANDLE;
VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;

SDL_Window *g_Window = nullptr;

ImGui_ImplVulkanH_Window g_MainWindowData;
ImGui_ImplVulkanH_Window *g_WD = nullptr;
constexpr uint32_t g_MinImageCount = 2;
bool g_SwapChainRebuild = false;

ImGuiIO *g_IO;

bool g_IsRunning = true;

glm::vec4 g_ClearColor{0.45f, 0.55f, 0.60f, 1.0f};