#include "walink.h"

#include <stdlib.h>
#include <string.h>
#include <string_view>

namespace walink {

BaseContainer* wl_alloc_container(uint32_t meta, uint64_t size) noexcept {
    // Request allocation for the BaseContainer header + payload.
    WL_VALUE wl_value = walink_alloc(static_cast<uint32_t>(sizeof(BaseContainer) + size));
    if (!wl_value) {
        return nullptr;
    }
    // walink_alloc returns payload as data pointer; interpret directly.
    auto* container = reinterpret_cast<BaseContainer*>(static_cast<uintptr_t>(wl_get_payload32(wl_value)));
    container->cap = static_cast<uint32_t>(size);
    container->size = 0;
    return container;
}

Float64Container* wl_alloc_f64(double v) noexcept {
    WL_VALUE wl_value = walink_alloc(static_cast<uint32_t>(sizeof(Float64Container)));
    if (!wl_value) {
        return nullptr;
    }
    // payload is the data pointer
    auto* c = reinterpret_cast<Float64Container*>(static_cast<uintptr_t>(wl_get_payload32(wl_value)));
    c->v = v;
    return c;
}

WL_VALUE wl_make_string(std::string_view sv, bool free_flag_for_receiver) noexcept {
    uint32_t meta = wl_build_meta(WL_TAG_STRING, true, free_flag_for_receiver);
    BaseContainer* c = wl_alloc_container(meta, sv.size());
    if (!c) return 0;
    if (!sv.empty()) {
        c->size = static_cast<uint32_t>(sv.size());
        memcpy(c->data, sv.data(), sv.size());
    }
    return wl_from_address(c, WL_TAG_STRING, free_flag_for_receiver);
}

WL_VALUE wl_make_error(std::string_view msg) noexcept {
    uint32_t meta = wl_build_meta(WL_TAG_ERROR, true, true);
    BaseContainer* c = wl_alloc_container(meta, msg.size());
    if (!c) return 0;
    if (!msg.empty()) {
        c->size = static_cast<uint32_t>(msg.size());
        memcpy(c->data, msg.data(), msg.size());
    }
    return wl_from_address(c, WL_TAG_ERROR, true);
}

} // namespace walink

extern "C" {

WL_VALUE walink_alloc(uint32_t size) noexcept {
    // Allocate exactly `size` bytes and return a WL_VALUE whose payload is the
    // data pointer (caller-visible start of the allocation).
    char* raw = reinterpret_cast<char*>(malloc(size));
    if (!raw) {
        return 0;
    }

    const uint32_t payload = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(raw));
    return walink::wl_make(0, payload);
}

// Deallocate containers referenced by address-based WL_VALUEs.
WL_VALUE walink_free(WL_VALUE value) {
    const uint32_t tag = walink::wl_get_tag(value);
 
    // Only address-based tags are valid here; ignore otherwise.
    if (!walink::wl_is_address(value)) {
        return walink::wl_from_bool(false);
    }
 
    // walink_alloc returns the data pointer as the payload; free that pointer.
    const uint32_t payload = walink::wl_get_payload32(value);
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(payload));
    free(ptr);
 
    return walink::wl_from_bool(true);
}

} // extern "C"