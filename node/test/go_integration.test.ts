import { readFile } from 'node:fs/promises';
import * as path from 'node:path';
import { fileURLToPath } from 'node:url';
import { beforeAll, describe, expect, it } from 'vitest';
import { WalinkHost, Walink } from '../src/walink';
import './wasm_exec.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

declare const Go: any;

async function loadGoWasmInstance(runtime: WalinkHost): Promise<{ instance: WebAssembly.Instance, go: any }> {
  const wasmPath = path.resolve(__dirname, './go-integration-test.wasm');
  const wasmBytes = await readFile(wasmPath);

  const go = new Go();
  // Go WASM expects walink_callback in 'env'
  go.importObject.env = {
    ...go.importObject.env,
    walink_callback: (v: bigint, argsPtr: number, argsLen: number) => {
      return runtime.goCallbackHandler(v, argsPtr, argsLen);
    }
  };

  const result = await WebAssembly.instantiate(wasmBytes, go.importObject);
  go.run(result.instance); // Start Go runtime
  return { instance: result.instance, go };
}

describe('Go WASM integration', () => {
  let walink: Walink;
  let runtime: WalinkHost;

  beforeAll(async () => {
    runtime = new WalinkHost();
    const { instance } = await loadGoWasmInstance(runtime);
    console.error('instance : ', instance.exports);
    walink = runtime.createWalinkFromInstance(instance, {
      memory: instance.exports['mem'] as any,
    });
  });

  it('adds numbers through Go WASM', () => {
    const exports = (walink as any).exports;
    const a = walink.toWlSint32(10);
    const b = walink.toWlSint32(20);
    const result = exports.add(a, b);
    expect(walink.fromWlSint32(result)).toBe(30);
  });

  it('echoes string through Go WASM', () => {
    const exports = (walink as any).exports;
    const input = 'Hello from Node.js';
    const vInput = walink.toWlString(input);
    const vResult = exports.echo_string(vInput);
    expect(walink.fromWlString(vResult)).toBe(input);
  });

  it('calls back to Node.js from Go WASM', () => {
    const exports = (walink as any).exports;
    let called = false;
    let receivedVal: any = null;

    const cb = walink.registerCallback((v: bigint) => {
      called = true;
      receivedVal = walink.decode(v);
      return walink.toWlSint32(123);
    });

    const inputVal = walink.toWlSint32(456);
    const result = exports.call_me_back(cb, inputVal);

    expect(called).toBe(true);
    expect(receivedVal).toBe(456);
    expect(walink.fromWlSint32(result)).toBe(123);
  });
});
