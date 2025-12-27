import { readFile } from 'node:fs/promises';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';

import { beforeAll, describe, expect, it } from 'vitest';

import { createWalinkWithSampleApi, WalinkWithSampleApi } from './walinkSampleApi';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function loadWasmInstance(): Promise<WebAssembly.Instance> {
  const wasmPath = path.resolve(__dirname, '../../cpp/build/walink_test.wasm');
  const bytes = await readFile(wasmPath);

  const wasmImports = {
    emscripten_notify_memory_growth: (memoryIndex) => {
      console.log('emscripten_notify_memory_growth: ', memoryIndex);
    },
  };
  const result = await WebAssembly.instantiate(bytes, {
    'env': wasmImports,
    'wasi_snapshot_preview1': wasmImports,
  });
  return result.instance;
}

describe('walink wasm integration', () => {
  let walink: WalinkWithSampleApi;

  beforeAll(async () => {
    const instance = await loadWasmInstance();
    walink = createWalinkWithSampleApi(instance);
  });

  it('roundtrips boolean values', () => {
    expect(walink.roundtripBool(true)).toBe(true);
    expect(walink.roundtripBool(false)).toBe(false);
  });

  it('adds sint32 values', () => {
    expect(walink.addSint32(1, 2)).toBe(3);
    expect(walink.addSint32(-10, 5)).toBe(-5);
  });

  it('creates hello string from wasm', () => {
    const str = walink.makeHelloString();
    expect(str).toBe('hello from wasm');
  });

  it('echoes hello string through wasm', () => {
    const str = walink.echoHelloString();
    expect(str).toBe('hello from wasm');
  });
});