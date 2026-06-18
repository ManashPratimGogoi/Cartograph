import { THEME_PRESETS, type TerrainSettings, type ThemeId, type ViewModeId } from "../api";

interface Props {
  settings: TerrainSettings;
  busy: boolean;
  onChange: (patch: Partial<TerrainSettings>) => void;
  onGenerate: () => void;
  onRandomize: () => void;
  onSave: () => void;
}

const THEMES: { id: ThemeId; label: string; note: string }[] = [
  { id: "mixed", label: "Mixed", note: "Balanced climate" },
  { id: "desert", label: "Desert", note: "Dunes & mesas" },
  { id: "forest", label: "Forest", note: "Dense canopy" },
  { id: "alpine", label: "Alpine", note: "Snow & glaciers" },
];

const VIEWS: { id: ViewModeId; label: string }[] = [
  { id: "biomes", label: "Atlas" },
  { id: "height", label: "Elevation" },
  { id: "temperature", label: "Thermal" },
  { id: "moisture", label: "Moisture" },
];

interface SliderProps {
  label: string;
  value: number;
  min: number;
  max: number;
  step: number;
  format: (v: number) => string;
  onChange: (v: number) => void;
}

function Slider({ label, value, min, max, step, format, onChange }: SliderProps) {
  const pct = ((value - min) / (max - min)) * 100;
  return (
    <label className="field">
      <span className="field__head">
        <span className="field__label">{label}</span>
        <span className="field__value">{format(value)}</span>
      </span>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        style={{ ["--fill" as string]: `${pct}%` }}
        onChange={(e) => onChange(Number(e.target.value))}
      />
    </label>
  );
}

export default function ControlPanel({
  settings,
  busy,
  onChange,
  onGenerate,
  onRandomize,
  onSave,
}: Props) {
  const applyTheme = (theme: ThemeId) => {
    onChange({ theme, viewMode: "biomes", ...THEME_PRESETS[theme] });
  };

  return (
    <aside className="panel">
      <header className="panel__head">
        <span className="panel__eyebrow">Instrument</span>
        <h2 className="panel__title">Generation controls</h2>
      </header>

      <div className="panel__group">
        <span className="panel__group-label">Environment</span>
        <div className="theme-grid">
          {THEMES.map((theme) => (
            <button
              key={theme.id}
              type="button"
              className={`theme${settings.theme === theme.id ? " theme--active" : ""}`}
              data-theme={theme.id}
              onClick={() => applyTheme(theme.id)}
            >
              <span className="theme__name">{theme.label}</span>
              <span className="theme__note">{theme.note}</span>
            </button>
          ))}
        </div>
      </div>

      <div className="panel__group">
        <span className="panel__group-label">Seed</span>
        <div className="seed-row">
          <input
            className="seed-input"
            type="number"
            value={settings.seed}
            onChange={(e) => onChange({ seed: Number(e.target.value) || 0 })}
          />
          <button type="button" className="ghost-btn" onClick={onRandomize}>
            Randomize
          </button>
        </div>
      </div>

      <div className="panel__group">
        <span className="panel__group-label">Noise parameters</span>
        <Slider
          label="Frequency"
          value={settings.frequency}
          min={0.45}
          max={3.2}
          step={0.01}
          format={(v) => v.toFixed(2)}
          onChange={(v) => onChange({ frequency: v })}
        />
        <Slider
          label="Octaves"
          value={settings.octaves}
          min={1}
          max={8}
          step={1}
          format={(v) => String(v)}
          onChange={(v) => onChange({ octaves: v })}
        />
        <Slider
          label="Persistence"
          value={settings.persistence}
          min={0.25}
          max={0.8}
          step={0.01}
          format={(v) => v.toFixed(2)}
          onChange={(v) => onChange({ persistence: v })}
        />
        <Slider
          label="Water level"
          value={settings.waterLevel}
          min={0.2}
          max={0.55}
          step={0.01}
          format={(v) => v.toFixed(2)}
          onChange={(v) => onChange({ waterLevel: v })}
        />
      </div>

      <div className="panel__group">
        <span className="panel__group-label">View layer</span>
        <div className="view-row">
          {VIEWS.map((view) => (
            <button
              key={view.id}
              type="button"
              className={`view-chip${settings.viewMode === view.id ? " view-chip--active" : ""}`}
              onClick={() => onChange({ viewMode: view.id })}
            >
              {view.label}
            </button>
          ))}
        </div>
      </div>

      <div className="panel__actions">
        <button type="button" className="primary-btn" onClick={onGenerate} disabled={busy}>
          {busy ? "Surveying…" : "Generate world"}
        </button>
        <button type="button" className="ghost-btn ghost-btn--wide" onClick={onSave}>
          Save to atlas
        </button>
      </div>
    </aside>
  );
}
