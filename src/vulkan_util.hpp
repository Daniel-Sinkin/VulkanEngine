#include <cstring>
#include <format>
#include <print>
#include <vector>

#include <vulkan/vulkan.h>

using std::println, std::print;

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
} // namespace std

namespace DS {
namespace Vulkan {
using VulkanExtension = const char *;
using VulkanValidationLayer = const char *;

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

namespace Strings {
const char *vkCreateDebugReportCallbackEXT = "vkCreateDebugReportCallbackEXT";
const char *vkDestroyDebugReportCallbackEXT = "vkDestroyDebugReportCallbackEXT";
} // namespace Strings
} // namespace Vulkan
} // namespace DS
