#include <format>
#include <print>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_version.h>

#include <vulkan/vulkan.h>

using std::println, std::print;

constexpr bool print_version = true;

struct MySDLVersion {
    int maj;
    int min;
    int mic;
};

constexpr MySDLVersion decode_sdl_version(int version_encoded) {
    return {
        .maj = version_encoded / 1000000,
        .min = (version_encoded / 1000) % 1000,
        .mic = version_encoded % 1000
    };
}

constexpr std::string_view vk_result_to_string(VkResult r) {
    switch (r) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    default: return "Not supported, see https://registry.khronos.org/vulkan/specs/latest/man/html/VkResult.html";
    }
}

namespace std {
template <>
struct std::formatter<MySDLVersion> : std::formatter<std::string> {
    auto format(const MySDLVersion& v, auto& ctx) const {
        return std::format_to(ctx.out(), "{}.{}.{}", v.maj, v.min, v.mic);
    }
};

template <>
struct formatter<VkResult> : formatter<string_view> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    template <class FormatContext>
    auto format(VkResult r, FormatContext& ctx) const {
        return format_to(ctx.out(), "{} ({})", vk_result_to_string(r), static_cast<int>(r));
    }
};
} // namespace std

void check_vk_result(VkResult err) {
    if(err == VK_SUCCESS) return;
    println(stderr, "[Vulkan] Error: VkResult = {}", err);
    if (err < 0) abort();
}

void print_versions() {
    println("Version Printout:");
    MySDLVersion sdl_headers_version{SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION};
    println("\tSDL (headers): {}", sdl_headers_version);

    const int _sdl_linked_version = SDL_GetVersion();
    MySDLVersion sdl_linked_version = decode_sdl_version(_sdl_linked_version);
    println("\tSDL (runtime): {}", sdl_linked_version);

    println("\tImGui version: {}", IMGUI_VERSION);
    println("\tVulkan header version: {}", VK_HEADER_VERSION);
}

int main() {
    if(print_version) print_versions();
    check_vk_result(VK_SUCCESS);
    check_vk_result(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT);
}
