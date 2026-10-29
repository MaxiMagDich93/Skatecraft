#include "overlay_feed.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>

namespace dingosdk::skatecraft::overlay_feed {
namespace {
std::mutex mutex;
Frame waiting;
bool fresh{};
std::atomic<bool> visible{};
// A 4K frame is 33 MB; anything bigger is not a frame Minecraft should have sent.
constexpr std::uint64_t most_bytes = 3840ull * 2160ull * 4ull;
}

void publish(const std::uint8_t* pixels, std::uint32_t width, std::uint32_t height, bool bottom_up, std::uint64_t id) {
    const std::uint64_t bytes = std::uint64_t{width} * height * 4;
    if (!pixels || !width || !height || bytes > most_bytes) return;
    std::lock_guard lock(mutex);
    waiting.width = width;
    waiting.height = height;
    waiting.id = id;
    waiting.rgba.resize(static_cast<std::size_t>(bytes));
    const std::size_t row = std::size_t{width} * 4;
    if (!bottom_up) std::memcpy(waiting.rgba.data(), pixels, static_cast<std::size_t>(bytes));
    else
        for (std::uint32_t y = 0; y < height; ++y) std::memcpy(waiting.rgba.data() + y * row, pixels + (height - 1 - y) * row, row);
    // Premultiplied to straight alpha. Nearly every pixel is fully clear or fully solid, which
    // need nothing; only the soft edges are divided.
    auto* p = waiting.rgba.data();
    for (std::size_t i = 0; i < bytes; i += 4) {
        const unsigned a = p[i + 3];
        if (a == 0 || a == 255) continue;
        for (int k = 0; k < 3; ++k) p[i + k] = static_cast<std::uint8_t>(std::min(255u, (p[i + k] * 255u + a / 2) / a));
    }
    fresh = true;
    visible.store(true, std::memory_order_release);
}

void clear() {
    std::lock_guard lock(mutex);
    fresh = false;
    waiting.rgba.clear();
    visible.store(false, std::memory_order_release);
}

bool take(Frame& out) {
    std::lock_guard lock(mutex);
    if (!fresh) return false;
    fresh = false;
    out.width = waiting.width;
    out.height = waiting.height;
    out.id = waiting.id;
    out.rgba.swap(waiting.rgba);
    return true;
}

bool shown() { return visible.load(std::memory_order_acquire); }
}
