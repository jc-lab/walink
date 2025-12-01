import { defineConfig } from "rolldown";

export default defineConfig({
  input: "src/walink.ts",
  output: [
    {
      file: "dist/walink.mjs",
      format: "esm",
      sourcemap: true,
    },
    {
      file: "dist/walink.cjs",
      format: "cjs",
      sourcemap: true,
    },
  ],
  treeshake: true,
  platform: "neutral",
});