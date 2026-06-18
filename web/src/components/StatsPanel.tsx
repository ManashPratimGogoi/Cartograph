import type { GeneratedWorld } from "../api";

interface Props {
  world: GeneratedWorld | null;
}

export default function StatsPanel({ world }: Props) {
  if (!world) {
    return (
      <section className="readings">
        <header className="readings__head">
          <span className="panel__eyebrow">Readings</span>
          <h2 className="panel__title">Survey log</h2>
        </header>
        <p className="readings__empty">
          Generate a world to record its elevation, climate, and biome spread.
        </p>
      </section>
    );
  }

  const { stats } = world;

  return (
    <section className="readings">
      <header className="readings__head">
        <span className="panel__eyebrow">Readings</span>
        <h2 className="panel__title">Survey log</h2>
      </header>

      <div className="gauge">
        <div className="gauge__bar">
          <span className="gauge__land" style={{ width: `${stats.landPercent}%` }} />
          <span className="gauge__water" style={{ width: `${stats.waterPercent}%` }} />
        </div>
        <div className="gauge__legend">
          <span>
            <i className="dot dot--land" /> Land {stats.landPercent.toFixed(1)}%
          </span>
          <span>
            <i className="dot dot--water" /> Water {stats.waterPercent.toFixed(1)}%
          </span>
        </div>
      </div>

      <dl className="metric-grid">
        <div className="metric">
          <dt>Settlements</dt>
          <dd>{stats.settlementCount}</dd>
        </div>
        <div className="metric">
          <dt>Mean temp</dt>
          <dd>{stats.meanTemperature.toFixed(2)}</dd>
        </div>
        <div className="metric">
          <dt>Mean moist</dt>
          <dd>{stats.meanMoisture.toFixed(2)}</dd>
        </div>
        <div className="metric">
          <dt>Relief</dt>
          <dd>
            {stats.minHeight.toFixed(2)}–{stats.maxHeight.toFixed(2)}
          </dd>
        </div>
      </dl>

      <div className="biomes">
        <span className="biomes__title">Biome distribution</span>
        {stats.biomes.map((biome) => (
          <div key={biome.name} className="biome">
            <span className="biome__name">{biome.name}</span>
            <span className="biome__track">
              <span className="biome__fill" style={{ width: `${biome.percent}%` }} />
            </span>
            <span className="biome__pct">{biome.percent.toFixed(1)}%</span>
          </div>
        ))}
      </div>
    </section>
  );
}
