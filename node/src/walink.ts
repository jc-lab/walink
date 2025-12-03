import {
  type WlValue,
  type WlAddress,
  WlTag,
  hasFreeFlag,
  getTag,
  getValueOrAddr,
  toBool,
  toSint8,
  toUint8,
  toSint16,
  toUint16,
  toSint32,
  toUint32,
  toFloat32,
  fromBool,
  fromSint8,
  fromUint8,
  fromSint16,
  fromUint16,
  fromUint32,
  fromFloat32,
  fromSint32,
  makeMeta,
  makeValue,
  getAddress,
} from './wlvalue';

import { pack, unpack } from 'msgpackr';

const BaseContainerSize = 8;

export abstract class BaseContainer {
  // cap: u32
  // size: u32

  public abstract get cap(): number;

  public abstract get size(): number;

  public abstract set size(v: number);

  public abstract get view(): DataView;

  public get viewAsUint8Array(): Uint8Array {
    return new Uint8Array(this.view.buffer, this.view.byteOffset, this.size);
  }

  public abstract getRawView(): DataView;
}

export class CopiedBaseContainer extends BaseContainer {
  constructor(public readonly buffer: Uint8Array) {
    super();
  }

  public get cap(): number {
    return this.buffer.length;
  }

  public get size(): number {
    return this.buffer.length;
  }

  public set size(v: number) {
    throw new Error('readonly');
  }

  public get view(): DataView {
    return new DataView(this.buffer.buffer, this.buffer.byteOffset, this.buffer.length);
  }

  public getRawView(): DataView {
    return this.view;
  }
}

export class BaseContainerView extends BaseContainer {
  // cap: u32
  // size: u32

  constructor(public readonly addr: WlAddress, cap?: number) {
    super();
    if (cap) {
      this.addr.view.setUint32(0, cap, true);
      this.addr.view.setUint32(4, 0, true);
    }
  }

  public get cap(): number {
    return this.addr.view.getUint32(0, true);
  }

  public get size(): number {
    return this.addr.view.getUint32(4, true);
  }

  public set size(v: number) {
    this.addr.view.setUint32(4, v, true);
  }

  public get view(): DataView {
    return new DataView(this.addr.view.buffer, this.addr.view.byteOffset + BaseContainerSize, this.size);
  }

  public getRawView(): DataView {
    return new DataView(this.addr.view.buffer, this.addr.view.byteOffset + BaseContainerSize);
  }
}

// ---- High-level core wasm exports interface (library-agnostic) ----

export interface WalinkCoreExports {
  memory: WebAssembly.Memory;
  // WL_VALUE walink_alloc(uint32_t meta, uint32_t size);
  walink_alloc(meta: number, size: number): WlValue;
  // WL_VALUE walink_free(WL_VALUE value);
  walink_free(value: WlValue): WlValue;
}

export interface WalinkOptions {
  exports: WalinkCoreExports;
}

// Core Walink runtime: generic WL_VALUE helpers bound to a wasm instance.
export class Walink {
  protected readonly exports: WalinkCoreExports;
  protected readonly memory: WebAssembly.Memory;
  private readonly textDecoder: TextDecoder;

  constructor(options: WalinkOptions) {
    this.exports = options.exports;
    this.memory = options.exports.memory;
    this.textDecoder = new TextDecoder('utf-8');
  }

  public wlValueGetAddress(value: WlValue): WlAddress {
    return getAddress(this.memory.buffer, value);
  }

  public wlValueAllocate(meta: number, size: number): WlAddress {
    const wlValue = this.exports.walink_alloc(meta, size);
    if (!wlValue) {
      throw new Error(`memory allocate failed (size: ${size})`);
    }
    const ptr = getValueOrAddr(wlValue);
    return {
      meta: meta,
      ptr: ptr,
      view: new DataView(this.memory.buffer, ptr),
    }
  }

  newBaseContainer(meta: number, cap: number): BaseContainerView {
    const addr = this.wlValueAllocate(meta, BaseContainerSize + cap);
    return new BaseContainerView(addr, cap);
  }

  toWlBaseContainerValue(meta: number, bytes: Uint8Array): WlValue {
    const baseContainer = this.newBaseContainer(meta, bytes.length);
    baseContainer.size = bytes.length;
    baseContainer.viewAsUint8Array.set(bytes);
    return makeValue(baseContainer.addr.meta, baseContainer.addr.ptr);
  }

  fromWlBaseContainer(value: WlValue, autoFree: boolean): BaseContainer {
    const addr = getAddress(this.memory.buffer, value);
    const view = new BaseContainerView(addr);
    if (!autoFree) {
      return view;
    }
    const copiedBuffer = new Uint8Array(view.size);
    copiedBuffer.set(view.viewAsUint8Array);
    if (hasFreeFlag(value)) {
      this.exports.walink_free(value);
    }
    return new CopiedBaseContainer(copiedBuffer);
  }

  // ---- Public helpers for scalars (encoders use low-level from wlvalue) ----
  toWlBool(v: boolean): WlValue {
    return fromBool(v); // low-level encoder
  }

  toWlSint8(v: number): WlValue {
    return fromSint8(v);
  }

  toWlUint8(v: number): WlValue {
    return fromUint8(v);
  }

  toWlSint16(v: number): WlValue {
    return fromSint16(v);
  }

  toWlUint16(v: number): WlValue {
    return fromUint16(v);
  }

  toWlSint32(v: number): WlValue {
    return fromSint32(v);
  }

  toWlUint32(v: number): WlValue {
    return fromUint32(v);
  }

  toWlFloat32(v: number): WlValue {
    return fromFloat32(v);
  }

  toWlFloat64(v: number): WlValue {
    const meta = makeMeta(WlTag.FLOAT64, true, true, false);
    const result = this.wlValueAllocate(meta, 8);
    result.view.setFloat64(0, v);
    return makeValue(result.meta, result.ptr);
  }

  toWlBytes(bytes: Uint8Array): WlValue {
    const meta = makeMeta(WlTag.BYTES, true, true, false);
    return this.toWlBaseContainerValue(meta, bytes);
  }

  toWlMsgpack(obj: unknown): WlValue {
    const meta = makeMeta(WlTag.MSGPACK, true, true, false);
    // Accept either a pre-serialized Uint8Array or a plain JS object.
    // If it's an object, automatically serialize it with msgpackr.pack.
    if (obj instanceof Uint8Array) {
      return this.toWlBaseContainerValue(meta, obj);
    }
    // serialize arbitrary JS values to msgpack bytes
    const bytes = pack(obj as any) as Uint8Array;
    return this.toWlBaseContainerValue(meta, bytes);
  }

  // ---- Public validated scalar decoders (use low-level to* from wlvalue after tag check) ----
  fromWlBool(value: WlValue): boolean {
    if (getTag(value) !== WlTag.BOOLEAN) {
      throw new Error(`Expected BOOLEAN tag, got 0x${getTag(value).toString(16)}`);
    }
    return toBool(value);
  }

  fromWlSint8(value: WlValue): number {
    if (getTag(value) !== WlTag.SINT8) {
      throw new Error(`Expected SINT8 tag, got 0x${getTag(value).toString(16)}`);
    }
    return toSint8(value);
  }

  fromWlUint8(value: WlValue): number {
    if (getTag(value) !== WlTag.UINT8) {
      throw new Error(`Expected UINT8 tag, got 0x${getTag(value).toString(16)}`);
    }
    return toUint8(value);
  }

  fromWlSint16(value: WlValue): number {
    if (getTag(value) !== WlTag.SINT16) {
      throw new Error(`Expected SINT16 tag, got 0x${getTag(value).toString(16)}`);
    }
    return toSint16(value);
  }

  fromWlUint16(value: WlValue): number {
    if (getTag(value) !== WlTag.UINT16) {
      throw new Error(`Expected UINT16 tag, got 0x${getTag(value).toString(16)}`);
    }
    return toUint16(value);
  }

  fromWlSint32(value: WlValue): number {
    if (getTag(value) !== WlTag.SINT32) {
      throw new Error(`Expected SINT32 tag, got 0x${getTag(value).toString(16)}`);
    }
    return toSint32(value);
  }

  fromWlUint32(value: WlValue): number {
    if (getTag(value) !== WlTag.UINT32) {
      throw new Error(`Expected UINT32 tag, got 0x${getTag(value).toString(16)}`);
    }
    return toUint32(value);
  }

  fromWlFloat32(value: WlValue): number {
    if (getTag(value) !== WlTag.FLOAT32) {
      throw new Error(`Expected FLOAT32 tag, got 0x${getTag(value).toString(16)}`);
    }
    return toFloat32(value);
  }

  // ---- Address-based decoders ----

  fromWlFloat64(value: WlValue): number {
    if (getTag(value) !== WlTag.FLOAT64) {
      throw new Error(`Expected FLOAT64 tag, got 0x${getTag(value).toString(16)}`);
    }
    const addr = this.wlValueGetAddress(value);
    return addr.view.getFloat64(0, true);
  }

  fromWlBytes(value: WlValue): Uint8Array {
    const container = this.fromWlBaseContainer(value, true);
    return container.viewAsUint8Array;
  }

  fromWlMsgpack<T = any>(value: WlValue): T {
    const tag = getTag(value);
    if (tag !== WlTag.MSGPACK) {
      throw new Error(`Expected MSGPACK tag, got 0x${tag.toString(16)}`);
    }
    const container = this.fromWlBaseContainer(value, false);
    try {
      // unpack returns any; cast to Record<string, any> for callers
      return unpack(container.viewAsUint8Array) as T;
    } catch (e) {
      // If msgpack parsing fails, surface as an error
      throw new Error(`fromWlMsgpack: msgpack unpack failed: ${(e as Error).message}`);
    } finally {
      if (hasFreeFlag(value)) {
        this.exports.walink_free(value);
      }
    }
  }

  fromWlString(value: WlValue, ignoreTag?: boolean): string {
    const tag = getTag(value);
    if (!ignoreTag && tag !== WlTag.STRING) {
      throw new Error(`Expected STRING tag, got 0x${tag.toString(16)}`);
    }
    const container = this.fromWlBaseContainer(value, false);
    try {
      return this.textDecoder.decode(container.viewAsUint8Array);
    } finally {
      if (hasFreeFlag(value)) {
        this.exports.walink_free(value);
      }
    }
  }

  // ---- Generic decode helper for common tags ----

  decode(value: WlValue): unknown {
    const tag = getTag(value);
    switch (tag) {
      case WlTag.BOOLEAN:
        return this.fromWlBool(value);
      case WlTag.SINT8:
        return this.fromWlSint8(value);
      case WlTag.UINT8:
        return this.fromWlUint8(value);
      case WlTag.SINT16:
        return this.fromWlSint16(value);
      case WlTag.UINT16:
        return this.fromWlUint16(value);
      case WlTag.SINT32:
        return this.fromWlSint32(value);
      case WlTag.UINT32:
        return this.fromWlUint32(value);
      case WlTag.FLOAT32:
        return this.fromWlFloat32(value);
      case WlTag.FLOAT64:
        return this.fromWlFloat64(value);
      case WlTag.BYTES:
        return this.fromWlBytes(value);
      case WlTag.STRING:
        return this.fromWlString(value);
      case WlTag.MSGPACK:
        return this.fromWlMsgpack(value);
      case WlTag.ERROR:
        const text = this.fromWlString(value, true);
        throw new Error(text);
      default:
        return value; // raw for unsupported
    }
  }
}

// Convenience helper to build Walink from a raw WebAssembly.Instance
export function createWalinkFromInstance(instance: WebAssembly.Instance): Walink {
  const exports = instance.exports as unknown as WalinkCoreExports;
  if (!(exports.memory instanceof WebAssembly.Memory)) {
    throw new Error('walink: wasm instance.exports.memory must be a WebAssembly.Memory');
  }
  return new Walink({ exports });
}
