# MeasureTool

An editor measuring tool for Unreal Engine. Drop a measure actor into your level, pick two
anchor points, and get a live, camera-facing distance readout drawn directly in the viewport —
complete with a ruler-style line, tick marks, and a units label.

![Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-blue) ![License](https://img.shields.io/badge/license-MIT-green)

## Features

- **Two-anchor measurement** — measure the distance between any two actors in the level.
- **Live editor preview** — the line and label update every frame in the viewport, including
  when you drag the anchors around. No PIE / runtime needed.
- **Camera-facing ruler line** — a billboarded procedural-mesh line keeps a constant apparent
  thickness from any viewing angle, with optional tick marks spaced at one display unit each.
- **Unit conversion** — display in centimeters, meters, kilometers, inches, feet, yards, or miles.
- **Configurable label** — toggle visibility, set decimal precision, slide its position along
  the line, and choose its color.
- **One-click anchor creation** — spawn anchor `TargetPoint`s straight from the details panel.
- **Pivot snapping** — re-center the actor's pivot on anchor A without moving the anchors in
  world space.

## Usage

1. In the Content Browser, navigate to **MeasureTool Content** and drag **`BP_MeasureActor`**
   into your level.
2. Assign the two endpoints you want to measure:
   - Set **Anchor A** and **Anchor B** in the details panel to any actors already in the level, **or**
   - Click **Create Anchor** in the details panel to spawn a `TargetPoint` at the measure
     actor's location. The first click fills Anchor A, the second fills Anchor B. New anchors
     are attached to the measure actor and named `<Actor>_AnchorA` / `<Actor>_AnchorB`.
3. Move the anchors. The line, tick marks, and label update live, and the read-only
   **Distance CM** field reflects the raw distance in centimeters.

> **Tip:** `BP_MeasureActor` is the intended entry point. The underlying C++ class
> `AMeasureActor` is marked `Abstract` and cannot be placed directly.

## Properties

All properties are exposed in the **Measure** category and are keyframable so you
can animate them in Sequencer.

### Measure

| Property        | Description                                                        |
| --------------- | ------------------------------------------------------------------ |
| `Anchor A`      | First endpoint actor.                                              |
| `Anchor B`      | Second endpoint actor.                                             |
| `Distance CM`   | Read-only measured distance in centimeters.                       |

### Measure &#124; Line

| Property      | Default        | Description                                          |
| ------------- | -------------- | ---------------------------------------------------- |
| `Line Width`  | `5.0`          | Apparent thickness of the line.                      |
| `Line Color`  | White          | Line color.                                          |
| `Show Ticks`  | `true`         | Draw tick marks at every display-unit interval.      |
| `Tick Width`  | `4.0`          | Thickness of each tick mark.                         |
| `Tick Length` | `25.0`         | Length of each tick mark.                            |

### Measure &#124; Label

| Property         | Default   | Description                                                  |
| ---------------- | --------- | ------------------------------------------------------------ |
| `Show Label`     | `true`    | Toggle the distance label.                                   |
| `Display Unit`   | Meters    | cm, m, km, in, ft, yd, or mi. Also drives tick spacing.      |
| `Label Decimals` | `2`       | Decimal places shown (0–6).                                  |
| `Label Position` | `0.5`     | Position of the label along the line (0 = A, 1 = B).         |
| `Label Color`    | White     | Label text color.                                            |

## License

[MIT](LICENSE) © 2026 Raphael Gaudin
