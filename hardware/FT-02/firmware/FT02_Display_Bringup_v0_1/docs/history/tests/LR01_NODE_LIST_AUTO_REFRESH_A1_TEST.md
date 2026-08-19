# LR01 Node List Auto Refresh A1

## Purpose
Fix first-entry node page behavior. `MESH_NODES?` is asynchronous; the screen used to draw before `MESH_NODE_END`, so the completed list was only visible after leaving and entering again.

## Expected sequence
1. Enter Internal Communication -> Network Nodes.
2. Core sends `MESH_NODES?` and shows the loading/request notice.
3. LR01 returns zero or more `MESH_NODE ...` lines.
4. LR01 sends `MESH_NODE_END count=N`.
5. Core logs:
   `[LR01] NODE_LIST complete count=N`
   `[LR01-NODE-UI-A1] node list commit redraw count=N`
6. The node list appears automatically on the same page. No exit/re-entry is required.

## Refresh behavior
Only `MESH_NODE_END` triggers the automatic full page redraw. Individual `MESH_NODE` records do not trigger separate EPD refreshes.
