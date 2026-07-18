/// <reference types="vitest/config" />
import { fileURLToPath } from 'node:url'
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      // Shared, game-agnostic building blocks (webLib/WebGUI_Lib) — the web
      // equivalent of pyLib/gmGui. Consumed as TS source, no build step yet
      // since this is currently its only consumer (see tsconfig.app.json for
      // the matching `paths` entry used by the TypeScript compiler/IDE).
      '@webgui': fileURLToPath(new URL('../../../../webLib/WebGUI_Lib/src', import.meta.url)),
      // WebGUI_Lib's source has no node_modules of its own above it on disk
      // (it lives outside webapp_frontend's tree), so plain 'react' imports
      // from it would fail to resolve — force them to this app's own copy.
      react: fileURLToPath(new URL('./node_modules/react', import.meta.url)),
      'react-dom': fileURLToPath(new URL('./node_modules/react-dom', import.meta.url)),
    },
  },
  server: {
    // Exposes the dev server through ngrok for remote tablet access.
    allowedHosts: ['elf-chaplain-spindle.ngrok-free.dev'],
    // Forwards REST + WebSocket calls to eng_serve (uvicorn default port) so the
    // browser never needs to know about the backend origin in dev (Phase 1: no auth).
    proxy: {
      '/sessions': {
        target: 'http://127.0.0.1:8000',
        ws: true,
      },
      '/health': 'http://127.0.0.1:8000',
    },
  },
  test: {
    environment: 'jsdom',
    setupFiles: ['./src/setupTests.ts'],
    globals: true,
  },
})
