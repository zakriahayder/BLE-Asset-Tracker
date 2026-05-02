"use client";

import { useState, useEffect, useCallback } from "react";
import type { ReactNode } from "react";
import { useBLE } from "@/hooks/use-ble";
import {
  loadInventory,
  saveInventory,
  addItem,
  removeItem,
  updateLastSeen,
  type InventoryItem,
} from "@/lib/inventory";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Bluetooth, BluetoothOff, Plus, List, Scan, Trash2, RefreshCw, X } from "lucide-react";

export default function Home() {
  const { isConnected, isConnecting, connect, disconnect, scanPair, scanAll } = useBLE();
  const [isScanOpen, setIsScanOpen] = useState(false);
  const [inventory, setInventory] = useState<InventoryItem[]>([]);
  const [lastScanEpcs, setLastScanEpcs] = useState<string[]>([]);

  // Load inventory
  useEffect(() => {
    setInventory(loadInventory());
  }, []);

  // Save inventory whenever it changes
  useEffect(() => {
    if (inventory.length > 0 || loadInventory().length > 0) {
      saveInventory(inventory);
    }
  }, [inventory]);

  const handleDeleteItem = useCallback((epc: string) => {
    setInventory((prev) => removeItem(prev, epc));
  }, []);

  const handleRefresh = useCallback(async () => {
    const { tags, error } = await scanAll();
    if (!error) {
      const epcs = tags.map((t) => t.epc);
      setLastScanEpcs(epcs);
      setInventory((prev) => updateLastSeen(prev, epcs));
    }
  }, [scanAll]);

  const handleAddItem = useCallback(
    (epc: string, idType: number, displayName: string) => {
      setInventory((prev) => addItem(prev, epc, idType, displayName));
      void handleRefresh();
    },
    [handleRefresh]
  );

  return (
    <div className="min-h-screen bg-background">
      <Header
        isConnected={isConnected}
        onDisconnect={disconnect}
      />

      <main className="mx-auto w-full max-w-2xl px-6 py-8">
        {!isConnected ? (
          <DisconnectedView isConnecting={isConnecting} onConnect={connect} />
        ) : (
          <>
            <HomeView onScanNew={() => setIsScanOpen(true)} />
            <InventoryView
              items={inventory}
              lastScanEpcs={lastScanEpcs}
              onRefresh={handleRefresh}
              onDelete={handleDeleteItem}
            />
          </>
        )}
      </main>

      {isScanOpen && (
        <ScanModal onClose={() => setIsScanOpen(false)}>
          <ScanView
            scanPair={scanPair}
            onAdd={handleAddItem}
            onDone={() => setIsScanOpen(false)}
            existingEpcs={inventory.map((i) => i.epc)}
          />
        </ScanModal>
      )}
    </div>
  );
}

function Header({
  isConnected,
  onDisconnect,
}: {
  isConnected: boolean;
  onDisconnect: () => void;
}) {
  return (
    <header className="border-b border-border">
      <div className="px-6 h-16 flex items-center justify-between">
        <div className="flex items-center gap-2">
          <h1 className="font-semibold text-lg">Nanotrace Pro</h1>
        </div>
        <div className="flex items-center gap-2">
          {isConnected ? (
            <>
              <div className="flex items-center gap-1.5 text-sm text-muted-foreground">
                <span className="h-2 w-2 rounded-full bg-green-500" />
                Connected
              </div>
              <Button variant="ghost" size="sm" onClick={onDisconnect}>
                <BluetoothOff className="h-4 w-4" />
              </Button>
            </>
          ) : (
            <div className="flex items-center gap-1.5 text-sm text-muted-foreground">
              <span className="h-2 w-2 rounded-full bg-muted-foreground" />
              Disconnected
            </div>
          )}
        </div>
      </div>
    </header>
  );
}

function ScanModal({
  children,
  onClose,
}: {
  children: ReactNode;
  onClose: () => void;
}) {
  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 px-4">
      <div className="w-full max-w-md rounded-lg border border-border bg-background shadow-lg">
        <div className="flex h-12 items-center justify-between border-b border-border px-4">
          <h2 className="font-medium">Scan New Item</h2>
          <Button variant="ghost" size="icon-sm" onClick={onClose}>
            <X className="h-4 w-4" />
          </Button>
        </div>
        <div className="px-6">{children}</div>
      </div>
    </div>
  );
}

function DisconnectedView({
  isConnecting,
  onConnect,
}: {
  isConnecting: boolean;
  onConnect: () => void;
}) {
  return (
    <div className="mx-auto flex max-w-md flex-col items-center justify-center py-24 text-center">
      <Bluetooth className="h-16 w-16 text-muted-foreground mb-6" />
      <p className="text-lg text-muted-foreground mb-8">Connect your scanner to get started</p>
      <Button onClick={onConnect} disabled={isConnecting}>
        {isConnecting ? "Connecting..." : "Connect"}
      </Button>
    </div>
  );
}

function HomeView({
  onScanNew,
}: {
  onScanNew: () => void;
}) {
  return (
    <div className="pb-8 pt-4">
      <Button
        variant="outline"
        className="h-32 w-full flex-col gap-3 text-lg"
        onClick={onScanNew}
      >
        <Plus className="h-10 w-10" />
        <span>Scan New Item</span>
      </Button>
    </div>
  );
}

function ScanView({
  scanPair,
  onAdd,
  onDone,
  existingEpcs,
}: {
  scanPair: () => Promise<{ tags: { epc: string; idType: number }[]; error?: string }>;
  onAdd: (epc: string, idType: number, name: string) => void;
  onDone: () => void;
  existingEpcs: string[];
}) {
  const [isScanning, setIsScanning] = useState(false);
  const [scannedTag, setScannedTag] = useState<{ epc: string; idType: number } | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [itemName, setItemName] = useState("");

  const handleScan = async () => {
    if (isScanning) return;

    setIsScanning(true);
    setError(null);
    setScannedTag(null);

    const { tags, error: scanError } = await scanPair();
    setIsScanning(false);

    if (scanError) {
      setError("No tag detected. Make sure the tag is touching the scanner.");
      return;
    }

    if (tags.length > 0) {
      const tag = tags[0];
      if (existingEpcs.includes(tag.epc)) {
        setError("This tag is already in your inventory.");
        return;
      }
      setScannedTag(tag);
    }
  };

  const handleAdd = () => {
    if (!scannedTag || !itemName.trim()) return;
    onAdd(scannedTag.epc, scannedTag.idType, itemName.trim());
    setScannedTag(null);
    setItemName("");
    onDone();
  };

  if (scannedTag) {
    return (
      <div className="mx-auto max-w-md space-y-6 py-8">
        <div className="text-center">
          <div className="inline-flex items-center justify-center h-20 w-20 rounded-full bg-green-100 text-green-600 mb-4">
            <Scan className="h-10 w-10" />
          </div>
          <h2 className="text-lg font-medium">Tag Detected</h2>
          <p className="text-xs text-muted-foreground mt-1 font-mono">{scannedTag.epc}</p>
        </div>
        <div className="space-y-4">
          <Input
            placeholder="Name this item (e.g., Backpack)"
            value={itemName}
            onChange={(e) => setItemName(e.target.value)}
            autoFocus
          />
          <div className="flex gap-2">
            <Button variant="outline" className="flex-1" onClick={() => setScannedTag(null)}>
              Cancel
            </Button>
            <Button className="flex-1" onClick={handleAdd} disabled={!itemName.trim()}>
              Add to Inventory
            </Button>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="mx-auto flex max-w-md flex-col items-center justify-center py-24 text-center">
      <Scan className="h-16 w-16 text-muted-foreground mb-6" />
      <p className="text-lg text-muted-foreground mb-2">Hold a tag close to the scanner</p>
      {error && <p className="text-sm text-destructive mb-4">{error}</p>}
      <Button onClick={handleScan} disabled={isScanning}>
        {isScanning && <RefreshCw className="mr-2 h-4 w-4 animate-spin" />}
        {isScanning ? "Scanning..." : "Scan"}
      </Button>
    </div>
  );
}

function InventoryView({
  items,
  lastScanEpcs,
  onRefresh,
  onDelete,
}: {
  items: InventoryItem[];
  lastScanEpcs: string[];
  onRefresh: () => Promise<void>;
  onDelete: (epc: string) => void;
}) {
  const [isRefreshing, setIsRefreshing] = useState(false);

  const handleRefresh = async () => {
    setIsRefreshing(true);
    await onRefresh();
    setIsRefreshing(false);
  };

  if (items.length === 0) {
    return (
      <div className="mx-auto flex max-w-md flex-col items-center justify-center py-24 text-center">
        <List className="h-16 w-16 text-muted-foreground mb-6" />
        <p className="text-lg text-muted-foreground">No items in inventory</p>
        <p className="text-muted-foreground">Scan a tag to add your first item</p>
      </div>
    );
  }

  return (
    <div className="py-4">
      <div className="flex items-center justify-between mb-4">
        <h2 className="font-medium">My Inventory</h2>
        <Button variant="outline" size="sm" onClick={handleRefresh} disabled={isRefreshing}>
          <RefreshCw className={`h-4 w-4 mr-2 ${isRefreshing ? "animate-spin" : ""}`} />
          {isRefreshing ? "Scanning..." : "Refresh"}
        </Button>
      </div>
      <div className="space-y-2">
        {items.map((item) => {
          const isFound = lastScanEpcs.includes(item.epc);
          const hasBeenScanned = lastScanEpcs.length > 0;

          return (
            <div
              key={item.epc}
              className="flex items-center justify-between p-3 rounded-lg border border-border bg-card"
            >
              <div className="flex items-center gap-3">
                {hasBeenScanned && (
                  <span
                    className={`h-2.5 w-2.5 rounded-full ${
                      isFound ? "bg-green-500" : "bg-red-400"
                    }`}
                  />
                )}
                <div>
                  <p className="font-medium">{item.displayName}</p>
                  <p className="text-xs text-muted-foreground font-mono">{item.epc}</p>
                </div>
              </div>
              <Button
                variant="ghost"
                size="icon"
                className="text-muted-foreground hover:text-destructive"
                onClick={() => onDelete(item.epc)}
              >
                <Trash2 className="h-4 w-4" />
              </Button>
            </div>
          );
        })}
      </div>
      {lastScanEpcs.length > 0 && (
        <p className="text-xs text-muted-foreground text-center mt-4">
          {items.filter((i) => lastScanEpcs.includes(i.epc)).length} of {items.length} items found
        </p>
      )}
    </div>
  );
}
