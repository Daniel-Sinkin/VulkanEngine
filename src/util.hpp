#pragma once
#include <cstdint>
#include <format>
#include <limits>
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

constexpr size_t uuid_size = 16;
constexpr size_t uuid_string_length = uuid_size * 2 + (uuid_size - 1);
static_assert(uuid_string_length == 47);

constexpr char HEX_charset[] = "0123456789ABCDEF";

constexpr char null_terminator = '\0';

constexpr uint32_t queue_familily_not_init = std::numeric_limits<uint32_t>::max();
} // namespace DS::Constants

namespace DS::Util {
inline std::string uuid_to_string(const std::uint8_t (&uuid)[Constants::uuid_size]) {
    std::string s(Constants::uuid_string_length, Constants::null_terminator);

    std::size_t pos = 0;
    for (int i = 0; i < Constants::uuid_size; ++i) {
        if (i > 0) s[pos++] = '-';
        unsigned v = uuid[i];
        s[pos++] = Constants::HEX_charset[(v >> 4) & 0xF];
        s[pos++] = Constants::HEX_charset[v & 0xF];
    }
    return s;
}

struct VersionTriple {
    int maj;
    int min;
    int mic;
};

constexpr VersionTriple decode_sdl_version(int version_encoded) {
    return {
        .maj = version_encoded / 1000000,
        .min = (version_encoded / 1000) % 1000,
        .mic = version_encoded % 1000};
}
} // namespace DS::Util

namespace std {
template <>
struct std::formatter<DS::Util::VersionTriple> : std::formatter<std::string> {
    auto format(const DS::Util::VersionTriple &v, auto &ctx) const {
        return std::format_to(ctx.out(), "{}.{}.{}", v.maj, v.min, v.mic);
    }
};
} // namespace std

namespace DS::Util {
void print_versions() {
    println("[Global] Info: Version Printout");
    Util::VersionTriple sdl_headers_version{SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION};
    println("[   SDL] Info: \tHeaders Version {}", sdl_headers_version);

    const int _sdl_linked_version = SDL_GetVersion();
    Util::VersionTriple sdl_linked_version = Util::decode_sdl_version(_sdl_linked_version);
    println("[   SDL] Info: \tLinked Version {}", sdl_linked_version);

    println("[ ImGui] Info: \tVersion {}", IMGUI_VERSION);
    println("[Vulkan] Info: \tVersion {}", VK_HEADER_VERSION);
}
} // namespace DS::Util