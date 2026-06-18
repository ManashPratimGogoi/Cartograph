import { useEffect, useMemo, useRef, useState } from "react";
import {
  decodeBytes,
  pixelsToImageData,
  type GeneratedWorld,
} from "../api";

interface Reading {
  xNorm: number;
  yNorm: number;
  biome: string;
  height: number;
  temperature: number;
  moisture: number;
}

interface Props {
  world: GeneratedWorld | null;
  loading: boolean;
  error: string | null;
}

// Decoded reading-grid channels, kept alongside the world they came from.
function useReadingGrid(world: GeneratedWorld | null) {
  return useMemo(() => {
    if (!world) return null;
    return {
      step: world.grid.step,
      width: world.grid.width,
      height: world.grid.height,
      heightData: decodeBytes(world.grid.heightData),
      tempData: decodeBytes(world.grid.tempData),
      moistData: decodeBytes(world.grid.moistData),
      biomeData: decodeBytes(world.grid.biomeData),
      biomeNames: world.grid.biomeNames,
    };
  }, [world]);
}

export default function SurveyPlate({ world, loading, error }: Props) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const frameRef = useRef<HTMLDivElement | null>(null);
  const grid = useReadingGrid(world);
  const [reading, setReading] = useState<Reading | null>(null);
  const [cursor, setCursor] = useState<{ x: number; y: number } | null>(null);
  const [sweep, setSweep] = useState(false);

  // Draw the world image to the canvas at device resolution.
  useEffect(() => {
    const canvas = canvasRef.current;
    const frame = frameRef.current;
    if (!canvas || !frame || !world) return;

    const image = pixelsToImageData(world.pixels, world.width, world.height);
    const source = document.createElement("canvas");
    source.width = world.width;
    source.height = world.height;
    source.getContext("2d")!.putImageData(image, 0, 0);

    const render = () => {
      const rect = frame.getBoundingClientRect();
      const dpr = Math.min(window.devicePixelRatio || 1, 2);
      canvas.width = Math.round(rect.width * dpr);
      canvas.height = Math.round(rect.height * dpr);
      const ctx = canvas.getContext("2d")!;
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = "high";
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      ctx.drawImage(source, 0, 0, canvas.width, canvas.height);
    };

    render();
    const observer = new ResizeObserver(render);
    observer.observe(frame);
    return () => observer.disconnect();
  }, [world]);

  // Trigger the surveying sweep when a fresh world arrives.
  useEffect(() => {
    if (!world) return;
    setSweep(false);
    const id = requestAnimationFrame(() => setSweep(true));
    return () => cancelAnimationFrame(id);
  }, [world]);

  const handleMove = (event: React.MouseEvent<HTMLDivElement>) => {
    const frame = frameRef.current;
    if (!frame || !grid) return;
    const rect = frame.getBoundingClientRect();
    const xNorm = (event.clientX - rect.left) / rect.width;
    const yNorm = (event.clientY - rect.top) / rect.height;
    if (xNorm < 0 || xNorm > 1 || yNorm < 0 || yNorm > 1) {
      setReading(null);
      setCursor(null);
      return;
    }

    const gx = Math.min(grid.width - 1, Math.floor(xNorm * grid.width));
    const gy = Math.min(grid.height - 1, Math.floor(yNorm * grid.height));
    const idx = gy * grid.width + gx;
    const biomeId = grid.biomeData[idx] ?? 0;

    setCursor({ x: event.clientX - rect.left, y: event.clientY - rect.top });
    setReading({
      xNorm,
      yNorm,
      biome: grid.biomeNames[biomeId] ?? "Unknown",
      height: grid.heightData[idx] / 255,
      temperature: grid.tempData[idx] / 255,
      moisture: grid.moistData[idx] / 255,
    });
  };

  const clearReading = () => {
    setReading(null);
    setCursor(null);
  };

  const rulerTicks = Array.from({ length: 9 }, (_, i) => i / 8);

  return (
    <section className="plate" aria-label="World survey plate">
      <div className="plate__rail plate__rail--top">
        {rulerTicks.map((t) => (
          <span key={`tx-${t}`} className="plate__tick" style={{ left: `${t * 100}%` }}>
            <i />
            <em>{Math.round(t * 100)}</em>
          </span>
        ))}
      </div>
      <div className="plate__rail plate__rail--left">
        {rulerTicks.map((t) => (
          <span key={`ty-${t}`} className="plate__tick" style={{ top: `${t * 100}%` }}>
            <i />
            <em>{Math.round(t * 100)}</em>
          </span>
        ))}
      </div>

      <div
        ref={frameRef}
        className={`plate__frame${sweep ? " plate__frame--swept" : ""}`}
        onMouseMove={handleMove}
        onMouseLeave={clearReading}
      >
        <canvas ref={canvasRef} className="plate__canvas" />

        <span className="plate__corner plate__corner--tl" />
        <span className="plate__corner plate__corner--tr" />
        <span className="plate__corner plate__corner--bl" />
        <span className="plate__corner plate__corner--br" />

        {world && <div className="plate__sweep" key={world.pixels.length + world.settings.seed} />}

        {world?.viewMode === "biomes" &&
          world.features.map((feature, i) => (
            <div
              key={`${feature.name}-${i}`}
              className="marker"
              style={{
                left: `${(feature.x / world.width) * 100}%`,
                top: `${(feature.y / world.height) * 100}%`,
              }}
            >
              <span className="marker__dot" data-theme={feature.theme} />
              <span className="marker__label">{feature.name}</span>
            </div>
          ))}

        {cursor && (
          <>
            <span className="crosshair crosshair--v" style={{ left: cursor.x }} />
            <span className="crosshair crosshair--h" style={{ top: cursor.y }} />
          </>
        )}

        <div className="compass" aria-hidden="true">
          <span className="compass__needle" />
          <em>N</em>
        </div>

        {loading && (
          <div className="plate__status">
            <span className="plate__spinner" />
            <p>Surveying terrain…</p>
          </div>
        )}

        {!loading && error && (
          <div className="plate__status plate__status--error">
            <p className="plate__status-title">No signal from the survey engine</p>
            <p>{error}</p>
            <p className="plate__status-hint">
              Start the C++ API (TerrainServer.exe), then generate again.
            </p>
          </div>
        )}

        {!loading && !error && !world && (
          <div className="plate__status">
            <p className="plate__status-title">Empty plate</p>
            <p>Set a seed and generate a world to begin the survey.</p>
          </div>
        )}
      </div>

      <div className={`sounding${reading ? " sounding--live" : ""}`}>
        {reading ? (
          <>
            <span className="sounding__coord">
              X {reading.xNorm.toFixed(3)} · Y {reading.yNorm.toFixed(3)}
            </span>
            <span className="sounding__biome">{reading.biome}</span>
            <span className="sounding__metric">
              <em>ELEV</em>
              {reading.height.toFixed(2)}
            </span>
            <span className="sounding__metric">
              <em>TEMP</em>
              {reading.temperature.toFixed(2)}
            </span>
            <span className="sounding__metric">
              <em>MOIST</em>
              {reading.moisture.toFixed(2)}
            </span>
          </>
        ) : (
          <span className="sounding__idle">Move across the plate to take a sounding</span>
        )}
      </div>
    </section>
  );
}
