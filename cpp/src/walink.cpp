#include "walink.h"

#include <stdlib.h>
#include <string.h>
#include <string_view>

namespace walink_internal {

// ---- Shared allocation helpers (exported via libwalink) --------------------

BaseContainer* wl_alloc_container(uint32_t meta, uint64_t size) noexcept {
    WL_VALUE wl_value = walink_alloc(meta, 8 + size);
    if (!wl_value) {
        return nullptr;
    }
    auto* container = reinterpret_cast<BaseContainer*>(wl_value & 0xffffffff);
    container->cap = size;
    container->size = 0;
    return container;
}

Float64Container* wl_alloc_f64(double v) noexcept {
    void* raw = malloc(sizeof(Float64Container));
    if (!raw) {
        return nullptr;
    }
    auto* c = static_cast<Float64Container*>(raw);
    c->v = v;
    return c;
}

} // namespace walink_internal

namespace {

// ---- WL_VALUE meta/payload helpers -----------------------------------------

inline uint32_t wl_get_meta(WL_VALUE v) noexcept {
    return static_cast<uint32_t>(v >> 32);
}

inline uint32_t wl_get_payload32(WL_VALUE v) noexcept {
    return static_cast<uint32_t>(v & 0xffffffffu);
}

inline WL_VALUE wl_make(uint32_t meta, uint32_t payload) noexcept {
    return (static_cast<WL_VALUE>(meta) << 32) | static_cast<WL_VALUE>(payload);
}

inline uint32_t wl_get_tag(WL_VALUE v) noexcept {
    const auto meta = wl_get_meta(v);
    return (meta & WL_META_TAG_MASK);
}

inline bool wl_is_address(WL_VALUE v) noexcept {
    return (wl_get_meta(v) & WL_META_IS_ADDRESS) != 0;
}

inline uint32_t wl_build_meta(uint32_t tag,
                                   bool is_address,
                                   bool free_flag = false,
                                   bool user_defined = false) noexcept {
    uint32_t meta = (tag & WL_META_TAG_MASK);
    if (user_defined) {
        meta |= WL_META_USER_DEFINED;
    }
    if (is_address) {
        meta |= WL_META_IS_ADDRESS;
    }
    if (free_flag) {
        meta |= WL_META_FREE_FLAG;
    }
    return meta;
}

// ---- Address helpers -------------------------------------------------------

inline void* wl_to_address(WL_VALUE v) noexcept {
    const uint32_t payload = wl_get_payload32(v);
    return reinterpret_cast<void*>(
        static_cast<uintptr_t>(payload));
}

// ---- Simple boolean factory (for return values) ----------------------------

inline WL_VALUE wl_from_bool(bool b) noexcept {
    const uint32_t payload = b ? 1u : 0u;
    const uint32_t meta = wl_build_meta(WL_TAG_BOOLEAN, /*is_address*/ false);
    return wl_make(meta, payload);
}

// ---- String / Error factories (shared between lib and tests) ---------------

inline WL_VALUE wl_from_address(void* ptr,
                                uint32_t tag,
                                bool free_flag_for_receiver) noexcept {
    const uint32_t payload =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr));
    const uint32_t meta =
        wl_build_meta(tag, /*is_address*/ true, free_flag_for_receiver);
    return wl_make(meta, payload);
}

} // namespace

namespace walink_internal {

WL_VALUE wl_make_string(std::string_view sv,
                        bool free_flag_for_receiver) noexcept {
    uint32_t meta = wl_build_meta(WL_TAG_STRING, true, free_flag_for_receiver);
    BaseContainer* c = wl_alloc_container(meta, sv.size());
    if (!c) {
        return 0;
    }
    if (!sv.empty()) {
        c->size = sv.size();
        memcpy(c->data, sv.data(), sv.size());
    }
    return wl_from_address(c, WL_TAG_STRING, free_flag_for_receiver);
}

WL_VALUE wl_make_error(std::string_view msg) noexcept {
    uint32_t meta = wl_build_meta(WL_TAG_STRING, true, true);
    BaseContainer* c = wl_alloc_container(meta, msg.size());
    if (!c) {
        return 0;
    }
    if (!msg.empty()) {
        c->size = msg.size();
        memcpy(c->data, msg.data(), msg.size());
    }
    return wl_from_address(c, WL_TAG_STRING, true);
}

} // namespace walink_internal

extern "C" {
 
// Allocate a block of memory suitable for address-based WL_VALUE payloads.
// The allocated layout is:
//   [0..3]   : uint32_t meta (little-endian)
//   [4..]    : container payload (e.g. BaseContainer or Float64Container)
// For container-like payloads we place the BaseContainer struct at offset +4
// so that existing host-side readers that skip the first 4 bytes can find
// cap/size at offsets (ptr+4) and (ptr+12) respectively.
//
// Returns a WL_VALUE with the supplied meta and the allocated pointer as payload.
// On allocation failure, returns wl_make(meta, 0).
WL_VALUE walink_alloc(uint32_t meta, uint32_t size) noexcept {
    // We'll allocate space for a BaseContainer header + data payload after the 4-byte meta.
    // Total allocation = 4 (meta) + sizeof(BaseContainer) + size
    const size_t total_size = 4 + size;
 
    char* raw = reinterpret_cast<char*>(malloc(total_size));
    if (!raw) {
        // allocation failed -> return a WL_VALUE with null payload
        return 0;
    }
 
    uint32_t* meta_ptr = reinterpret_cast<uint32_t*>(raw);
    *meta_ptr = meta;

    const uint32_t payload = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(raw));
    return wl_make(meta, payload);
}
 
// Deallocate containers referenced by address-based WL_VALUEs.
// - Bytes/String/Object/Error: BaseContainer*
// - Float64                    : Float64Container*
// 그 외 태그나 direct-value(is-address = 0) 값은 무시합니다.
WL_VALUE walink_free(WL_VALUE value) {
    const uint32_t tag = wl_get_tag(value);
 
    // Only address-based tags are valid here; ignore otherwise.
    if (!wl_is_address(value)) {
        return wl_from_bool(false);
    }
 
    free(wl_to_address(value));
 
    return wl_from_bool(true);
}
 
} // extern "C"