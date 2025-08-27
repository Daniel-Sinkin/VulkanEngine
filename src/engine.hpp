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
#ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#define VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME "VK_KHR_portability_enumeration"
#endif
#ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif
#ifndef VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
#define VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR 0x00000001
#endif

#include "global.hpp"
#include "util.hpp"
#include "vulkan_util.hpp"

using std::println, std::print;

using namespace DS;

using Vulkan::Extension;
using Vulkan::ValidationLayer;

namespace DS::Engine {
void setup_vulkan(std::vector<Vulkan::Extension> extensions) {
    VkResult err;

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    uint32_t properties_count;
    std::vector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    Vulkan::check(vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data()));

    println("[Vulkan] Info: Availiable Extensions");
    for (const auto &p : properties)
        println("[Vulkan] Info: \t{}", p);

    println("[Vulkan] Info: Enabling required extensions");
    {
        Extension ext = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
        if (Vulkan::check_extension(properties, ext)) extensions.push_back(ext);

        ext = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
        if (Vulkan::check_extension(properties, ext)) {
            extensions.push_back(ext);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
    }

    println("[Vulkan] Info: Enabling validation layers");
    {
        ValidationLayer layers[] = {Vulkan::Strings::layer_validation};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        // TODO: This is deprecated use VK_EXT_debug_utils + VkDebugUtilsMessengerEXT instead
        extensions.push_back(Vulkan::Strings::extension_debug_report);
    }

    println("[Vulkan] Info: Creating Vulkan Instance");
    { // Create Vulkan Instance
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        Vulkan::check(vkCreateInstance(&create_info, g_Allocator, &g_Instance));
    }

    println("[Vulkan] Info: Setup the debug report callback");
    { // Setup the debug report callback
        auto f_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(g_Instance, Vulkan::Strings::vkCreateDebugReportCallbackEXT));
        if (!f_vkCreateDebugReportCallbackEXT) {
            print(stderr, "[Vulkan] Error: Failed to setup debug report callback!");
            abort();
        }
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = Vulkan::debug_report;
        debug_report_ci.pUserData = nullptr;
        Vulkan::check(f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport));
    }

    println("[Vulkan] Info: Select physical device");
    {
        g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
        if (g_PhysicalDevice == VK_NULL_HANDLE) {
            println(stderr, "[Vulkan] Error: Failed to select physical device!");
            abort();
        }
    }

    println("[Vulkan] Info: Select graphics queue family");
    {
        g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
        if (g_QueueFamily == static_cast<uint32_t>(-1)) {
            println(stderr, "[Vulkan] Error: Failed to select graphics queue family!");
            abort();
        }
    }

    println("[Vulkan] Info: Creating Logical Device (with 1 queue)");
    {
        std::vector<Extension> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        uint32_t properties_count;
        std::vector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(
            g_PhysicalDevice,
            nullptr,
            &properties_count,
            nullptr);
        properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(
            g_PhysicalDevice,
            nullptr,
            &properties_count,
            properties.data());
        Vulkan::Extension ext = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
        if (Vulkan::check_extension(properties, ext)) {
            device_extensions.push_back(ext);
        }

        const float queue_priority[] = {1.0f};
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = (uint32_t)device_extensions.size();
        create_info.ppEnabledExtensionNames = device_extensions.data();
        Vulkan::check(vkCreateDevice(
            g_PhysicalDevice,
            &create_info,
            g_Allocator, &g_Device));
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    }

    println("[Vulkan] Info: Creating Descriptor Pool");
    {
        VkDescriptorPoolSize pool_sizes[] =
            {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
            };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize &pool_size : pool_sizes) {
            pool_info.maxSets += pool_size.descriptorCount;
        }
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        Vulkan::check(
            vkCreateDescriptorPool(
                g_Device,
                &pool_info,
                g_Allocator,
                &g_DescriptorPool));
    }
}

void setup_vulkan_window(ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int width, int height) {
    wd->Surface = surface;

    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        g_PhysicalDevice,
        g_QueueFamily,
        wd->Surface,
        &res);
    if (res != VK_TRUE) {
        println(stderr, "[Vulkan] Error: No WSI support on physical device 0");
        exit(-1);
    }

    const VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        g_PhysicalDevice,
        wd->Surface,
        requestSurfaceImageFormat,
        (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
        requestSurfaceColorSpace);

    // Select Present Mode
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        g_PhysicalDevice,
        wd->Surface,
        &present_modes[0],
        IM_ARRAYSIZE(present_modes));
    print("[Vulkan] Info: Selected PresentMode = {}\n", static_cast<int>(wd->PresentMode));

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        g_Instance,
        g_PhysicalDevice,
        g_Device,
        wd,
        g_QueueFamily,
        g_Allocator,
        width,
        height,
        g_MinImageCount);
}

static void FrameRender(ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data) {
    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, Constants::no_timeout, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);

    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        println(stderr, "[Vulkan] Error: vkAcquireNextImageKHR gave {} ({}). Rebuilding Swapchain and cancelling FrameRender.", err, static_cast<int>(err));
        g_SwapChainRebuild = true;
        return;
    }
    if (err == VK_SUBOPTIMAL_KHR) {
        if (false) { // TODO: Uncomment this once we have swapchains actually implemented
            println(stderr, "[Vulkan] Warning: vkAcquireNextImageKHR gave {} ({}). Rebuilding Swapchain.", err, static_cast<int>(err));
        }
        g_SwapChainRebuild = true;
    } else {
        Vulkan::check(err);
    }

    ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
    {
        Vulkan::check(vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, Constants::no_timeout));
        Vulkan::check(vkResetFences(g_Device, 1, &fd->Fence));
    }
    {
        Vulkan::check(vkResetCommandPool(g_Device, fd->CommandPool, Constants::no_flags));
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Vulkan::check(vkBeginCommandBuffer(fd->CommandBuffer, &info));
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        Vulkan::check(vkEndCommandBuffer(fd->CommandBuffer));
        Vulkan::check(vkQueueSubmit(g_Queue, 1, &info, fd->Fence));
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window *wd) {
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        g_SwapChainRebuild = true;
        return;
    }
    if (err == VK_SUBOPTIMAL_KHR) {
        g_SwapChainRebuild = true;
    } else {
        Vulkan::check(err);
    }
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
}

void setup() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        println("[   SDL] Error: SDL_Init(): {}", SDL_GetError());
        abort();
    }

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    println("[   SDL] Info: main_scale = {}", main_scale);

    g_Window = SDL_CreateWindow(
        "VulkanEngine 2.0",
        static_cast<int>(Constants::window_width * main_scale),
        static_cast<int>(Constants::window_height * main_scale),
        Constants::window_flags);
    if (!g_Window) {
        println("[   SDL] Error: SDL_CreateWindow(): {}", SDL_GetError());
        abort();
    }

    std::vector<Extension> extensions = Vulkan::get_sdl_extensions();
    println("[Vulkan] Info: There are {} SDL extensions", extensions.size());
    for (const auto &ext : extensions) {
        println("[Vulkan] Info: \t{}", ext);
    }

    println("[Vulkan] Info: Starting Setup.");
    setup_vulkan(extensions);
    println("[Vulkan] Info: Finished Setup.");

    println("[Vulkan] Info: Creating Window Surfaces");
    VkSurfaceKHR surface;
    if (SDL_Vulkan_CreateSurface(g_Window, g_Instance, g_Allocator, &surface) == 0) {
        println(stderr, "Failed to create Vulkan Surface.");
        abort();
    }
    println("[Vulkan] Info: Creating Framebuffers");
    int w, h;
    SDL_GetWindowSize(g_Window, &w, &h);
    g_WD = &g_MainWindowData;

    println("[Vulkan] Info: Starting Window Setup");
    { // Vulkan Window Setup
        setup_vulkan_window(g_WD, surface, w, h);

        SDL_SetWindowPosition(g_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(g_Window);
        println("[Vulkan] Info: Finished Window Setup");
    } // Vulkan Window Setup

    println("[ IMGUI] Info: Setting up Context");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    g_IO = ImGui::GetIO();
    g_IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    println("[ IMGUI] Info: Setting up scaling");
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    println("[Render] Info: Setting up Backends");
    ImGui_ImplSDL3_InitForVulkan(g_Window);
    ImGui_ImplVulkan_InitInfo init_info = {
        .ApiVersion = VK_API_VERSION_1_3,
        .Instance = g_Instance,
        .PhysicalDevice = g_PhysicalDevice,
        .Device = g_Device,
        .QueueFamily = g_QueueFamily,
        .Queue = g_Queue,
        .PipelineCache = g_PipelineCache,
        .DescriptorPool = g_DescriptorPool,
        .RenderPass = g_WD->RenderPass,
        .Subpass = 0,
        .MinImageCount = g_MinImageCount,
        .ImageCount = g_WD->ImageCount,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .Allocator = g_Allocator,
        .CheckVkResultFn = Vulkan::check};
    ImGui_ImplVulkan_Init(&init_info);
}

void cleanup() {
    Vulkan::check(vkDeviceWaitIdle(g_Device));
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    println("[Vulkan] Info: Starting cleanup.");
    {
        auto f_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(g_Instance, Vulkan::Strings::vkDestroyDebugReportCallbackEXT));
        if (f_vkDestroyDebugReportCallbackEXT) {
            f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
            g_DebugReport = nullptr;
        }
    }

    println("[Vulkan] Info: Cleaning up vulkan window");
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);

    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);
    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
    println("[Vulkan] Info: Finished cleanup.");

    println("[SDL] Info: Starting Cleanup");
    SDL_DestroyWindow(g_Window);
    SDL_Quit();
    println("[SDL] Info: FinishedCleanup");
}
} // namespace DS::Engine