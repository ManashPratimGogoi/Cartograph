import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// During development the React app runs on :5173 and the Crow API on :18080.
// Proxying /api avoids cross-origin friction; the server also sends CORS
// headers so a deployed build can call it directly.
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      "/api": {
        target: "http://localhost:18080",
        changeOrigin: true,
      },
    },
  },
});
