cmake_minimum_required(VERSION 3.20)

project(walink_test
    VERSION 0.0.1
    LANGUAGES C CXX
)

# Core walink static library (pure C++20, no tests here)
add_library(walink STATIC
    src/walink.cc
)

target_include_directories(walink
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(walink PUBLIC cxx_std_20)

# Emscripten wasm target (standalone .wasm, no JS glue)
#
# 빌드 예시:
#   emcmake cmake -B build -S cpp
#   cmake --build build
#
# 결과물:
#   build/walink_test.wasm
#
# 이 wasm 은 WebAssembly.instantiate(...) 로 직접 로드할 수 있으며,
# Node 통합 테스트에서 사용됩니다.
#
# walink_test 타깃:
#   - 소스: tests/walink_test_api.cpp (테스트 전용 C API)
#   - 링크: libwalink (src/walink.cpp; walink_free, wl_make_string, wl_make_error 등)
option(WALINK_TEST_BUILD "Build walink wasm module (requires Emscripten)" ON)

if (WALINK_TEST_BUILD)
    # Emscripten 여부를 간단히 확인
    if (CMAKE_CXX_COMPILER MATCHES "em\\+\\+|emcc")
        add_executable(walink_test
            tests/walink_test_api.cpp
        )

        target_link_libraries(walink_test
            PRIVATE
                walink
        )

        target_include_directories(walink_test
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

        target_compile_features(walink_test PUBLIC cxx_std_20)
        target_compile_options(walink_test
            PRIVATE
                -fno-exceptions
        )

        # standalone wasm: no JS glue, exported C 함수와 memory 만 사용
        # -sWASM_BIGINT=1: JS <-> wasm i64 인자/리턴을 BigInt로 직접 교환하기 위함
        target_link_options(walink_test
            PRIVATE
                "-sSTANDALONE_WASM=1"
                "-sALLOW_MEMORY_GROWTH=1"
                "-sERROR_ON_UNDEFINED_SYMBOLS=0"
                "-sWASM_BIGINT=1"
                "-sEXPORTED_FUNCTIONS=['_walink_free','_wl_roundtrip_bool','_wl_add_sint32','_wl_make_hello_string','_wl_echo_string']"
                "-sEXPORTED_RUNTIME_METHODS=[]"
                "--no-entry"
        )
    else ()
        message(WARNING "WALINK_TEST_BUILD is ON but compiler does not look like Emscripten (em++/emcc). Skipping wasm target.")
    endif ()
endif ()
