#include "walink.h"

#include <stdint.h>
#include <stddef.h>
#include <string_view>

// Test-only C API implementations used for integration tests.
// 라이브러리 코드와 분리하기 위해 별도 TU 및 빌드 타깃에서만 사용됩니다.
// 메모리 할당/해제, 문자열/에러 생성은 walink 라이브러리의 공유 헬퍼를 사용합니다.

namespace walink_internal {

// walink 라이브러리 쪽에서 정의된 헬퍼 함수들 (cpp/src/walink.cpp)
// 여기서는 선언만 하고, 구현은 libwalink 에서 가져옵니다.

BaseContainer*    wl_alloc_container(uint64_t size) noexcept;
void              wl_free_container(BaseContainer* c) noexcept;
Float64Container* wl_alloc_f64(double v) noexcept;
void              wl_free_f64(Float64Container* c) noexcept;

WL_VALUE          wl_make_string(std::string_view sv,
                                 bool free_flag_for_receiver) noexcept;
WL_VALUE          wl_make_error(std::string_view msg) noexcept;

} // namespace walink_internal

namespace {

// --- Local WL_VALUE helpers (thin wrappers, no allocation logic) ------------

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

inline WL_VALUE wl_from_sint32(int32_t value) noexcept {
    const uint32_t payload = static_cast<uint32_t>(value);
    const uint32_t meta = wl_build_meta(WL_TAG_SINT32, /*is_address*/ false);
    return wl_make(meta, payload);
}

inline int32_t wl_to_sint32(WL_VALUE v) noexcept {
    return static_cast<int32_t>(wl_get_payload32(v));
}

} // namespace

extern "C" {

// --- Test-only exported APIs -------------------------------------------------
//
// NOTE:
//   이 함수들은 wasm 테스트 모듈(예: walink_wasm)에서만 export 되며,
//   라이브러리의 코어 ABI 에는 포함되지 않습니다.

WL_VALUE wl_roundtrip_bool(WL_VALUE value) {
    const uint32_t tag = wl_get_tag(value);
    const bool is_addr = wl_is_address(value);

    // BOOLEAN must be a direct value (is-address = 0)
    if (tag != WL_TAG_BOOLEAN || is_addr) {
        return walink_internal::wl_make_error("wl_roundtrip_bool: invalid tag");
    }

    return value;
}

WL_VALUE wl_add_sint32(WL_VALUE a, WL_VALUE b) {
    const std::int32_t av = wl_to_sint32(a);
    const std::int32_t bv = wl_to_sint32(b);
    const std::int32_t result = static_cast<std::int32_t>(av + bv);
    return wl_from_sint32(result);
}

WL_VALUE wl_make_hello_string() {
    constexpr std::string_view msg = "hello from wasm";
    return walink_internal::wl_make_string(msg, /*free_flag_for_receiver*/ true);
}

WL_VALUE wl_echo_string(WL_VALUE str_value) {
    const uint32_t tag = wl_get_tag(str_value);
    const bool is_addr = wl_is_address(str_value);

    if (tag != WL_TAG_STRING || !is_addr) {
        return walink_internal::wl_make_error("wl_echo_string: invalid tag");
    }
    return str_value;
}

} // extern "C"