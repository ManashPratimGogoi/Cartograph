import type { SavedWorldSummary } from "../api";

interface Props {
  worlds: SavedWorldSummary[];
  busy: boolean;
  onLoad: (id: number) => void;
  onDelete: (id: number) => void;
}

export default function WorldLibrary({ worlds, busy, onLoad, onDelete }: Props) {
  return (
    <section className="library">
      <header className="library__head">
        <span className="panel__eyebrow">Atlas</span>
        <h2 className="panel__title">Saved worlds</h2>
      </header>

      {worlds.length === 0 ? (
        <p className="library__empty">
          Nothing archived yet. Save a world and it lands here, reproducible from its seed.
        </p>
      ) : (
        <ul className="library__list">
          {worlds.map((world) => (
            <li key={world.id} className="archive">
              <button
                type="button"
                className="archive__open"
                onClick={() => onLoad(world.id)}
                disabled={busy}
              >
                <span className="archive__swatch" data-theme={world.theme} />
                <span className="archive__text">
                  <span className="archive__name">{world.name}</span>
                  <span className="archive__meta">
                    {world.theme} · seed {world.seed}
                  </span>
                </span>
              </button>
              <button
                type="button"
                className="archive__delete"
                aria-label={`Delete ${world.name}`}
                onClick={() => onDelete(world.id)}
              >
                ✕
              </button>
            </li>
          ))}
        </ul>
      )}
    </section>
  );
}
