import {
  type WlValue,
  Walink,
  WalinkCoreExports, WalinkHost,
} from '../src';

// wasm 테스트 모듈이 export 하는 테스트용 C API 시그니처
export interface WalinkTestExports extends WalinkCoreExports {
  wl_roundtrip_bool(value: WlValue): WlValue;
  wl_add_sint32(a: WlValue, b: WlValue): WlValue;
  wl_make_hello_string(): WlValue;
  wl_echo_string(value: WlValue): WlValue;
}

// 라이브러리 코어 Walink 위에 테스트용 샘플 API 래퍼를 올린 클래스
export class WalinkWithSampleApi extends Walink {
  protected readonly testExports: WalinkTestExports;

  constructor(exports: any) {
    super(new WalinkHost(), {
      exports: exports,
      memory: exports.memory,
    });
    this.testExports = exports;
  }

  roundtripBool(v: boolean): boolean {
    const input = this.toWlBool(v);
    const result = this.testExports.wl_roundtrip_bool(input);
    // tag 검사는 Walink.fromWlBool 이 처리
    return this.fromWlBool(result);
  }

  addSint32(a: number, b: number): number {
    const av = this.toWlSint32(a);
    const bv = this.toWlSint32(b);
    const result = this.testExports.wl_add_sint32(av, bv);
    return this.fromWlSint32(result);
  }

  makeHelloString(): string {
    const value = this.testExports.wl_make_hello_string();
    return this.fromWlString(value);
  }

  echoHelloString(): string {
    const hello = this.testExports.wl_make_hello_string();
    const echoed = this.testExports.wl_echo_string(hello);
    return this.fromWlString(echoed);
  }
}

// 통합 테스트에서 사용할 편의 생성 함수
export function createWalinkWithSampleApi(
  instance: WebAssembly.Instance,
): WalinkWithSampleApi {
  const exports = instance.exports as unknown as WalinkTestExports;
  if (!(exports.memory instanceof WebAssembly.Memory)) {
    throw new Error('walink: wasm instance.exports.memory must be a WebAssembly.Memory');
  }
  return new WalinkWithSampleApi(exports);
}