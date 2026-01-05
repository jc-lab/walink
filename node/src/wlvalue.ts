export type WlValue = bigint;

// ---- Tag and meta-bit layout (must mirror cpp/include/walink.h) ----

export enum WlTag {
    // direct values (is-address = 0)
    BOOLEAN = 0x10,
    SINT8 = 0x11,
    UINT8 = 0x21,
    SINT16 = 0x12,
    UINT16 = 0x22,
    SINT32 = 0x14,
    UINT32 = 0x24,
    FLOAT32 = 0x30,

    CALLBACK = 0x1000000,

    // address-based values (is-address = 1)
    FLOAT64 = 0x31,
    BYTES = 0x01,
    STRING = 0x02,
    MSGPACK = 0x0100,
    ERROR = 0x7fffff0,
}

// upper 32 bits meta layout
// bit 31: user-defined tag flag
// bit 30: is-address flag (1: payload is address, 0: direct value)
// bit 29: free flag (1: receiver must walink_free the address)
// bit 28: reserved
// bit 27-0: 28-bit tag value
export const WL_META_USER_DEFINED = 0x80000000;
export const WL_META_IS_ADDRESS = 0x40000000;
export const WL_META_FREE_FLAG = 0x20000000;
export const WL_META_TAG_MASK = 0x0fffffff;

const U32_MASK = 0xffffffffn;

export interface WlAddress {
    meta: number; // u32: meta
    ptr: number; // data pointer
    view: DataView; // data view
}

// ---- Low-level helpers: encode/decode WL_VALUE ----

export function getMeta(value: WlValue): number {
    return Number((value >> 32n) & U32_MASK);
}

export function getValueOrAddr(value: WlValue): number {
    return Number(value & U32_MASK);
}

export function getAddress(memory: ArrayBuffer, value: WlValue): WlAddress {
    if (!isAddress(value)) {
        throw new Error('Expected address-based value');
    }
 
    const ptr = getValueOrAddr(value);
    // WALink ABI: walink_alloc returns a data pointer as the payload.
    // Meta/tag bits are stored inside the WL_VALUE itself (upper 32 bits),
    // so read meta from the value, not from linear memory.
    return {
        ptr: ptr,
        meta: getMeta(value),
        view: new DataView(memory, ptr),
    }
}

export function makeMeta(
    tag: WlTag,
    isAddress = false,
    freeFlag = false,
    userDefined = false,
): number {
    let meta = Number(tag) & WL_META_TAG_MASK;
    if (userDefined) {
        meta |= WL_META_USER_DEFINED;
    }
    if (isAddress) {
        meta |= WL_META_IS_ADDRESS;
    }
    if (freeFlag) {
        meta |= WL_META_FREE_FLAG;
    }
    return meta >>> 0;
}

export function makeValue(meta: number, payload: number): WlValue {
    return (BigInt(meta >>> 0) << 32n) | (BigInt(payload >>> 0) & U32_MASK);
}

export function getTag(value: WlValue): WlTag {
    const meta = getMeta(value);
    const tagBits = meta & WL_META_TAG_MASK;
    return tagBits as WlTag;
}
 
// Null value helper (tag = 0)
export function wlNull(): WlValue {
    return makeValue(0, 0);
}

export function hasFreeFlag(value: WlValue): boolean {
    return (getMeta(value) & WL_META_FREE_FLAG) !== 0;
}

export function isAddress(value: WlValue): boolean {
    return (getMeta(value) & WL_META_IS_ADDRESS) !== 0;
}

export function toBool(value: WlValue): boolean {
    return (getValueOrAddr(value) & 1) !== 0;
}

export function fromBool(v: boolean): WlValue {
    const payload = v ? 1 : 0;
    const meta = makeMeta(WlTag.BOOLEAN, false, false, false);
    return makeValue(meta, payload);
}

export function fromSint8(v: number): WlValue {
    const int = v << 24 >> 24; // sign-extend to 32-bit
    const payload = int >>> 0;
    const meta = makeMeta(WlTag.SINT8, false, false, false);
    return makeValue(meta, payload);
}

export function toSint8(value: WlValue): number {
    return getValueOrAddr(value) << 24 >> 24;
}

export function fromUint8(v: number): WlValue {
    const uint = v & 0xff;
    const payload = uint >>> 0;
    const meta = makeMeta(WlTag.UINT8, false, false, false);
    return makeValue(meta, payload);
}

export function toUint8(value: WlValue): number {
    return getValueOrAddr(value) & 0xff;
}

export function fromSint16(v: number): WlValue {
    const int = v << 16 >> 16; // sign-extend to 32-bit
    const payload = int >>> 0;
    const meta = makeMeta(WlTag.SINT16, false, false, false);
    return makeValue(meta, payload);
}

export function toSint16(value: WlValue): number {
    return getValueOrAddr(value) << 16 >> 16;
}

export function fromUint16(v: number): WlValue {
    const uint = v & 0xffff;
    const payload = uint >>> 0;
    const meta = makeMeta(WlTag.UINT16, false, false, false);
    return makeValue(meta, payload);
}

export function toUint16(value: WlValue): number {
    return getValueOrAddr(value) & 0xffff;
}

export function fromUint32(v: number): WlValue {
    const uint = v >>> 0;
    const payload = uint >>> 0;
    const meta = makeMeta(WlTag.UINT32, false, false, false);
    return makeValue(meta, payload);
}

export function toUint32(value: WlValue): number {
    return getValueOrAddr(value) >>> 0;
}

export function fromFloat32(v: number): WlValue {
    const floatView = new Float32Array(1);
    const intView = new Uint32Array(floatView.buffer);
    floatView[0] = v;
    const payload = intView[0];
    const meta = makeMeta(WlTag.FLOAT32, false, false, false);
    return makeValue(meta, payload);
}

export function toFloat32(value: WlValue): number {
    const intView = new Uint32Array(1);
    const floatView = new Float32Array(intView.buffer);
    intView[0] = getValueOrAddr(value);
    return floatView[0];
}

export function fromSint32(v: number): WlValue {
    const int = v | 0;
    const payload = int >>> 0;
    const meta = makeMeta(WlTag.SINT32, false, false, false);
    return makeValue(meta, payload);
}

export function toSint32(value: WlValue): number {
    // force 32-bit signed range
    return (getValueOrAddr(value) << 0) | 0;
}
