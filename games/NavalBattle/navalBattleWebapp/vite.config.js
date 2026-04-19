import { defineConfig } from 'vite'

export default defineConfig(({ mode }) => ({
  base: `/${mode}/`,
  server: {
    proxy: {
      '/ws': {
        target: process.env.VITE_WS_URL || 'ws://localhost:8080',
        ws: true,
      }
    }
  }
}))
