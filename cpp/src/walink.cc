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

static Float64Container* wl_alloc_f64(double v) noexcept {
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


// ---- Convenience factories for direct-value scalars (to/from) -----------

WL_VALUE wl_from_bool(bool b) noexcept {
    const uint32_t payload = b ? 1u : 0u;
    const uint32_t meta = wl_build_meta(WL_TAG_BOOLEAN, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

WL_VALUE wl_from_sint8(int32_t v) noexcept {
    const int32_t val = static_cast<int32_t>(static_cast<int8_t>(v));
    const uint32_t payload = static_cast<uint32_t>(val);
    const uint32_t meta = wl_build_meta(WL_TAG_SINT8, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

int32_t wl_to_sint8(WL_VALUE v) noexcept {
    return static_cast<int32_t>(static_cast<int8_t>(wl_get_payload32(v)));
}

WL_VALUE wl_from_uint8(uint32_t v) noexcept {
    const uint32_t payload = v & 0xffu;
    const uint32_t meta = wl_build_meta(WL_TAG_UINT8, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

uint32_t wl_to_uint8(WL_VALUE v) noexcept {
    return wl_get_payload32(v) & 0xffu;
}

WL_VALUE wl_from_sint16(int32_t v) noexcept {
    const int32_t val = static_cast<int32_t>(static_cast<int16_t>(v));
    const uint32_t payload = static_cast<uint32_t>(val);
    const uint32_t meta = wl_build_meta(WL_TAG_SINT16, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

int32_t wl_to_sint16(WL_VALUE v) noexcept {
    return static_cast<int32_t>(static_cast<int16_t>(wl_get_payload32(v)));
}

WL_VALUE wl_from_uint16(uint32_t v) noexcept {
    const uint32_t payload = v & 0xffffu;
    const uint32_t meta = wl_build_meta(WL_TAG_UINT16, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

uint32_t wl_to_uint16(WL_VALUE v) noexcept {
    return wl_get_payload32(v) & 0xffffu;
}

WL_VALUE wl_from_uint32(uint32_t v) noexcept {
    const uint32_t payload = v;
    const uint32_t meta = wl_build_meta(WL_TAG_UINT32, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

uint32_t wl_to_uint32(WL_VALUE v) noexcept {
    return wl_get_payload32(v);
}

WL_VALUE wl_from_float32(float f) noexcept {
    uint32_t payload;
    static_assert(sizeof(float) == sizeof(uint32_t), "float must be 32-bit");
    memcpy(&payload, &f, sizeof(payload));
    const uint32_t meta = wl_build_meta(WL_TAG_FLOAT32, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

float wl_to_float32(WL_VALUE v) noexcept {
    uint32_t payload = wl_get_payload32(v);
    float f;
    memcpy(&f, &payload, sizeof(f));
    return f;
}

WL_VALUE wl_from_sint32(int32_t v) noexcept {
    const uint32_t payload = static_cast<uint32_t>(v);
    const uint32_t meta = wl_build_meta(WL_TAG_SINT32, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

int32_t wl_to_sint32(WL_VALUE v) noexcept {
    return static_cast<int32_t>(wl_get_payload32(v));
}

// ---- Address-based factories (containers / float64) ---------------------

WL_VALUE wl_make_f64(double v, bool free_flag_for_receiver) noexcept {
    Float64Container* c = wl_alloc_f64(v);
    if (!c) return 0;
    return wl_from_address(c, WL_TAG_FLOAT64, free_flag_for_receiver);
}

WL_VALUE wl_make_bytes(std::string_view sv, bool free_flag_for_receiver) noexcept {
    const uint32_t meta = wl_build_meta(WL_TAG_BYTES, /*is_address*/ true, free_flag_for_receiver, /*user*/false);
    BaseContainer* c = wl_alloc_container(meta, sv.size());
    if (!c) return 0;
    if (!sv.empty()) {
        c->size = static_cast<uint32_t>(sv.size());
        memcpy(c->data, sv.data(), sv.size());
    }
    return wl_from_address(c, WL_TAG_BYTES, free_flag_for_receiver);
}

WL_VALUE wl_make_msgpack(std::string_view sv, bool free_flag_for_receiver) noexcept {
    const uint32_t meta = wl_build_meta(WL_TAG_MSGPACK, /*is_address*/ true, free_flag_for_receiver, /*user*/false);
    BaseContainer* c = wl_alloc_container(meta, sv.size());
    if (!c) return 0;
    if (!sv.empty()) {
        c->size = static_cast<uint32_t>(sv.size());
        memcpy(c->data, sv.data(), sv.size());
    }
    return wl_from_address(c, WL_TAG_MSGPACK, free_flag_for_receiver);
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