<!-- doxygenix: sha256=0be8bea3c45b21e7c5183625c97637f513873d08e95b7fb28b94c67936411111 -->

# engine/botlib/be_aas_debug.cpp

## File Purpose
- Implements visualization support for the AAS (Area A* Search) system, enabling developers to see polygons, lines, crosses, arrows, and area reachabilities during debugging sessions.

## Core Responsibilities
- Provide runtime visualization helpers for AAS debugging.
- Manage creation, reuse, and deletion of debug line and polygon handles.
- Draw geometric primitives (cross, arrow, plane cross, bounding boxes) to aid developers in understanding navigation data.
- Translate travel‑type constants into human‑readable messages.
- Compute and display predicted client movement for reachabilities.

## Key Types and Functions
- debuglines — array of active debug‑line identifiers.
- debuglinevisible — parallel flags marking whether a line handle is currently shown.
- debugpolygons — array of active debug‑polygon identifiers.
- AAS_ClearShownPolygons() — deletes all active polygons and clears the handle array.
- AAS_ShowPolygon(int color, int numpoints, vec3_t* points) — creates the first free polygon, sets its vertices, and displays it.
- AAS_ClearShownDebugLines() — deletes all active lines and marks them invisible.
- AAS_DebugLine(vec3_t start, vec3_t end, int color) — reuses or creates a line, makes it visible, and draws a segment.
- AAS_PermanentLine(vec3_t start, vec3_t end, int color) — creates a line that remains until explicitly deleted.
- AAS_DrawArrow(vec3_t start, vec3_t end, int linecolor, int arrowcolor) — renders an arrow by drawing shaft and head lines.
- AAS_DrawCross(vec3_t origin, float size, int color) — draws a 3‑D cross at the origin.
- AAS_DrawPlaneCross(vec3_t point, vec3_t normal, float dist, int type, int color) — draws a cross in an arbitrary plane.
- AAS_DrawPermanentCross(vec3_t origin, float size, int color) — permanent 3‑D cross using `AAS_DebugLine`.
- AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly) — visualizes face polygons belonging to an area.
- AAS_ShowArea(int areanum, int groundfacesonly) — draws edges of all faces in an area.
- AAS_ShowFace(int facenum) — visualizes the edges and normal of a single face.
- AAS_ShowFacePolygon(int facenum, int color, int flip) — collects face vertices and passes them to `AAS_ShowPolygon`.
- AAS_ShowBoundingBox(vec3_t origin, vec3_t mins, vec3_t maxs) — draws the edges of an axis‑aligned bounding box.
- AAS_PrintTravelType(int traveltype) — logs a readable name for a travel‑type constant (no‑op in non‑DEBUG builds).
- AAS_ShowReachability(aas_reachability_t* reach) — displays reachability polygons, arrows, and predicted movement.
- AAS_ShowReachableAreas(int areanum) — iterates over all reachable areas, throttles display updates, prints travel type/time, and calls `AAS_ShowReachability`.

## Important Control Flow
- Global arrays hold temporary line and polygon handles (`debuglines`, `debuglinevisible`, `debugpolygons`).
- `AAS_ClearShownPolygons` / `AAS_ClearShownDebugLines` iterate these arrays to delete existing handles via `botimport` API and reset the entries.
- `AAS_ShowPolygon`, `AAS_ShowFacePolygon`, `AAS_ShowAreaPolygons`, etc., locate a free index, create a handle with `botimport.DebugPolygonCreate`/`DebugLineCreate`, and immediately show it with `botimport.DebugPolygonShow`/`DebugLineShow`.
- `AAS_DrawArrow`, `AAS_DrawCross`, `AAS_DrawPlaneCross`, `AAS_DrawPermanentCross`, and `AAS_ShowBoundingBox` compute geometry, then use the debug line utilities to render static visual markers.
- `AAS_ShowReachability` uses travel type logic to compute velocities, calls `AAS_PredictClientMovement`, and renders arrows/crosses to illustrate the expected movement for each reachability.
- `AAS_ShowReachableAreas` maintains a static index over a area's reachable list, throttles updates to every 1.5 s via `AAS_Time()`, and drives `AAS_ShowReachability`.
- All functions indirectly rely on the external `aasworld` data structure and bot imports.

## External Dependencies
- "q_shared.h" – core engine types and definitions.
- "l_memory.h", "l_script.h", "l_precomp.h", "l_struct.h", "l_libvar.h" – low‑level engine utilities.
- "aasfile.h", "botshared/botlib.h", "botshared/be_aas.h" – AAS data structures and bot library interfaces.
- "be_interface.h", "be_aas_funcs.h", "be_aas_def.h" – bot engine specific interfaces.

## How It Fits
- Acts as a debugging extension within the bot library; it does not alter navigation logic, only provides visual markers.
- The functions are invoked from debugging or profiling tools, or from other parts of the bot engine when stepping through reachabilities.
- Keeps the core bot navigation code isolated from rendering concerns by interfacing with the `botimport` debug API.
