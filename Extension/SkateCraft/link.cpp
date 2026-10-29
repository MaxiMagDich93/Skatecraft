#include "link.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <immintrin.h>
#ifdef _WIN32
#include <Windows.h>
#include <sddl.h>
#include <vector>
#endif

// Ring and seqlock code as in SkyCraft's skse/src/Link.cpp (MIT, chasmlol), so the two sides stay
// byte-for-byte compatible. Only the naming, the logging and the split between the protocol
// (LinkView, plain memory, testable anywhere) and the Windows mapping (Link) are ours.
namespace dingosdk::skatecraft {
namespace {
template<class T> std::atomic_ref<T> atomic(T& value) { return std::atomic_ref<T>(value); }
// Minecraft's render thread, which beats, stops for several seconds while it opens its world
// (hidden, nobody sees it); SkyCraft's own Java side gives Skyrim 8 s for the same reason.
constexpr std::uint64_t minecraft_timeout_ms = 10000;

template<class T> void seqlock_write(T* dst, const T& value) {
    auto seq = atomic(*reinterpret_cast<std::uint32_t*>(dst));
    const auto s = seq.load(std::memory_order_relaxed);
    seq.store(s + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    std::memcpy(reinterpret_cast<std::uint8_t*>(dst) + 4, reinterpret_cast<const std::uint8_t*>(&value) + 4, sizeof(T) - 4);
    seq.store(s + 2, std::memory_order_release);
}
}

void LinkView::reset_own_regions(std::uint32_t pid, std::uint64_t now_ms) {
    if (!base_) return;
    auto* header = at<proto::Header>(proto::kOffHeader);
    std::memset(base_ + proto::kOffSkyState, 0, sizeof(proto::SkyState));
    std::memset(base_ + proto::kOffOverlayCtl, 0, 0x100);
    std::memset(base_ + proto::kOffWaterGrid, 0, sizeof(proto::WaterGrid));
    std::memset(base_ + proto::kOffInputRing, 0, proto::kInputRingDataOff);
    std::memset(base_ + proto::kOffCollisionRing, 0, proto::kColRingDataOff);
    std::memset(base_ + proto::kOffActorTable, 0, sizeof(proto::ActorTable));
    std::memset(base_ + proto::kOffEventRing, 0, proto::kEventRingDataOff);
    std::memset(base_ + proto::kOffWorldEntities, 0, sizeof(proto::WorldEntities));
    std::memset(base_ + proto::kOffRenderRing, 0, proto::kRenRingDataOff);
    header->version = proto::kVersion;
    header->skyrimPid = pid; // "skyrim" in the protocol's names means the host game: here skate.
    header->skyrimHeartbeatMs = now_ms;
    atomic(header->magic).store(proto::kMagic, std::memory_order_release);
    // skate. has no water Minecraft should swim in, and no NPCs to fight: say so once, plainly.
    proto::WaterGrid water{};
    for (auto& surface : water.surface) surface = proto::kNoWater;
    seqlock_write(at<proto::WaterGrid>(proto::kOffWaterGrid), water);
    overlay_front_ = 2;
}

void LinkView::heartbeat(std::uint64_t now_ms) {
    if (base_) atomic(at<proto::Header>(proto::kOffHeader)->skyrimHeartbeatMs).store(now_ms, std::memory_order_release);
}

bool LinkView::minecraft_alive(std::uint64_t now_ms) const {
    if (!base_) return false;
    const auto last = atomic(at<proto::Header>(proto::kOffHeader)->mcHeartbeatMs).load(std::memory_order_acquire);
    return last != 0 && now_ms - last < minecraft_timeout_ms;
}

std::uint32_t LinkView::minecraft_pid() const {
    return base_ ? atomic(at<proto::Header>(proto::kOffHeader)->mcPid).load(std::memory_order_acquire) : 0;
}

void LinkView::write_sky_state(const proto::SkyState& state) {
    if (base_) seqlock_write(at<proto::SkyState>(proto::kOffSkyState), state);
}

bool LinkView::read_mc_state(proto::McState& out) const {
    if (!base_) return false;
    auto* src = at<proto::McState>(proto::kOffMcState);
    auto seq = atomic(src->seq);
    for (int attempt = 0; attempt < 64; ++attempt) {
        const auto s1 = seq.load(std::memory_order_acquire);
        if (s1 & 1) {
            _mm_pause();
            continue;
        }
        std::memcpy(&out, src, sizeof(proto::McState));
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq.load(std::memory_order_relaxed) == s1) return s1 != 0; // 0: Minecraft never wrote one
    }
    return false;
}

bool LinkView::read_selection(Selection& out) const {
    if (!base_) return false;
    auto* src = at<proto::WorldEntities>(proto::kOffWorldEntities);
    auto seq = atomic(src->seq);
    for (int attempt = 0; attempt < 16; ++attempt) {
        const auto s1 = seq.load(std::memory_order_acquire);
        if (s1 & 1) {
            _mm_pause();
            continue;
        }
        Selection copy;
        copy.has = src->hasSelection != 0;
        std::memcpy(copy.min, src->selMin, sizeof copy.min);
        std::memcpy(copy.max, src->selMax, sizeof copy.max);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq.load(std::memory_order_relaxed) == s1) {
            out = copy;
            return true;
        }
    }
    return false;
}

void LinkView::push_input(proto::InputType type, std::uint16_t code, std::int32_t a, std::int32_t b, std::int32_t c) {
    if (!base_) return;
    auto* ring = base_ + proto::kOffInputRing;
    auto& head_ref = *reinterpret_cast<std::uint64_t*>(ring + proto::kInputRingHeadOff);
    auto& tail_ref = *reinterpret_cast<std::uint64_t*>(ring + proto::kInputRingTailOff);
    const auto head = atomic(head_ref).load(std::memory_order_relaxed);
    const auto tail = atomic(tail_ref).load(std::memory_order_acquire);
    if (head - tail >= proto::kInputRingEntries) return;
    auto* entry = reinterpret_cast<proto::InputEvent*>(ring + proto::kInputRingDataOff) + (head & (proto::kInputRingEntries - 1));
    *entry = {static_cast<std::uint16_t>(type), code, a, b, c};
    atomic(head_ref).store(head + 1, std::memory_order_release);
}

bool LinkView::write_collision(proto::ColType type, const void* payload, std::uint32_t bytes) {
    if (!base_) return false;
    auto* ring = base_ + proto::kOffCollisionRing;
    auto& head_ref = *reinterpret_cast<std::uint64_t*>(ring + proto::kColRingHeadOff);
    auto& tail_ref = *reinterpret_cast<std::uint64_t*>(ring + proto::kColRingTailOff);
    auto* data = ring + proto::kColRingDataOff;
    constexpr auto size = proto::kColRingDataBytes;
    const std::uint64_t message = (sizeof(proto::ColMsgHeader) + bytes + 7) & ~7ull;
    if (message > size / 2) return false;
    auto head = atomic(head_ref).load(std::memory_order_relaxed);
    const auto tail = atomic(tail_ref).load(std::memory_order_acquire);
    auto pos = head % size;
    const auto pad = (pos + message > size) ? size - pos : 0;
    if (size - (head - tail) < message + pad) return false;
    if (pad) {
        *reinterpret_cast<proto::ColMsgHeader*>(data + pos) = {proto::kColPad, 0};
        head += pad;
        pos = 0;
    }
    *reinterpret_cast<proto::ColMsgHeader*>(data + pos) = {type, bytes};
    if (bytes) std::memcpy(data + pos + sizeof(proto::ColMsgHeader), payload, bytes);
    atomic(head_ref).store(head + message, std::memory_order_release);
    return true;
}

std::uint64_t LinkView::collision_room() const {
    if (!base_) return 0;
    auto* ring = base_ + proto::kOffCollisionRing;
    const auto head = atomic(*reinterpret_cast<std::uint64_t*>(ring + proto::kColRingHeadOff)).load(std::memory_order_relaxed);
    const auto tail = atomic(*reinterpret_cast<std::uint64_t*>(ring + proto::kColRingTailOff)).load(std::memory_order_acquire);
    return proto::kColRingDataBytes - (head - tail);
}

bool LinkView::pop_event(proto::McEvent& out) {
    if (!base_) return false;
    auto* ring = base_ + proto::kOffEventRing;
    auto& head_ref = *reinterpret_cast<std::uint64_t*>(ring + proto::kEventRingHeadOff);
    auto& tail_ref = *reinterpret_cast<std::uint64_t*>(ring + proto::kEventRingTailOff);
    const auto head = atomic(head_ref).load(std::memory_order_acquire);
    auto tail = atomic(tail_ref).load(std::memory_order_relaxed);
    if (tail >= head) return false;
    if (head - tail > proto::kEventRingEntries) tail = head - proto::kEventRingEntries;
    out = reinterpret_cast<const proto::McEvent*>(ring + proto::kEventRingDataOff)[tail & (proto::kEventRingEntries - 1)];
    atomic(tail_ref).store(tail + 1, std::memory_order_release);
    return true;
}

void LinkView::drain_render(const std::function<void(std::uint32_t, const std::uint8_t*, std::uint32_t)>& fn, std::uint64_t max_bytes) {
    if (!base_) return;
    auto* ring = base_ + proto::kOffRenderRing;
    auto& head_ref = *reinterpret_cast<std::uint64_t*>(ring + proto::kRenRingHeadOff);
    auto& tail_ref = *reinterpret_cast<std::uint64_t*>(ring + proto::kRenRingTailOff);
    const auto head = atomic(head_ref).load(std::memory_order_acquire);
    auto tail = atomic(tail_ref).load(std::memory_order_relaxed);
    auto* data = ring + proto::kRenRingDataOff;
    constexpr auto size = proto::kRenRingDataBytes;
    std::uint64_t done = 0;
    while (tail < head && done < max_bytes) {
        const auto pos = tail % size;
        const auto* header = reinterpret_cast<const proto::ColMsgHeader*>(data + pos);
        if (header->type == proto::kRenPad) {
            tail += size - pos;
            continue;
        }
        fn(header->type, data + pos + sizeof(proto::ColMsgHeader), header->payloadBytes);
        const auto message = (sizeof(proto::ColMsgHeader) + header->payloadBytes + 7) & ~7ull;
        tail += message;
        done += message;
    }
    atomic(tail_ref).store(tail, std::memory_order_release);
}

bool LinkView::acquire_overlay_frame() {
    if (!base_) return false;
    auto& state = at<proto::OverlayCtl>(proto::kOffOverlayCtl)->state;
    if (!(atomic(state).load(std::memory_order_acquire) & proto::kOverlayDirty)) return false;
    const auto old = atomic(state).exchange(overlay_front_, std::memory_order_acq_rel);
    overlay_front_ = old & 3;
    return true;
}

void LinkView::reset_overlay() {
    if (!base_) return;
    atomic(at<proto::OverlayCtl>(proto::kOffOverlayCtl)->state).store(0, std::memory_order_release);
    overlay_front_ = 2;
}

const std::uint8_t* LinkView::front_pixels() const {
    return base_ + proto::kOffOverlayPixels + proto::kOverlaySlotBytes * overlay_front_;
}

const proto::OverlaySlotHdr* LinkView::front_header() const {
    return at<proto::OverlaySlotHdr>(proto::kOffOverlaySlotHdr + sizeof(proto::OverlaySlotHdr) * overlay_front_);
}

Link& Link::get() {
    static Link link;
    return link;
}

#ifdef _WIN32
namespace {
// Who may open the mapping: this Windows user, the system and administrators, at normal integrity.
// As in SkyCraft: a skate. run as administrator would otherwise make it administrators-only, and
// Minecraft (never elevated) could not open it. Free with LocalFree.
PSECURITY_DESCRIPTOR shared_with_this_user() {
    std::wstring sddl = L"D:P(A;;GA;;;SY)(A;;GA;;;BA)";
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        DWORD size = 0;
        GetTokenInformation(token, TokenUser, nullptr, 0, &size);
        std::vector<std::uint8_t> buffer(size);
        if (size && GetTokenInformation(token, TokenUser, buffer.data(), size, &size)) {
            LPWSTR sid = nullptr;
            if (ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &sid)) {
                sddl += std::wstring(L"(A;;GA;;;") + sid + L")";
                LocalFree(sid);
            }
        }
        CloseHandle(token);
    }
    sddl += L"S:(ML;;NW;;;ME)";
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr)) return nullptr;
    return descriptor;
}
}

std::string Link::create() {
    if (view_.valid()) return {};
    const auto size = proto::kMappingBytes;
    SECURITY_ATTRIBUTES access{sizeof(access), shared_with_this_user(), FALSE};
    const HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, access.lpSecurityDescriptor ? &access : nullptr, PAGE_READWRITE,
                                              static_cast<DWORD>(size >> 32), static_cast<DWORD>(size & 0xFFFFFFFF), mapping_name);
    const DWORD error = GetLastError();
    if (access.lpSecurityDescriptor) LocalFree(access.lpSecurityDescriptor);
    if (!mapping) return "CreateFileMapping failed (" + std::to_string(error) + ")";
    auto* base = static_cast<std::uint8_t*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0));
    if (!base) {
        const DWORD map_error = GetLastError();
        CloseHandle(mapping);
        return "MapViewOfFile failed (" + std::to_string(map_error) + ")";
    }
    mapping_ = mapping;
    view_ = LinkView(base);
    view_.reset_own_regions(GetCurrentProcessId(), GetTickCount64());
    return {};
}

void Link::close() {
    if (view_.valid()) UnmapViewOfFile(view_.base());
    if (mapping_) CloseHandle(static_cast<HANDLE>(mapping_));
    mapping_ = nullptr;
    view_ = LinkView();
}
#else
std::string Link::create() { return "shared memory is Windows only"; }
void Link::close() {
    mapping_ = nullptr;
    view_ = LinkView();
}
#endif
}
