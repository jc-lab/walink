# walink

Wasm ↔ host(Node/Browser) 간에 64비트 `WL_VALUE` 기반으로 다양한 typed value 를 전달하기 위한 라이브러리입니다.

- `./cpp/` : C++20 + Emscripten + CMake 기반 wasm 측 라이브러리
- `./node/` : TypeScript + rolldown 기반 Node/브라우저 호스트 라이브러리

테스트 코드와 라이브러리 코드는 각각 분리되어 있습니다.

## WL_VALUE 개요

`WL_VALUE` 는 64비트 부호 없는 정수입니다.

```text
[63 ............. 32][31 ......................... 0]
       meta(상위 32비트)          payload(하위 32비트)
```

### meta(상위 32비트)

- bit 31: `user-defined tag` 플래그 (`WL_META_USER_DEFINED`)
- bit 30: `is-address` 플래그. 하위 32bit 값이 address 인지 여부. `1` 인 경우 address 이며, `0` 인 경우 direct value 임. (`WL_META_IS_ADDRESS`)
- bit 29: 수신자가 payload 로 가리키는 address 를 `walink_free` 로 해제해야 하는지 여부 (`WL_META_FREE_FLAG`)
- bit 28: reserved
- bit 27~0: 28bit tag 값 (`WL_META_TAG_MASK`)

### payload(하위 32비트)

- `is-address = 0` 인 경우: 32비트로 직접 표현 가능한 값(boolean, ~32bit 정수, float32 등)은 그대로 value 를 저장합니다.
- `is-address = 1` 인 경우: wasm 메모리 내 컨테이너의 주소(32bit offset)를 저장합니다. (모든 BaseContainer 기반 타입, `Float64Container` 포함)

### Tag 값

Tag (is-address = 0):

- `0x10` : boolean
- `0x11` : sint8
- `0x21` : uint8
- `0x12` : sint16
- `0x22` : uint16
- `0x14` : sint32
- `0x24` : uint32
- `0x30` : float32

Tag (is-address = 1):

- `0x31` : float64 (`Float64Container*`)
- `0x01` : Bytes (= BaseContainer, Node 에서는 Buffer)
- `0x02` : String (= BaseContainer)
- `0x100` : Object (= BaseContainer, MsgPack 직렬화, Node 에서는 Object)
- `0x7fffff0` : Error (= BaseContainer, 문자열 오류 메세지, host 에서는 예외로 throw)

## BaseContainer ABI

```c
struct BaseContainer {
  uint64_t cap;
  uint64_t size;
  uint8_t  data[];
};
```

- `cap` : data 버퍼의 총 용량
- `size` : 실제 사용 중인 길이
- `data` : `size` 바이트의 payload

wasm 측에서 BaseContainer 를 할당하며, `WL_VALUE` 의 free 플래그가 1 인 경우 수신자(host 또는 wasm)가 `walink_free` 를 호출하여 해제해야 합니다.

추가로, `is-address = 1` 이고 tag 가 `0x31` 인 경우에는 다음과 같은 `Float64Container` 구조를 사용합니다.

```c
struct Float64Container {
  double v;
};
```

- 하위 32bit payload 는 `Float64Container` 가 위치한 wasm 메모리의 주소입니다.
- 수신자는 free 플래그가 1 인 경우 `walink_free` 를 호출하여 이 컨테이너를 해제해야 합니다.

# 동적 메모리 할당

```
WL_VALUE walink_alloc(uint32_t meta, uint32_t size);
WL_VALUE walink_free(WL_VALUE value);
```

# License

Apache-2.0

