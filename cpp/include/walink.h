#pragma once

#include <stdint.h>
#include <stddef.h>

#include <string_view>

// Public C ABI for walink wasm side

using WL_VALUE = uint64_t;

// Tag values (28-bit) stored in the lower bits of the meta word.
// Interpretation differs by is-address flag:
//
// is-address = 0 (direct value in lower 32 bits)
//   0x10 : boolean
//   0x11 : sint8
//   0x21 : uint8
//   0x12 : sint16
//   0x22 : uint16
//   0x14 : sint32
//   0x24 : uint32
//   0x30 : float32
//
// is-address = 1 (payload is an address)
//   0x31     : float64 (Float64Container*)
//   0x01     : Bytes  (BaseContainer*; Node 에서는 Buffer)
//   0x02     : String (BaseContainer*)
//   0x0100   : Object (BaseContainer*; MsgPack 직렬화, Node 에서는 Object)
//   0x7fffff0: Error  (BaseContainer*; 문자열 오류 메세지, host 에서는 예외로 throw)
enum WL_TAG : uint32_t {
    // region: direct values (is-address = 0)

    WL_TAG_BOOLEAN  = 0x10,
    WL_TAG_SINT8    = 0x11,
    WL_TAG_UINT8    = 0x21,
    WL_TAG_SINT16   = 0x12,
    WL_TAG_UINT16   = 0x22,
    WL_TAG_SINT32   = 0x14,
    WL_TAG_UINT32   = 0x24,
    WL_TAG_FLOAT32  = 0x30,

    // endregion

    // region: address-based (is-address = 1)

    // Float64Container*
    WL_TAG_FLOAT64  = 0x31,
    // BaseContainer*
    WL_TAG_BYTES    = 0x01,
    // BaseContainer*
    WL_TAG_STRING   = 0x02,
    // MsgPack 직렬화, Node 에서는 Object
    WL_TAG_OBJECT   = 0x0100,
    // BaseContainer*; 문자열 오류 메세지, host 에서는 예외로 throw
    WL_TAG_ERROR    = 0x7fffff0,

    // endregion
};

// Bit layout inside the upper 32 bits of WL_VALUE
// bit 31: user-defined tag flag
// bit 30: is-address flag (1: payload is address, 0: direct value)
// bit 29: free flag (1: receiver must walink_free the address)
// bit 28: reserved
// bit 27-0: 28-bit tag value
constexpr uint32_t WL_META_USER_DEFINED = 0x80000000u;
constexpr uint32_t WL_META_IS_ADDRESS   = 0x40000000u;
constexpr uint32_t WL_META_FREE_FLAG    = 0x20000000u;
constexpr uint32_t WL_META_TAG_MASK     = 0x0FFFFFFFu;

struct BaseContainer {
    uint32_t cap;
    uint32_t size;
    uint8_t  data[];
};

struct Float64Container {
    double v;
};

namespace walink {

// Inline low-level meta/payload helpers
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

// Simple boolean factory (for return values)
inline WL_VALUE wl_from_bool(bool b) noexcept {
    const uint32_t payload = b ? 1u : 0u;
    const uint32_t meta = wl_build_meta(WL_TAG_BOOLEAN, /*is_address*/ false, /*free*/false, /*user*/false);
    return wl_make(meta, payload);
}

// String / Error factories (shared between lib and tests)
inline WL_VALUE wl_from_address(void* ptr,
                                uint32_t tag,
                                bool free_flag_for_receiver) noexcept {
    const uint32_t payload =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr));
    const uint32_t meta =
        wl_build_meta(tag, /*is_address*/ true, free_flag_for_receiver, /*user*/false);
    return wl_make(meta, payload);
}

// Allocation helpers (declarations only; implemented in cpp)
BaseContainer*    wl_alloc_container(uint32_t meta, uint64_t size) noexcept;
Float64Container* wl_alloc_f64(double v) noexcept;
WL_VALUE          wl_make_string(std::string_view sv,
                                 bool free_flag_for_receiver) noexcept;
WL_VALUE          wl_make_error(std::string_view msg) noexcept;

} // namespace walink

extern "C" {

// Helper API to be consumed from the host side
//
// NOTE: walink_alloc no longer accepts or writes a meta value into the
// allocated memory. It simply allocates `size` bytes and returns a WL_VALUE
// whose payload is the data pointer. The caller (library code) is expected
// to construct WL_VALUE meta bits separately when returning values.
//
// Allocate a container-like block in wasm memory.
WL_VALUE walink_alloc(uint32_t size) noexcept;

// Deallocate a container previously allocated in wasm memory.
// The input must be a value whose payload is the container address.
WL_VALUE walink_free(WL_VALUE value);

} // extern "C"