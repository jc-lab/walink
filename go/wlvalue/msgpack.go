package wlvalue

import (
	"bytes"

	"github.com/vmihailenco/msgpack/v5"
)

type MsgpackOption func(d *msgpack.Decoder)

func MsgpackTag(tag string) MsgpackOption {
	return func(d *msgpack.Decoder) {
		d.SetCustomStructTag(tag)
	}
}

func DecodeMsgpack(v Value, allowFree bool, out interface{}, options ...MsgpackOption) error {
	raw := ToMsgpackBytes(v, allowFree)
	d := msgpack.NewDecoder(bytes.NewReader(raw))
	for _, option := range options {
		option(d)
	}
	return d.Decode(out)
}
