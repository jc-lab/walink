//go:build wasm

package wlvalue

//export walink_callback
//go:wasmimport env walink_callback
func walinkCallback(v Value, argsPtr uint32, argsLen uint32) Value
