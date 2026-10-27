import { createSignal } from "solid-js";
import { mount } from "@pocketjs/framework/solid";
import { Text, View } from "@pocketjs/framework/solid/components";
import { onFrame } from "@pocketjs/framework/solid/lifecycle";
import { BTN } from "@pocketjs/framework/input";

// Extension bits used by the local Linux evdev bridge; the latest EV_KEY code
// is packed separately in bits 20..29.
const FN = 1 << 16;
const PLUS = 1 << 17;
const MINUS = 1 << 18;
const UNKNOWN = 1 << 19;

const controls = [
  { name: "UP", mask: BTN.UP, group: "D-PAD" },
  { name: "DOWN", mask: BTN.DOWN, group: "D-PAD" },
  { name: "LEFT", mask: BTN.LEFT, group: "D-PAD" },
  { name: "RIGHT", mask: BTN.RIGHT, group: "D-PAD" },
  { name: "X", mask: BTN.SQUARE, group: "FACE" },
  { name: "Y", mask: BTN.TRIANGLE, group: "FACE" },
  { name: "A", mask: BTN.CIRCLE, group: "FACE" },
  { name: "B", mask: BTN.CROSS, group: "FACE" },
  { name: "L1", mask: BTN.LTRIGGER, group: "SHOULDERS" },
  { name: "R1", mask: BTN.RTRIGGER, group: "SHOULDERS" },
  { name: "L2", mask: BTN.ZL, group: "SHOULDERS" },
  { name: "R2", mask: BTN.ZR, group: "SHOULDERS" },
  { name: "SELECT", mask: BTN.SELECT, group: "SYSTEM" },
  { name: "START", mask: BTN.START, group: "SYSTEM" },
  { name: "FN", mask: FN, group: "SYSTEM" },
  { name: "PLUS", mask: PLUS, group: "SYSTEM" },
  { name: "MINUS", mask: MINUS, group: "SYSTEM" },
  { name: "DESCONOCIDO", mask: UNKNOWN, group: "SYSTEM" },
];

function GamepadTest() {
  const [held, setHeld] = createSignal(0);
  const [lastPressed, setLastPressed] = createSignal("—");
  const [pressCount, setPressCount] = createSignal(0);
  let previous = 0;

  onFrame((buttons) => {
    const pressed = buttons & ~previous;
    const keyCode = (buttons >>> 20) & 0x3ff;
    const previousKeyCode = (previous >>> 20) & 0x3ff;
    if (buttons !== previous) setHeld(buttons);
    if (pressed !== 0 || keyCode !== previousKeyCode) {
      const names = controls
        .filter((control) => (buttons & control.mask) !== 0)
        .map((control) => control.name);
      const keyId = keyCode > 0
        ? `KEY ${keyCode} (0x${keyCode.toString(16).padStart(3, "0")})`
        : "KEY —";
      setLastPressed(`${names.join(" + ") || "DESCONOCIDO"} · ${keyId}`);
      if (pressed !== 0) setPressCount((count) => count + Math.max(names.length, 1));
    }
    previous = buttons;
  });

  const groups = ["D-PAD", "FACE", "SHOULDERS", "SYSTEM"];

  return (
    <View class="flex-col w-full h-full px-7 py-5 gap-3 bg-slate-950">
      <View class="flex-row items-center justify-between">
        <View class="flex-col gap-1">
          <Text class="text-sm text-cyan-400 font-bold">NEXUS · PRUEBA DE MANDO R36S</Text>
          <Text class="text-2xl text-white font-bold">Gamepad tester</Text>
        </View>
        <Text class="text-sm text-slate-400">640 × 480</Text>
      </View>

      <View class="flex-row gap-3">
        {groups.map((group) => (
          <View class="flex-col flex-1 gap-2 p-3 rounded-lg bg-slate-800 border-slate-700">
            <Text class="text-xs text-slate-400 font-bold">{group}</Text>
            <View class="flex-row flex-wrap gap-2">
              {controls.filter((control) => control.group === group).map((control) => (
                <View
                  class={held() & control.mask
                    ? "px-3 py-2 rounded-md border bg-emerald-600 border-emerald-400"
                    : "px-3 py-2 rounded-md border bg-slate-900 border-slate-700"}
                >
                  <Text class={held() & control.mask ? "text-sm font-bold text-white" : "text-sm font-bold text-slate-500"}>
                    {control.name}{held() & control.mask ? " · DOWN" : ""}
                  </Text>
                </View>
              ))}
            </View>
          </View>
        ))}
      </View>

      <View class="flex-row items-center justify-between px-4 py-3 rounded-lg bg-slate-800 border-slate-700">
        <View class="flex-col gap-1">
          <Text class="text-xs text-slate-400">ÚLTIMO BOTÓN</Text>
          <Text class="text-base text-cyan-300 font-bold">{lastPressed()}</Text>
        </View>
        <View class="flex-col gap-1">
          <Text class="text-xs text-slate-400">PULSACIONES</Text>
          <Text class="text-base text-white font-bold">{pressCount()}</Text>
        </View>
        <Text class="text-xs text-slate-500">Mantén y suelta cada control</Text>
      </View>
      <Text class="text-xs text-slate-500">ÚLTIMO BOTÓN incluye su código Linux KEY (decimal y hexadecimal) · sticks analógicos no incluidos</Text>
    </View>
  );
}

mount(() => <GamepadTest />);
