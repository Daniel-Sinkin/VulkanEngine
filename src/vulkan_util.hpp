#pragma once
#include <cstring>
#include <format>
#include <print>
#include <vector>

#include <vulkan/vulkan.h>

#include "util.hpp"

using std::println, std::print;

using namespace DS;

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

template <>
struct formatter<VkPhysicalDeviceProperties> : formatter<string_view> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <class FormatContext>
    auto format(const VkPhysicalDeviceProperties &prop, FormatContext &ctx) const {
        return format_to(
            ctx.out(),
            "VkPhysicalDeviceProperties {{\n"
            "\tapiVersion: {} ({}.{}.{})\n"
            "\tdriverVersion: {}\n"
            "\tvendorID: {}\n"
            "\tdeviceID: {}\n"
            "\tdeviceType: {}\n"
            "\tdeviceName: {}\n"
            "\tpipelineCacheUUID: {}\n"
            "}}",
            prop.apiVersion,
            VK_VERSION_MAJOR(prop.apiVersion),
            VK_VERSION_MINOR(prop.apiVersion),
            VK_VERSION_PATCH(prop.apiVersion),
            prop.driverVersion,
            prop.vendorID,
            prop.deviceID,
            static_cast<int>(prop.deviceType),
            prop.deviceName,
            Util::uuid_to_string(prop.pipelineCacheUUID));
    }
};
} // namespace std

namespace DS {
namespace Vulkan {
using Extension = const char *;
using ValidationLayer = const char *;

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

void check(VkResult err) {
    if (err == VK_SUCCESS) return;
    println(stderr, "[Vulkan] Error: VkResult = {}", err);
    if (err < 0) abort();
}

std::vector<Extension> get_sdl_extensions() {
    uint32_t sdl_extensions_count = 0;
    const Extension *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);
    std::vector<const char *> extensions;
    for (uint32_t n = 0; n < sdl_extensions_count; ++n) {
        extensions.push_back(sdl_extensions[n]);
    }
    return extensions;
}

bool check_extension(const std::vector<VkExtensionProperties> &properties, Extension extension) {
    for (const auto &p : properties) {
        if (strcmp(p.extensionName, extension) == 0) {
            println("[Vulkan] Info: Extension {} is availiable.", extension);
            return true;
        }
    }
    println("[Vulkan] Warning: Extension {} is not availiable.", extension);
    return false;
}

namespace Strings {
const char *vkCreateDebugReportCallbackEXT = "vkCreateDebugReportCallbackEXT";
const char *vkDestroyDebugReportCallbackEXT = "vkDestroyDebugReportCallbackEXT";

Vulkan::Extension extension_debug_report = "VK_EXT_debug_report";

Vulkan::ValidationLayer layer_validation = "VK_LAYER_KHRONOS_validation";
} // namespace Strings
} // namespace Vulkan
} // namespace DS
