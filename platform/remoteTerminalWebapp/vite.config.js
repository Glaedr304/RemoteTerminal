import { defineConfig } from 'vite'

export default defineConfig({
  base: '/',
  server: {
    proxy: {
      '/api': {
        target: process.env.VITE_API_URL || 'http://localhost:8080',
        changeOrigin: true,
      },
      '/ws': {
        target: (process.env.VITE_API_URL || 'http://localhost:8080').replace(/^http/, 'ws'),
        ws: true,
      }
    }
  }
})
