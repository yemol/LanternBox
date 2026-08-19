# RF Fragment Boundary A2.1 - True E512 Board Outline

Status: A2.1 COMPLETE / ENGINEERING REVIEW REQUIRED
Date: 2026-08-14
Source board: `reference/e512_imported/e512_imported.kicad_pcb`

No RF geometry was cut, translated, cropped, routed, or modified. This report only verifies the official E512 board outline centerlines from the imported KiCad `Edge.Cuts` geometry.

## Method

The apparent size in `reference/e512_imported/e512_stats.txt` came from KiCad board-edge bounding boxes. That bounding-box method includes stroke width. For true physical board size, this audit used the `Edge.Cuts` primitive centerlines from the imported board.

The imported board has four `Edge.Cuts` line primitives, each with width `0.0500 mm`.

| Edge | Centerline start X,Y mm | Centerline end X,Y mm | Stroke width |
|---|---:|---:|---:|
| Top | 104.0011, 76.0036 | 193.0011, 76.0036 | 0.0500 mm |
| Left | 104.0011, 134.0036 | 104.0011, 76.0036 | 0.0500 mm |
| Right | 193.0011, 76.0036 | 193.0011, 134.0036 | 0.0500 mm |
| Bottom | 193.0011, 134.0036 | 104.0011, 134.0036 | 0.0500 mm |

## True Physical Outline From Centerlines

| Item | Value |
|---|---:|
| True left edge centerline X | 104.0011 mm |
| True right edge centerline X | 193.0011 mm |
| True top edge centerline Y | 76.0036 mm |
| True bottom edge centerline Y | 134.0036 mm |
| True physical width | 89.0000 mm |
| True physical height | 58.0000 mm |

## Bounding-Box Artifact Check

KiCad `GetBoardEdgesBoundingBox()` / stats reported:

| Item | Value |
|---|---:|
| Apparent left X | 103.9761 mm |
| Apparent right X | 193.0261 mm |
| Apparent top Y | 75.9786 mm |
| Apparent bottom Y | 134.0286 mm |
| Apparent width | 89.0500 mm |
| Apparent height | 58.0500 mm |

The difference is exactly `0.0500 mm` in width and height, matching the `Edge.Cuts` stroke width. The apparent bounding box expands by `0.0250 mm` on each side of the true centerline outline.

Conclusion: the apparent `89.0500 x 58.0500 mm` report is a stroke/bounding-box artifact. The true imported official E512 board physical outline is `89.0000 x 58.0000 mm` by Edge.Cuts centerline.

## Impact On Provisional X=153.5000 mm Boundary

Using true centerline outline rather than stroke-inclusive bounding box:

| Item | Previous stroke-bbox value | True-centerline value |
|---|---:|---:|
| E512 right edge | 193.0261 mm | 193.0011 mm |
| E512 left edge | 103.9761 mm | 104.0011 mm |
| Cut offset from left edge | 49.5239 mm | 49.4989 mm |
| Candidate RF fragment width | 39.5261 mm | 39.5011 mm |
| Candidate RF fragment height | 58.0500 mm | 58.0000 mm |

The provisional cut coordinate `X=153.5000 mm` remains unchanged. Only the reported physical outline and fragment dimensions are corrected to centerline values.
