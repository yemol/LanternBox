# FT-01 Navigation UI Notes

Version: `v0.3.1-navigation-ui`

This version starts aligning navigation pages with the confirmed stable audio log dashboard style.

## Changed

- Session list uses 3 dashboard rows.
- Point list uses 3 dashboard rows.
- Header uses compact status:
  - title
  - SD
  - GNSS
  - HH:MM
- Footer uses compact action/status text.
- `H` opens help from list / overview / map / compass.

## Not changed

- Track file format.
- GNSS parsing.
- Base point logic.
- Map rendering math.
- Compass / bearing calculation.
- Audio log module.

## Controls

```text
< > 选择 | Enter 打开
R 刷新 | L 列表
M 地图 | N 指南
B 基地 | H 帮助
```
