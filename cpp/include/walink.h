#pragma once

#include <stdint.h>
#include <stddef.h>

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
    // direct values (is-address = 0)
    WL_TAG_BOOLEAN  = 0x10,
    WL_TAG_SINT8    = 0x11,
    WL_TAG_UINT8    = 0x21,
    WL_TAG_SINT16   = 0x12,
    WL_TAG_UINT16   = 0x22,
    WL_TAG_SINT32   = 0x14,
    WL_TAG_UINT32   = 0x24,
    WL_TAG_FLOAT32  = 0x30,

    // address-based (is-address = 1)
    WL_TAG_FLOAT64  = 0x31,
    WL_TAG_BYTES    = 0x01,
    WL_TAG_STRING   = 0x02,
    WL_TAG_OBJECT   = 0x0100,
    WL_TAG_ERROR    = 0x7fffff0,
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

extern "C" {
 
// Helper API to be consumed from the host side
 
// Allocate a container-like block in wasm memory.
// The runtime will allocate (4 + size) bytes, write `meta` as the first
// uint32 (little-endian), and return a WL_VALUE payload that points to
// the start (the caller will use the payload as the address).
WL_VALUE walink_alloc(uint32_t meta, uint32_t size) noexcept;

// Deallocate a container previously allocated in wasm memory.
// The input must be a value whose payload is the container address.
WL_VALUE walink_free(WL_VALUE value);

} // extern "C"