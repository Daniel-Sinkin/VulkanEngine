#include <format>
#include <print>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_version.h>

#include <vulkan/vulkan.h>

using std::println;

struct Settings {
    inline static bool print_version = true;
};

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

namespace std {
template <>
struct std::formatter<MySDLVersion> : std::formatter<std::string> {
    auto format(const MySDLVersion& v, auto& ctx) const {
        return std::format_to(ctx.out(), "{}.{}.{}", v.maj, v.min, v.mic);
    :}
};
} // namespace std

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
    if(Settings::print_version) print_versions();
}
