import { defineConfig } from 'vite'

export default defineConfig({
  base: '/navalbattle/',
  server: {
    proxy: {
      '/ws': {
        target: process.env.VITE_WS_URL || 'ws://localhost:8080',
        ws: true,
      }
    }
  }
})
