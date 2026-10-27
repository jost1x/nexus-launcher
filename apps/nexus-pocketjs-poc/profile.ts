import { defineProfile } from "@pocketjs/framework/profile";

export default defineProfile({
  id: "r36s",
  platform: "linux",
  form: "embedded",
  tickHz: 60,
  display: {
    physicalViewport: [640, 480],
    logicalViewports: [[640, 480]],
    rasterDensity: 1,
    presentations: ["native"],
  },
  capabilities: ["input.buttons", "text.glyphs.baked"],
  roleCapabilities: {},
});
