// BLE Protocol constants
// These match the firmware exactly

export const CMD_SCAN = 0x01;
export const CMD_PAIR = 0x02;

export const RESP_SCAN_RESULT = 0x11;
export const RESP_PAIR_RESULT = 0x12;
export const RESP_ERROR = 0x13;

export const ERR_NO_TAG = 0x02;
export const ERR_UNKNOWN_CMD = 0xee;
const EPC_LEN = 12;
const TAG_PACKET_LEN = EPC_LEN + 1;
const ID_TYPE_EPC96 = 1;

export const TAG_TYPE_LABELS: Record<number, string> = {
  1: "EPC-96",
  2: "EPC-128",
  3: "UID",
  4: "UUID",
  5: "Custom",
};

export interface Tag {
  idType: number;
  epc: string;
  rssi: number;
}

export interface ScanResult {
  tags: Tag[];
}

export interface PairResult {
  tag: Tag;
}

export interface ErrorResult {
  code: number;
}

export type ParsedResponse =
  | { type: "scan"; result: ScanResult }
  | { type: "pair"; result: PairResult }
  | { type: "error"; result: ErrorResult };

export function epcToHex(bytes: Uint8Array): string {
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, "0").toUpperCase())
    .join("");
}

function parsePayload(data: Uint8Array, offset: number, count: number): Tag[] {
  const tags: Tag[] = [];
  let pos = offset;
  for (let i = 0; i < count; i++) {
    if (pos + TAG_PACKET_LEN > data.length) break;
    const epcBytes = data.slice(pos, pos + EPC_LEN);
    pos += EPC_LEN;
    const rssi = new Int8Array([data[pos++]])[0];
    tags.push({ idType: ID_TYPE_EPC96, epc: epcToHex(epcBytes), rssi });
  }
  return tags;
}

export function parseResponse(data: Uint8Array): ParsedResponse | null {
  if (data.length < 2) return null;
  const type = data[0];

  switch (type) {
    case RESP_SCAN_RESULT: {
      const count = data[1];
      if (data.length !== 2 + count * TAG_PACKET_LEN) return null;
      return { type: "scan", result: { tags: parsePayload(data, 2, count) } };
    }
    case RESP_PAIR_RESULT: {
      const count = data[1];
      if (data.length !== 2 + count * TAG_PACKET_LEN) return null;
      const tags = parsePayload(data, 2, count);
      if (tags.length === 0) return null;
      return { type: "pair", result: { tag: tags[0] } };
    }
    case RESP_ERROR:
      if (data.length !== 2) return null;
      return { type: "error", result: { code: data[1] } };
    default:
      return null;
  }
}

export function getExpectedResponseLength(data: Uint8Array): number | null {
  if (data.length < 2) return null;
  const type = data[0];
  if (type === RESP_ERROR) return 2;
  if (type !== RESP_SCAN_RESULT && type !== RESP_PAIR_RESULT) return null;
  return 2 + data[1] * TAG_PACKET_LEN;
}

export function concatBytes(a: Uint8Array, b: Uint8Array): Uint8Array {
  const out = new Uint8Array(a.length + b.length);
  out.set(a, 0);
  out.set(b, a.length);
  return out;
}
