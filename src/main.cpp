#include <cstring>
#include <format>
#include <print>
#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

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

using std::println, std::print;

using VulkanExtension = const char *;
using VulkanValidationLayer = const char *;
using CString = const char *;

namespace DS::Strings {
CString vkCreateDebugReportCallbackEXT = "vkCreateDebugReportCallbackEXT";
CString vkDestroyDebugReportCallbackEXT = "vkDestroyDebugReportCallbackEXT";
} // namespace DS::Strings

constexpr bool print_version = true;

VkAllocationCallbacks *g_Allocator = nullptr;
VkInstance g_Instance = VK_NULL_HANDLE;
VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
uint32_t g_QueueFamily = static_cast<uint32_t>(-1);
VkQueue g_Queue = VK_NULL_HANDLE;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char *pLayerPrefix,
    const char *pMessage,
    void *pUserData) {
    (void)flags;
    (void)object;
    (void)location;
    (void)messageCode;
    (void)pUserData;
    (void)pLayerPrefix; // Unused arguments
    println(stderr, "[Vulkan] Debug: ObjectType: {}\nMessage: {}\n", static_cast<int>(objectType), pMessage);
    return VK_FALSE;
}

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

constexpr std::string_view vk_result_to_string(VkResult r) {
    switch (r) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    default:
        return "Not supported, see https://registry.khronos.org/vulkan/specs/latest/man/html/VkResult.html";
    }
}

namespace std {
template <>
struct std::formatter<MySDLVersion> : std::formatter<std::string> {
    auto format(const MySDLVersion &v, auto &ctx) const {
        return std::format_to(ctx.out(), "{}.{}.{}", v.maj, v.min, v.mic);
    }
};

template <>
struct formatter<VkResult> : formatter<string_view> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <class FormatContext>
    auto format(VkResult r, FormatContext &ctx) const {
        return format_to(ctx.out(), "{} ({})", vk_result_to_string(r), static_cast<int>(r));
    }
};

template <>
struct formatter<VkExtensionProperties> : formatter<std::string_view> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <class FormatContext>
    auto format(VkExtensionProperties ext_prop, FormatContext &ctx) const {
        return format_to(ctx.out(), "{} ({})",
            ext_prop.extensionName,
            ext_prop.specVersion);
    }
};
} // namespace std

void check_vk_result(VkResult err) {
    if (err == VK_SUCCESS) return;
    println(stderr, "[Vulkan] Error: VkResult = {}", err);
    if (err < 0) abort();
}

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
    check_vk_result(vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data()));

    println("[Vulkan] Info: Availiable Extensions:");
    for (const auto &p : properties)
        println("[Vulkan] Info: \t{}", p);

    { // Enable required extensions
        VulkanExtension ext = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
        if (check_extension(properties, ext)) extensions.push_back(ext);

        ext = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
        if (check_extension(properties, ext)) {
            extensions.push_back(ext);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
    }

    { // Enable Validation Layers
        VulkanValidationLayer layers[] = {"VK_LAYER_KHRONOS_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        extensions.push_back("VK_EXT_debug_report"); // TODO: This is deprecated use VK_EXT_debug_utils + VkDebugUtilsMessengerEXT instead
    }

    { // Create Vulkan Instance
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        check_vk_result(vkCreateInstance(&create_info, g_Allocator, &g_Instance));
    }

    { // Setup the debug report callback
        auto f_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(g_Instance, DS::Strings::vkCreateDebugReportCallbackEXT));
        if (!f_vkCreateDebugReportCallbackEXT) {
            print(stderr, "[Vulkan] Error: Failed to setup debug report callback!");
            abort();
        }
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        check_vk_result(f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport));
    }

    { // Select Physical Device
        g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
        if (g_PhysicalDevice == VK_NULL_HANDLE) {
            println(stderr, "[Vulkan] Error: Failed to select physical device!");
            abort();
        }
    }

    { // Select graphics queue family
        g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
        if (g_QueueFamily == static_cast<uint32_t>(-1)) {
            println(stderr, "[Vulkan] Error: Failed to select graphics queue family!");
            abort();
        }
    }
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

    setup_vulkan(extensions);

    { // Cleanup the debug report callback
        auto f_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(g_Instance, DS::Strings::vkDestroyDebugReportCallbackEXT));
        if (f_vkDestroyDebugReportCallbackEXT) {
            f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
            g_DebugReport = nullptr;
        }
    }

    vkDestroyInstance(g_Instance, g_Allocator);

    SDL_DestroyWindow(window);
    SDL_Quit();
}