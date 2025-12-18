#include "walink.h"

#include <stdlib.h>
#include <string.h>
#include <string>
#include <string_view>
#include <stdexcept>

static void* walink_alloc_ptr(uint32_t size) {
    return malloc(size);
}

namespace walink {

// ---- Convenience factories for direct-value scalars (to/from) -----------

WL_VALUE wl_from_bool(bool b) noexcept {
    const uint32_t payload = b ? 1u : 0u;
    const uint32_t meta = wl_build_meta(WL_TAG_BOOLEAN, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

bool wl_to_bool(WL_VALUE v) noexcept {
    return wl_get_payload32(v) != 0;
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


BaseContainer* wl_alloc_container(uint32_t /*meta*/, uint32_t size) noexcept {
    // Allocate enough space for the header + payload
    void* raw = walink_alloc_ptr(static_cast<uint32_t>(sizeof(BaseContainer) + size));
    if (!raw) {
        return nullptr;
    }
    auto* container = reinterpret_cast<BaseContainer*>(raw);
    container->cap = static_cast<uint32_t>(size);
    container->size = 0;
    return container;
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

WL_VALUE wl_make_f64(double v, bool free_flag_for_receiver) noexcept {
    void* raw = walink_alloc_ptr(static_cast<uint32_t>(sizeof(Float64Container)));
    if (!raw) return 0;
    auto* c = reinterpret_cast<Float64Container*>(raw);
    c->v = v;
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
  
// Null value factory (tag = 0)
WL_VALUE wl_null() noexcept {
    return wl_make(0u, 0u);
}
 
// ---- Converters ----------------------------------------------------------
//
// These helpers extract payloads from WL_VALUE. If `allow_free` is true and
// the meta free-flag is set on the value, the underlying allocation is freed
// by calling walink_free(v) before returning.
 
std::string wl_to_string(WL_VALUE v, bool allow_free) {
    // Strict: only accept a value whose tag is exactly WL_TAG_STRING and is address-based.
    const uint32_t tag = wl_get_tag(v);
    if (!wl_is_address(v) || tag != WL_TAG_STRING) {
        throw std::runtime_error("wl_to_string: expected address-based STRING tag");
    }

    const uint32_t payload = wl_get_payload32(v);
    auto* c = reinterpret_cast<BaseContainer*>(static_cast<uintptr_t>(payload));
    if (!c) {
        throw std::runtime_error("wl_to_string: null container");
    }

    std::string out;
    if (c->size) {
        out.assign(reinterpret_cast<const char*>(c->data), c->size);
    }

    if (allow_free) {
        const uint32_t meta = wl_get_meta(v);
        if (meta & WL_META_FREE_FLAG) {
            ::walink_free(v);
        }
    }
    return out;
}

std::string wl_read_base_container(WL_VALUE v, bool allow_free) {
    const uint32_t tag = wl_get_tag(v);
    if (!wl_is_address(v)) {
        throw std::runtime_error("wl_to_bytes: expected address-based tag");
    }

    const uint32_t payload = wl_get_payload32(v);
    auto* c = reinterpret_cast<BaseContainer*>(static_cast<uintptr_t>(payload));
    if (!c) {
        throw std::runtime_error("wl_to_bytes: null container");
    }

    std::string out;
    if (c->size) {
        out.assign(reinterpret_cast<const char*>(c->data), c->size);
    }

    if (allow_free) {
        const uint32_t meta = wl_get_meta(v);
        if (meta & WL_META_FREE_FLAG) {
            ::walink_free(v);
        }
    }
    return out;
}

std::string wl_to_bytes(WL_VALUE v, bool allow_free) {
    // Strict: only accept a value whose tag is exactly WL_TAG_BYTES and is address-based.
    const uint32_t tag = wl_get_tag(v);
    if (!wl_is_address(v) || tag != WL_TAG_BYTES) {
        throw std::runtime_error("wl_to_bytes: expected address-based BYTES tag");
    }
    return wl_read_base_container(v, allow_free);
}
 
std::string wl_to_msgpack(WL_VALUE v, bool allow_free) {
    // Strict: only accept MSGPACK tag.
    const uint32_t tag = wl_get_tag(v);
    if (!wl_is_address(v) || tag != WL_TAG_MSGPACK) {
        throw std::runtime_error("wl_to_msgpack: expected address-based MSGPACK tag");
    }
    return wl_read_base_container(v, allow_free);
}
 
double wl_to_f64(WL_VALUE v, bool allow_free) {
    // Strict: only accept a value whose tag is exactly WL_TAG_FLOAT64 and is address-based.
    const uint32_t tag = wl_get_tag(v);
    if (!wl_is_address(v) || tag != WL_TAG_FLOAT64) {
        throw std::runtime_error("wl_to_f64: expected address-based FLOAT64 tag");
    }

    const uint32_t payload = wl_get_payload32(v);
    auto* f = reinterpret_cast<Float64Container*>(static_cast<uintptr_t>(payload));
    if (!f) {
        throw std::runtime_error("wl_to_f64: null Float64Container");
    }
    double result = f->v;

    if (allow_free) {
        const uint32_t meta = wl_get_meta(v);
        if (meta & WL_META_FREE_FLAG) {
            ::walink_free(v);
        }
    }

    return result;
}
 
} // namespace walink

extern "C" {

WL_VALUE walink_alloc(uint32_t size) noexcept {
    // Allocate exactly `size` bytes and return a WL_VALUE whose payload is the
    // data pointer (caller-visible start of the allocation).
    char* raw = reinterpret_cast<char*>(walink_alloc_ptr(size));
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