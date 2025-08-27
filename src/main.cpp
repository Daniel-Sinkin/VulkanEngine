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

#include "vulkan_util.hpp"

using std::println, std::print;

using VulkanExtension = const char *;
using VulkanValidationLayer = const char *;
using CString = const char *; // Assumed to be null terminated

using namespace DS;

constexpr bool print_version = true;

VkAllocationCallbacks *g_Allocator = nullptr;
VkInstance g_Instance = VK_NULL_HANDLE;
VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
VkDevice g_Device = VK_NULL_HANDLE;
uint32_t g_QueueFamily = static_cast<uint32_t>(-1);
VkQueue g_Queue = VK_NULL_HANDLE;
VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;

ImGui_ImplVulkanH_Window g_MainWindowData;
uint32_t g_MinImageCount = 2;
bool g_SwapChainRebuild = false;

struct MySDLVersion {
    int maj;
    int min;
    int mic;
};

constexpr MySDLVersion decode_sdl_version(int version_encoded) {
    return {
        .maj = version_encoded / 1000000,
        .min = (version_encoded / 1000) % 1000,
        .mic = version_encoded % 1000};
}

namespace std {
template <>
struct std::formatter<MySDLVersion> : std::formatter<std::string> {
    auto format(const MySDLVersion &v, auto &ctx) const {
        return std::format_to(ctx.out(), "{}.{}.{}", v.maj, v.min, v.mic);
    }
};
} // namespace std

void print_versions() {
    println("[Global] Info: Version Printout");
    MySDLVersion sdl_headers_version{SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION};
    println("[   SDL] Info: \tHeaders Version {}", sdl_headers_version);

    const int _sdl_linked_version = SDL_GetVersion();
    MySDLVersion sdl_linked_version = decode_sdl_version(_sdl_linked_version);
    println("[   SDL] Info: \tLinked Version {}", sdl_linked_version);

    println("[ ImGui] Info: \tVersion {}", IMGUI_VERSION);
    println("[Vulkan] Info: \tVersion {}", VK_HEADER_VERSION);
}

std::vector<VulkanExtension> get_extensions() {
    uint32_t sdl_extensions_count = 0;
    const VulkanExtension *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);
    std::vector<const char *> extensions;
    for (uint32_t n = 0; n < sdl_extensions_count; ++n) {
        extensions.push_back(sdl_extensions[n]);
    }
    return extensions;
}

bool check_extension(const std::vector<VkExtensionProperties> &properties, VulkanExtension extension) {
    for (const auto &p : properties) {
        if (strcmp(p.extensionName, extension) == 0) {
            println("[Vulkan] Info: Extension {} is availiable.", extension);
            return true;
        }
    }
    println("[Vulkan] Warning: Extension {} is not availiable.", extension);
    return false;
}

void setup_vulkan(std::vector<VulkanExtension> extensions) {
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
        VulkanExtension ext = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
        if (check_extension(properties, ext)) extensions.push_back(ext);

        ext = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
        if (check_extension(properties, ext)) {
            extensions.push_back(ext);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
    }

    println("[Vulkan] Info: Enabling validation layers");
    {
        VulkanValidationLayer layers[] = {"VK_LAYER_KHRONOS_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        extensions.push_back("VK_EXT_debug_report"); // TODO: This is deprecated use VK_EXT_debug_utils + VkDebugUtilsMessengerEXT instead
    }

    println("[Vulkan] Info: Creating Vulkan Instance");
    { // Create Vulkan Instance
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        DS::Vulkan::check(vkCreateInstance(&create_info, g_Allocator, &g_Instance));
    }

    println("[Vulkan] Info: Setup the debug report callback");
    { // Setup the debug report callback
        auto f_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(g_Instance, DS::Vulkan::Strings::vkCreateDebugReportCallbackEXT));
        if (!f_vkCreateDebugReportCallbackEXT) {
            print(stderr, "[Vulkan] Error: Failed to setup debug report callback!");
            abort();
        }
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = DS::Vulkan::debug_report;
        debug_report_ci.pUserData = nullptr;
        DS::Vulkan::check(f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport));
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
        std::vector<VulkanExtension> device_extensions;
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
        if (check_extension(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);

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
        DS::Vulkan::check(vkCreateDevice(
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
        for (VkDescriptorPoolSize &pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        DS::Vulkan::check(
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
    VkResult err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        DS::Vulkan::check(err);

    ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
    {
        // Blocking wait
        DS::Vulkan::check(vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX));
        DS::Vulkan::check(vkResetFences(g_Device, 1, &fd->Fence));
    }
    {
        DS::Vulkan::check(vkResetCommandPool(g_Device, fd->CommandPool, 0));
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        DS::Vulkan::check(vkBeginCommandBuffer(fd->CommandBuffer, &info));
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

        DS::Vulkan::check(vkEndCommandBuffer(fd->CommandBuffer));
        DS::Vulkan::check(vkQueueSubmit(g_Queue, 1, &info, fd->Fence));
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
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        DS::Vulkan::check(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
}

int main() {
    if (print_version) print_versions();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        println("[   SDL] Error: SDL_Init(): {}", SDL_GetError());
        return -1;
    }

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    println("[   SDL] Info: main_scale = {}", main_scale);

    SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL3+Vulkan example", static_cast<int>(1280 * main_scale), static_cast<int>(720 * main_scale), window_flags);
    if (!window) {
        println("[   SDL] Error: SDL_CreateWindow(): {}", SDL_GetError());
        return -1;
    }

    std::vector<VulkanExtension> extensions = get_extensions();
    println("[Vulkan] Info: There are {} SDL extensions", extensions.size());
    for (const auto &ext : extensions) {
        println("[Vulkan] Info: \t{}", ext);
    }

    println("[Vulkan] Info: Starting Setup.");
    setup_vulkan(extensions);
    println("[Vulkan] Info: Finished Setup.");

    println("[Vulkan] Info: Creating Window Surfaces");
    VkSurfaceKHR surface;
    if (SDL_Vulkan_CreateSurface(window, g_Instance, g_Allocator, &surface) == 0) {
        println(stderr, "Failed to create Vulkan Surface.");
        abort();
    }
    println("[Vulkan] Info: Creating Framebuffers");
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    ImGui_ImplVulkanH_Window *wd = &g_MainWindowData;

    println("[Vulkan] Info: Starting Window Setup");
    { // Vulkan Window Setup
        setup_vulkan_window(wd, surface, w, h);

        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(window);
        println("[Vulkan] Info: Finished Window Setup");
    } // Vulkan Window Setup

    println("[ IMGUI] Info: Setting up Context");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    println("[ IMGUI] Info: Setting up scaling");
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    println("[Render] Info: Setting up Backends");
    ImGui_ImplSDL3_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.RenderPass = wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = g_Allocator;
    init_info.CheckVkResultFn = DS::Vulkan::check;
    ImGui_ImplVulkan_Init(&init_info);

    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.0f);

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }
        int fb_width, fb_height;
        SDL_GetWindowSize(window, &fb_width, &fb_height);
        if (fb_width > 0 && fb_height > 0 && (g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height)) {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount);
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

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        ImGui::Render();
        ImDrawData *draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        if (!is_minimized) {
            wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            wd->ClearValue.color.float32[3] = clear_color.w;
            FrameRender(wd, draw_data);
            FramePresent(wd);
        }
    }

    DS::Vulkan::check(vkDeviceWaitIdle(g_Device));
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
    SDL_DestroyWindow(window);
    SDL_Quit();
    println("[SDL] Info: FinishedCleanup");
}