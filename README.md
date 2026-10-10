# BASSFORGE v0.5 — Preset Lab

Build on GitHub using the included `.github/workflows/build-windows.yml`.

## Preset workflow
- **SAVE PRESET** writes `.bfpreset` JSON containing normalised parameters and FX order.
- **LOAD PRESET** reads a `.bfpreset` file.
- **LOAD PACK** opens a ZIP of `.bfpreset` files and lets you choose one to load.
- The pack ZIP in `Preset-Lab/Punisher-Pack-01.zip` is an example with 10 editable patches.
- Edit JSON values in `parameters` between 0.0 and 1.0 to design new sounds together. `shape` is normalised (0 saw, 0.5 horn, 1 metal).
- Packs are **not installed permanently**; choose a preset from the pack when you want to load it, then save your FL Studio project or export it individually.
- Factory presets remain untouched.

## Known limitations
This is uncompiled, untested source. The 10 presets are algorithmic variations of FM Punisher, not auditioned sounds. The preset loader validates format and range but it does not support older or newer schemas. Import changes current sound; use FL Studio project save to retain it. The UI keyboard from earlier versions is unchanged.
