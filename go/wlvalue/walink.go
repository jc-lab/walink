package wlvalue

import (
	"errors"
	"math"
	"sync"
	"unsafe"
)

type Value uint64

type Tag uint32

const (
	TagNull    Tag = 0x0
	TagBoolean Tag = 0x10
	TagSint8   Tag = 0x11
	TagUint8   Tag = 0x21
	TagSint16  Tag = 0x12
	TagUint16  Tag = 0x22
	TagSint32  Tag = 0x14
	TagUint32  Tag = 0x24
	TagFloat32 Tag = 0x30

	TagFloat64  Tag = 0x31
	TagBytes    Tag = 0x01
	TagString   Tag = 0x02
	TagMsgpack  Tag = 0x0100
	TagError    Tag = 0xffffff0
	TagFunction Tag = 0x1000000
)

const (
	MetaUserDefined uint32 = 0x80000000
	MetaIsAddress   uint32 = 0x40000000
	MetaFreeFlag    uint32 = 0x20000000
	MetaTagMask     uint32 = 0x0FFFFFFF
)

var nullPointer = unsafe.Pointer(uintptr(0))

type baseContainer struct {
	cap  uint32
	size uint32
	data [0]byte
}

type float64Container struct {
	v float64
}

func GetMeta(v Value) uint32 {
	return uint32(v >> 32)
}

func GetPayload32(v Value) uint32 {
	return uint32(v & 0xffffffff)
}

func GetPointer(v Value) unsafe.Pointer {
	return unsafe.Pointer(uintptr(v & 0xffffffff))
}

func MakeValue(meta uint32, payload uint32) Value {
	return Value(uint64(meta)<<32 | uint64(payload))
}

func GetTag(v Value) Tag {
	return Tag(GetMeta(v) & MetaTagMask)
}

func IsAddress(v Value) bool {
	return (GetMeta(v) & MetaIsAddress) != 0
}

func (v Value) IsFunction() bool {
	return GetTag(v) == TagFunction
}

func BuildMeta(tag Tag, isAddress bool, freeFlag bool, userDefined bool) uint32 {
	meta := uint32(tag) & MetaTagMask
	if userDefined {
		meta |= MetaUserDefined
	}
	if isAddress {
		meta |= MetaIsAddress
	}
	if freeFlag {
		meta |= MetaFreeFlag
	}
	return meta
}

func FromAddress(ptr unsafe.Pointer, tag Tag, freeFlag bool) Value {
	payload := uint32(uintptr(ptr))
	meta := BuildMeta(tag, true, freeFlag, false)
	return MakeValue(meta, payload)
}

// ---- Registry for GC retention ----

var (
	registryMu sync.Mutex
	registry   = make(map[unsafe.Pointer]interface{})
)

func retain(ptr unsafe.Pointer, obj interface{}) {
	registryMu.Lock()
	defer registryMu.Unlock()
	registry[ptr] = obj
}

func release(ptr unsafe.Pointer, free bool) interface{} {
	registryMu.Lock()
	defer registryMu.Unlock()
	obj, ok := registry[ptr]
	if ok && free {
		delete(registry, ptr)
	}
	return obj
}

func getPointer(v Value, allowFree bool) unsafe.Pointer {
	if !IsAddress(v) {
		panic(errors.New("no address"))
	}
	ptr := GetPointer(v)
	if ptr == nullPointer {
		return nullPointer
	}
	needFree := (GetMeta(v) & MetaFreeFlag) != 0
	if allowFree && needFree {
		release(ptr, true)
	}
	return ptr
}

// ---- Factories ----

func FromBool(b bool) Value {
	var payload uint32
	if b {
		payload = 1
	}
	meta := BuildMeta(TagBoolean, false, false, false)
	return MakeValue(meta, payload)
}

func FromInt8(v int8) Value {
	meta := BuildMeta(TagSint8, false, false, false)
	return MakeValue(meta, uint32(int32(v)))
}

func ToInt8(v Value) int8 {
	return int8(int32(GetPayload32(v)))
}

func FromUint8(v uint8) Value {
	meta := BuildMeta(TagUint8, false, false, false)
	return MakeValue(meta, uint32(v))
}

func ToUint8(v Value) uint8 {
	return uint8(GetPayload32(v))
}

func FromInt16(v int16) Value {
	meta := BuildMeta(TagSint16, false, false, false)
	return MakeValue(meta, uint32(int32(v)))
}

func ToInt16(v Value) int16 {
	return int16(int32(GetPayload32(v)))
}

func FromUint16(v uint16) Value {
	meta := BuildMeta(TagUint16, false, false, false)
	return MakeValue(meta, uint32(v))
}

func ToUint16(v Value) uint16 {
	return uint16(GetPayload32(v))
}

func FromInt32(v int32) Value {
	meta := BuildMeta(TagSint32, false, false, false)
	return MakeValue(meta, uint32(v))
}

func ToInt32(v Value) int32 {
	return int32(GetPayload32(v))
}

func FromUint32(v uint32) Value {
	meta := BuildMeta(TagUint32, false, false, false)
	return MakeValue(meta, v)
}

func ToUint32(v Value) uint32 {
	return GetPayload32(v)
}

func FromFloat32(f float32) Value {
	payload := math.Float32bits(f)
	meta := BuildMeta(TagFloat32, false, false, false)
	return MakeValue(meta, payload)
}

func ToFloat32(v Value) float32 {
	return math.Float32frombits(GetPayload32(v))
}

func Null() Value {
	return MakeValue(0, 0)
}

// ---- Address-based ----

func allocContainer(size uint32) (*baseContainer, []byte) {
	buf := make([]byte, 8+size)
	c := (*baseContainer)(unsafe.Pointer(&buf[0]))
	c.cap = size
	c.size = 0
	return c, buf[8:]
}

func MakeString(s string, freeFlag bool) Value {
	c, buf := allocContainer(uint32(len(s)))
	c.size = uint32(len(s))
	if len(s) > 0 {
		copy(buf, s)
	}
	ptr := unsafe.Pointer(c)
	retain(ptr, c)
	return FromAddress(ptr, TagString, freeFlag)
}

func MakeBytes(b []byte, freeFlag bool) Value {
	c, buf := allocContainer(uint32(len(b)))
	c.size = uint32(len(b))
	if len(b) > 0 {
		copy(buf, b)
	}
	ptr := unsafe.Pointer(c)
	retain(ptr, c)
	return FromAddress(ptr, TagBytes, freeFlag)
}

func MakeMsgpack(b []byte, freeFlag bool) Value {
	c, buf := allocContainer(uint32(len(b)))
	c.size = uint32(len(b))
	if len(b) > 0 {
		copy(buf, b)
	}
	ptr := unsafe.Pointer(c)
	retain(ptr, c)
	return FromAddress(ptr, TagMsgpack, freeFlag)
}

func MakeError(msg string) Value {
	c, buf := allocContainer(uint32(len(msg)))
	c.size = uint32(len(msg))
	if len(msg) > 0 {
		copy(buf, msg)
	}
	ptr := unsafe.Pointer(c)
	retain(ptr, c)
	return FromAddress(ptr, TagError, true)
}

func MakeFloat64(f float64, freeFlag bool) Value {
	c := new(float64Container)
	c.v = f
	ptr := unsafe.Pointer(c)
	retain(ptr, c)
	return FromAddress(ptr, TagFloat64, freeFlag)
}

// ---- Converters ----

func ToString(v Value, allowFree bool) string {
	if !IsAddress(v) || GetTag(v) != TagString {
		panic("walink: expected address-based STRING tag")
	}

	payload := getPointer(v, allowFree)
	if payload == nullPointer {
		return ""
	}

	c := (*baseContainer)(payload)

	var res string
	if c.size > 0 {
		res = string(unsafe.Slice((*byte)(unsafe.Pointer(&c.data)), c.size))
	}

	if allowFree && (GetMeta(v)&MetaFreeFlag) != 0 {
		WalinkFree(v)
	}
	return res
}

func ToBytes(v Value, allowFree bool) []byte {
	if !IsAddress(v) || GetTag(v) != TagBytes {
		panic("walink: expected address-based BYTES tag")
	}

	payload := getPointer(v, allowFree)
	if payload == nullPointer {
		return nil
	}

	c := (*baseContainer)(payload)

	res := make([]byte, c.size)
	if c.size > 0 {
		copy(res, unsafe.Slice((*byte)(unsafe.Pointer(&c.data)), c.size))
	}
	if allowFree && (GetMeta(v)&MetaFreeFlag) != 0 {
		WalinkFree(v)
	}
	return res
}

func ToMsgpack(v Value, allowFree bool) []byte {
	if !IsAddress(v) || GetTag(v) != TagMsgpack {
		panic("walink: expected address-based MSGPACK tag")
	}

	payload := getPointer(v, allowFree)
	if payload == nullPointer {
		return nil
	}

	c := (*baseContainer)(payload)

	res := make([]byte, c.size)
	if c.size > 0 {
		copy(res, unsafe.Slice((*byte)(unsafe.Pointer(&c.data)), c.size))
	}
	if allowFree && (GetMeta(v)&MetaFreeFlag) != 0 {
		WalinkFree(v)
	}
	return res
}

func ToFloat64(v Value, allowFree bool) float64 {
	if !IsAddress(v) || GetTag(v) != TagFloat64 {
		panic("walink: expected address-based FLOAT64 tag")
	}

	payload := getPointer(v, allowFree)
	if payload == nullPointer {
		return 0
	}

	c := (*float64Container)(payload)

	res := c.v
	if allowFree && (GetMeta(v)&MetaFreeFlag) != 0 {
		WalinkFree(v)
	}
	return res
}

// ---- Exported C API ----

//export walink_alloc
//go:wasmexport walink_alloc
func WalinkAlloc(size uint32) Value {
	buf := make([]byte, size)
	ptr := unsafe.Pointer(&buf[0])
	retain(ptr, buf)
	return MakeValue(0, uint32(uintptr(ptr)))
}

//export walink_free
//go:wasmexport walink_free
func WalinkFree(v Value) Value {
	if !IsAddress(v) {
		return FromBool(false)
	}
	payload := GetPayload32(v)
	release(unsafe.Pointer(uintptr(payload)), true)
	return FromBool(true)
}
