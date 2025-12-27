package wlvalue

import (
	"fmt"
	"unsafe"
)

type CallbackFunc func(args ...Value) Value

func (v Value) ToFunction() CallbackFunc {
	if !v.IsFunction() {
		panic(fmt.Errorf("value(0x%x) is not a function", v))
	}
	return func(args ...Value) Value {
		var argsPtr uint32
		if len(args) > 0 {
			argsPtr = uint32(uintptr(unsafe.Pointer(&args[0])))
		}
		return walinkCallback(v, argsPtr, uint32(len(args)))
	}
}
