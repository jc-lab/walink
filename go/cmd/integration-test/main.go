package main

import (
	"github.com/jc-lab/walink/go/wlvalue"
)

//export add
//go:wasmexport add
func add(a, b wlvalue.Value) wlvalue.Value {
	return wlvalue.FromInt32(wlvalue.ToInt32(a) + wlvalue.ToInt32(b))
}

//export echo_string
//go:wasmexport echo_string
func echoString(v wlvalue.Value) wlvalue.Value {
	s := wlvalue.ToString(v, true)
	return wlvalue.MakeString(s, true)
}

//export call_me_back
//go:wasmexport call_me_back
func callMeBack(cb wlvalue.Value, val wlvalue.Value) wlvalue.Value {
	f := cb.ToFunction()
	return f(val)
}

func main() {}
