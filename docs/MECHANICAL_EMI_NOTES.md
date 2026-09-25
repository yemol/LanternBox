# Mechanical / EMI Constraints - Not Frozen

- Target four layers: L1 components/high-current, L2 continuous GND, L3 power, L4 low-current. Stack thickness/copper and fabrication rules await supplier confirmation.
- Area goal <=45x30mm; 40x28mm is aspirational. Height goal <=6mm excluding necessary connector protrusion. No PCB outline is released.
- USB-C must lie at the PCB edge. X/Y/Z, shell tabs, case opening, plug overmold clearance, insertion direction and strain relief require enclosure CAD and exact connector drawing.
- Do not copy the old charger board's USB position. Battery/MainBoard connector types, pitch, locking, current per contact and cable bend radius are unknown.
- No large screw terminal, tall electrolytic or vertical header. Low-profile parts remain subject to current/thermal suitability.
- BMM350 must not sit against the PowerBoard. Reserve separation from all converter/charging inductors, high-current paths, LoRa antenna and speaker magnet. Determine distance/orientation by magnetic/navigation tests in the final assembly.
- Route high-current outgoing/return paths compactly. No feedback, Kelvin or GNSS signals under SW nodes/inductors.
- Follow manufacturer reference power layouts. Kelvin picks must not share high-current copper after the sensing point. Exposed pads/vias follow package instructions.
- EXT marking: `EXT_5V ONLY` and `NO RAW SOLAR INPUT`. Check legibility on the real board before mechanical release.
- DSBGA soldermask/stencil, inspection/rework and yield capability must be accepted before BQ27220 freeze; no alternative package presumed.

Blocking inputs: enclosure CAD, holes, connector drawings, battery dimensions/lead exit/polarity/wire gauge, mating geometry, environmental/condensation requirements, antenna and magnetometer positions. See OI-002/015/016. No raw6-24V solar connector variant is authorized.
