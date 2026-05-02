"use client";

import { useState, useCallback, useRef } from "react";
import {
  CMD_SCAN,
  CMD_PAIR,
  parseResponse,
  getExpectedResponseLength,
  concatBytes,
  type Tag,
} from "@/lib/protocol";

const BLE_SERVICE_UUID = "7c6e4014-b6b1-4f7d-9a3d-8f1e2d4c1000";
const BLE_CHAR_CTRL_UUID = "7c6e4014-b6b1-4f7d-9a3d-8f1e2d4c1001";
const BLE_CHAR_DATA_UUID = "7c6e4014-b6b1-4f7d-9a3d-8f1e2d4c1002";

type CommandCallback = (tags: Tag[], error?: string) => void;

export function useBLE() {
  const [isConnected, setIsConnected] = useState(false);
  const [isConnecting, setIsConnecting] = useState(false);

  const deviceRef = useRef<any | null>(null);
  const ctrlRef = useRef<any | null>(null);
  const bufferRef = useRef<Uint8Array>(new Uint8Array(0));
  const callbackRef = useRef<CommandCallback | null>(null);
  const commandIdRef = useRef(0);
  const timeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const handleData = useCallback((event: Event) => {
    const target = event.target as any;
    const value = target.value;
    if (!value) return;

    const bytes = new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
    bufferRef.current = concatBytes(bufferRef.current, bytes);

    // Try to parse complete frames
    while (bufferRef.current.length > 0) {
      const expectedLen = getExpectedResponseLength(bufferRef.current);
      if (expectedLen === null) break;

      const frame = bufferRef.current.slice(0, expectedLen);
      bufferRef.current = bufferRef.current.slice(expectedLen);

      const parsed = parseResponse(frame);
      if (!parsed) continue;

      if (parsed.type === "error") {
        callbackRef.current?.([], "No tag detected");
        callbackRef.current = null;
        continue;
      }

      if (parsed.type === "pair") {
        callbackRef.current?.([parsed.result.tag]);
        callbackRef.current = null;
        continue;
      }

      if (parsed.type === "scan") {
        callbackRef.current?.(parsed.result.tags);
        callbackRef.current = null;
      }
    }
  }, []);

  const connect = useCallback(async () => {
    if (!(navigator as any).bluetooth) {
      throw new Error("Web Bluetooth not supported");
    }

    setIsConnecting(true);
    try {
      const device = await (navigator as any).bluetooth.requestDevice({
        filters: [{ services: [BLE_SERVICE_UUID] }],
      });

      device.addEventListener("gattserverdisconnected", () => {
        setIsConnected(false);
        deviceRef.current = null;
        ctrlRef.current = null;
      });

      const server = await device.gatt!.connect();
      const service = await server.getPrimaryService(BLE_SERVICE_UUID);
      const ctrl = await service.getCharacteristic(BLE_CHAR_CTRL_UUID);
      const data = await service.getCharacteristic(BLE_CHAR_DATA_UUID);

      await data.startNotifications();
      data.addEventListener("characteristicvaluechanged", handleData);

      deviceRef.current = device;
      ctrlRef.current = ctrl;
      setIsConnected(true);
    } finally {
      setIsConnecting(false);
    }
  }, [handleData]);

  const disconnect = useCallback(() => {
    if (deviceRef.current?.gatt?.connected) {
      deviceRef.current.gatt.disconnect();
    }
    setIsConnected(false);
    deviceRef.current = null;
    ctrlRef.current = null;
  }, []);

  const sendCommand = useCallback(
    (cmd: number): Promise<{ tags: Tag[]; error?: string }> => {
      return new Promise((resolve) => {
        if (!ctrlRef.current) {
          resolve({ tags: [], error: "Not connected" });
          return;
        }

        const commandId = ++commandIdRef.current;
        if (timeoutRef.current) {
          clearTimeout(timeoutRef.current);
          timeoutRef.current = null;
        }

        // Reset state
        bufferRef.current = new Uint8Array(0);

        callbackRef.current = (tags, error) => {
          if (timeoutRef.current) {
            clearTimeout(timeoutRef.current);
            timeoutRef.current = null;
          }
          resolve({ tags, error });
        };

        ctrlRef.current
          .writeValueWithResponse(new Uint8Array([cmd]))
          .catch((err: { message: any }) => {
            if (commandIdRef.current === commandId) {
              callbackRef.current = null;
              if (timeoutRef.current) {
                clearTimeout(timeoutRef.current);
                timeoutRef.current = null;
              }
            }
            resolve({ tags: [], error: err.message });
          });

        // Timeout after 10 seconds
        timeoutRef.current = setTimeout(() => {
          if (commandIdRef.current === commandId && callbackRef.current) {
            callbackRef.current = null;
            timeoutRef.current = null;
            resolve({ tags: [], error: "Timeout" });
          }
        }, 10000);
      });
    },
    [],
  );

  const scanPair = useCallback(() => sendCommand(CMD_PAIR), [sendCommand]);
  const scanAll = useCallback(() => sendCommand(CMD_SCAN), [sendCommand]);

  return {
    isConnected,
    isConnecting,
    connect,
    disconnect,
    scanPair,
    scanAll,
  };
}
