#pragma once
#include <cstdint>
#include <format>
#include <print>

using std::print, std::println;

namespace DS::Constants {
constexpr uint64_t no_timeout = UINT64_MAX;
constexpr uint32_t no_flags = 0;

constexpr int window_width = 1280;
constexpr int window_height = 720;

SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

constexpr bool log_setup = false;
constexpr bool print_version = true;
} // namespace DS::Constants

namespace DS::Util {
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
} // namespace DS::Util

namespace std {
template <>
struct std::formatter<DS::Util::MySDLVersion> : std::formatter<std::string> {
    auto format(const DS::Util::MySDLVersion &v, auto &ctx) const {
        return std::format_to(ctx.out(), "{}.{}.{}", v.maj, v.min, v.mic);
    }
};
} // namespace std

namespace DS::Util {
void print_versions() {
    println("[Global] Info: Version Printout");
    Util::MySDLVersion sdl_headers_version{SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION};
    println("[   SDL] Info: \tHeaders Version {}", sdl_headers_version);

    const int _sdl_linked_version = SDL_GetVersion();
    Util::MySDLVersion sdl_linked_version = Util::decode_sdl_version(_sdl_linked_version);
    println("[   SDL] Info: \tLinked Version {}", sdl_linked_version);

    println("[ ImGui] Info: \tVersion {}", IMGUI_VERSION);
    println("[Vulkan] Info: \tVersion {}", VK_HEADER_VERSION);
}
} // namespace DS::Util