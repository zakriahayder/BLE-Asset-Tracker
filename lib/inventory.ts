// Inventory persistence with localStorage

export interface InventoryItem {
  epc: string;
  idType: number;
  displayName: string;
  addedAt: string;
  lastSeenAt: string | null;
}

const STORAGE_KEY = "nanotrace-inventory";

export function loadInventory(): InventoryItem[] {
  if (typeof window === "undefined") return [];
  try {
    return JSON.parse(localStorage.getItem(STORAGE_KEY) || "[]");
  } catch {
    return [];
  }
}

export function saveInventory(items: InventoryItem[]): void {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(items));
}

export function addItem(
  items: InventoryItem[],
  epc: string,
  idType: number,
  displayName: string
): InventoryItem[] {
  // Check if already exists
  if (items.some((item) => item.epc === epc)) {
    return items;
  }
  const newItem: InventoryItem = {
    epc,
    idType,
    displayName,
    addedAt: new Date().toISOString(),
    lastSeenAt: null,
  };
  return [...items, newItem];
}

export function removeItem(items: InventoryItem[], epc: string): InventoryItem[] {
  return items.filter((item) => item.epc !== epc);
}

export function updateLastSeen(
  items: InventoryItem[],
  detectedEpcs: string[]
): InventoryItem[] {
  const now = new Date().toISOString();
  return items.map((item) => {
    if (detectedEpcs.includes(item.epc)) {
      return { ...item, lastSeenAt: now };
    }
    return item;
  });
}
