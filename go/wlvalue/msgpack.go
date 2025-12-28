package wlvalue

import (
	"bytes"

	"github.com/vmihailenco/msgpack/v5"
)

type MsgpackOption func(d *msgpack.Decoder) error

func MsgpackTag(tag string) MsgpackOption {
	return func(d *msgpack.Decoder) error {
		d.SetCustomStructTag(tag)
		return nil
	}
}

func DecodeMsgpack(v Value, allowFree bool, out interface{}, options ...MsgpackOption) error {
	raw := ToMsgpackBytes(v, allowFree)
	d := msgpack.NewDecoder(bytes.NewReader(raw))
	for _, option := range options {
		if err := option(d); err != nil {
			return err
		}
	}
	if out == nil {
		return nil
	}
	return d.Decode(out)
}
