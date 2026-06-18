import { useCallback, useEffect, useRef, useState } from "react";
import {
  DEFAULT_SETTINGS,
  deleteWorld,
  generateWorld,
  getWorld,
  listWorlds,
  saveWorld,
  type GeneratedWorld,
  type SavedWorldSummary,
  type TerrainSettings,
} from "./api";
import ControlPanel from "./components/ControlPanel";
import SurveyPlate from "./components/SurveyPlate";
import StatsPanel from "./components/StatsPanel";
import WorldLibrary from "./components/WorldLibrary";

const VIEW_LABELS: Record<string, string> = {
  biomes: "Atlas",
  height: "Elevation",
  temperature: "Thermal",
  moisture: "Moisture",
};

export default function App() {
  const [settings, setSettings] = useState<TerrainSettings>(DEFAULT_SETTINGS);
  const [world, setWorld] = useState<GeneratedWorld | null>(null);
  const [worlds, setWorlds] = useState<SavedWorldSummary[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [notice, setNotice] = useState<string | null>(null);
  const requestId = useRef(0);

  const flash = useCallback((message: string) => {
    setNotice(message);
    window.setTimeout(() => setNotice(null), 2600);
  }, []);

  const run = useCallback(async (next: TerrainSettings) => {
    const id = ++requestId.current;
    setLoading(true);
    setError(null);
    try {
      const result = await generateWorld(next);
      if (id === requestId.current) setWorld(result);
    } catch (err) {
      if (id === requestId.current) {
        setError(err instanceof Error ? err.message : "Generation failed");
      }
    } finally {
      if (id === requestId.current) setLoading(false);
    }
  }, []);

  const refreshLibrary = useCallback(async () => {
    try {
      setWorlds(await listWorlds());
    } catch {
      // Library is non-critical; ignore when the API is unavailable.
    }
  }, []);

  // Initial survey + library load.
  useEffect(() => {
    run(DEFAULT_SETTINGS);
    refreshLibrary();
  }, [run, refreshLibrary]);

  // Regenerate automatically when only the view layer changes (cheap + expected).
  const patch = useCallback(
    (changes: Partial<TerrainSettings>) => {
      setSettings((prev) => {
        const next = { ...prev, ...changes };
        if (changes.viewMode && Object.keys(changes).length === 1) {
          run(next);
        }
        return next;
      });
    },
    [run]
  );

  const handleRandomize = useCallback(() => {
    const seed = Math.floor(Math.random() * 2_000_000_000);
    setSettings((prev) => {
      const next = { ...prev, seed };
      run(next);
      return next;
    });
  }, [run]);

  const handleSave = useCallback(async () => {
    const name = window.prompt("Name this world", `${settings.theme} · seed ${settings.seed}`);
    if (!name) return;
    try {
      await saveWorld(name, settings);
      flash(`Saved "${name}" to the atlas`);
      refreshLibrary();
    } catch (err) {
      flash(err instanceof Error ? err.message : "Could not save world");
    }
  }, [settings, flash, refreshLibrary]);

  const handleLoad = useCallback(
    async (id: number) => {
      try {
        const detail = await getWorld(id);
        setSettings(detail.settings);
        run(detail.settings);
        flash(`Loaded "${detail.name}"`);
      } catch (err) {
        flash(err instanceof Error ? err.message : "Could not load world");
      }
    },
    [run, flash]
  );

  const handleDelete = useCallback(
    async (id: number) => {
      try {
        await deleteWorld(id);
        refreshLibrary();
      } catch (err) {
        flash(err instanceof Error ? err.message : "Could not delete world");
      }
    },
    [refreshLibrary, flash]
  );

  return (
    <div className="app">
      <header className="masthead">
        <div className="masthead__brand">
          <span className="masthead__mark" aria-hidden="true" />
          <div>
            <h1 className="masthead__title">Cartograph</h1>
            <p className="masthead__tagline">Procedural world survey · noise-based terrain</p>
          </div>
        </div>
        <div className="masthead__status">
          <span className={`pill${loading ? " pill--busy" : error ? " pill--down" : " pill--live"}`}>
            {loading ? "Surveying" : error ? "Engine offline" : "Engine ready"}
          </span>
          <span className="masthead__layer">Layer · {VIEW_LABELS[settings.viewMode]}</span>
        </div>
      </header>

      <main className="workbench">
        <ControlPanel
          settings={settings}
          busy={loading}
          onChange={patch}
          onGenerate={() => run(settings)}
          onRandomize={handleRandomize}
          onSave={handleSave}
        />

        <div className="stage">
          <SurveyPlate world={world} loading={loading} error={error} />
        </div>

        <div className="dock">
          <StatsPanel world={world} />
          <WorldLibrary
            worlds={worlds}
            busy={loading}
            onLoad={handleLoad}
            onDelete={handleDelete}
          />
        </div>
      </main>

      {notice && <div className="toast">{notice}</div>}
    </div>
  );
}
