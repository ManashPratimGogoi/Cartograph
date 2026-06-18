// Typed client for the Crow terrain API. The C++ engine produces every world;
// these calls only carry settings to it and bring back rendered data.

export type ThemeId = "mixed" | "desert" | "forest" | "alpine";
export type ViewModeId = "biomes" | "height" | "temperature" | "moisture";

export interface TerrainSettings {
  seed: number;
  frequency: number;
  octaves: number;
  persistence: number;
  lacunarity: number;
  waterLevel: number;
  offsetX: number;
  offsetY: number;
  theme: ThemeId;
  viewMode: ViewModeId;
}

export interface BiomeShare {
  name: string;
  percent: number;
}

export interface WorldStats {
  waterPercent: number;
  landPercent: number;
  minHeight: number;
  maxHeight: number;
  meanTemperature: number;
  meanMoisture: number;
  settlementCount: number;
  riverCells: number;
  biomes: BiomeShare[];
}

export interface MapFeature {
  x: number;
  y: number;
  name: string;
  theme: ThemeId;
}

export interface ReadingGrid {
  step: number;
  width: number;
  height: number;
  heightData: string; // base64 bytes
  tempData: string;
  moistData: string;
  biomeData: string;
  biomeNames: string[];
}

export interface GeneratedWorld {
  width: number;
  height: number;
  viewMode: ViewModeId;
  pixels: string; // base64 RGB, width*height*3 bytes
  stats: WorldStats;
  grid: ReadingGrid;
  features: MapFeature[];
  settings: TerrainSettings;
}

export interface SavedWorldSummary {
  id: number;
  name: string;
  createdAt: string;
  seed: number;
  theme: ThemeId;
}

export interface SavedWorldDetail {
  id: number;
  name: string;
  createdAt: string;
  settings: TerrainSettings;
}

const API_BASE = "/api";

async function asJson<T>(response: Response): Promise<T> {
  if (!response.ok) {
    let message = `Request failed (${response.status})`;
    try {
      const body = await response.json();
      if (body?.error) message = body.error;
    } catch {
      // keep default message
    }
    throw new Error(message);
  }
  return response.json() as Promise<T>;
}

export async function generateWorld(
  settings: TerrainSettings
): Promise<GeneratedWorld> {
  const response = await fetch(`${API_BASE}/generate`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(settings),
  });
  return asJson<GeneratedWorld>(response);
}

export async function listWorlds(): Promise<SavedWorldSummary[]> {
  return asJson<SavedWorldSummary[]>(await fetch(`${API_BASE}/worlds`));
}

export async function saveWorld(
  name: string,
  settings: TerrainSettings
): Promise<{ id: number; name: string }> {
  const response = await fetch(`${API_BASE}/worlds`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ name, settings }),
  });
  return asJson<{ id: number; name: string }>(response);
}

export async function getWorld(id: number): Promise<SavedWorldDetail> {
  return asJson<SavedWorldDetail>(await fetch(`${API_BASE}/worlds/${id}`));
}

export async function deleteWorld(id: number): Promise<void> {
  const response = await fetch(`${API_BASE}/worlds/${id}`, {
    method: "DELETE",
  });
  await asJson<{ deleted: boolean }>(response);
}

// Decode the base64 RGB buffer into RGBA ImageData for a canvas.
export function pixelsToImageData(
  base64: string,
  width: number,
  height: number
): ImageData {
  const binary = atob(base64);
  const rgba = new Uint8ClampedArray(width * height * 4);
  for (let i = 0, p = 0; i < binary.length; i += 3, p += 4) {
    rgba[p] = binary.charCodeAt(i);
    rgba[p + 1] = binary.charCodeAt(i + 1);
    rgba[p + 2] = binary.charCodeAt(i + 2);
    rgba[p + 3] = 255;
  }
  return new ImageData(rgba, width, height);
}

// Decode a base64 byte string into a Uint8Array (for reading-grid channels).
export function decodeBytes(base64: string): Uint8Array {
  const binary = atob(base64);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes;
}

export const THEME_PRESETS: Record<
  ThemeId,
  Pick<
    TerrainSettings,
    "waterLevel" | "frequency" | "octaves" | "persistence"
  >
> = {
  mixed: { waterLevel: 0.36, frequency: 1.0, octaves: 5, persistence: 0.5 },
  desert: { waterLevel: 0.26, frequency: 0.92, octaves: 5, persistence: 0.46 },
  forest: { waterLevel: 0.4, frequency: 1.18, octaves: 6, persistence: 0.56 },
  alpine: { waterLevel: 0.31, frequency: 1.36, octaves: 7, persistence: 0.6 },
};

export const DEFAULT_SETTINGS: TerrainSettings = {
  seed: 1337,
  frequency: 1.0,
  octaves: 5,
  persistence: 0.5,
  lacunarity: 2.0,
  waterLevel: 0.36,
  offsetX: 0,
  offsetY: 0,
  theme: "mixed",
  viewMode: "biomes",
};
