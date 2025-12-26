package wlvalue

import (
	"testing"
)

func TestValueLayout(t *testing.T) {
	v := FromInt32(123)
	if GetTag(v) != TagSint32 {
		t.Errorf("Expected TagSint32, got %v", GetTag(v))
	}
	if ToInt32(v) != 123 {
		t.Errorf("Expected 123, got %v", ToInt32(v))
	}

	b := FromBool(true)
	if GetTag(b) != TagBoolean {
		t.Errorf("Expected TagBoolean, got %v", GetTag(b))
	}
	if GetPayload32(b) != 1 {
		t.Errorf("Expected payload 1, got %v", GetPayload32(b))
	}
}

func TestString(t *testing.T) {
	s := "Hello Walink"
	v := MakeString(s, false)
	if GetTag(v) != TagString {
		t.Errorf("Expected TagString, got %v", GetTag(v))
	}
	if !IsAddress(v) {
		t.Errorf("Expected address-based value")
	}

	res := ToString(v, false)
	if res != s {
		t.Errorf("Expected %s, got %s", s, res)
	}
}

func TestFloat64(t *testing.T) {
	f := 3.1415926535
	v := MakeFloat64(f, false)
	if GetTag(v) != TagFloat64 {
		t.Errorf("Expected TagFloat64, got %v", GetTag(v))
	}

	res := ToFloat64(v, false)
	if res != f {
		t.Errorf("Expected %v, got %v", f, res)
	}
}
