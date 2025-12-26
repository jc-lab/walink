//go:build !wasm

package wlvalue

func walinkCallback(v Value, argsPtr uint32, argsLen uint32) Value {
	panic("walinkCallback is only available in wasm")
}
