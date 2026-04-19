<!-- doxygenix: sha256=a48c38eaa0700923df1c655c09c01afeeb2c9e2d686f517b09aa219d5380512b -->

# engine/botlib/be_aas_sample.h

## File Purpose
- Define the public interface for bot navigation within the Return to Castle Wolfenstein engine; this header exposes functions that return positioning and collision information to enable AI pathfinding and perception.

## Core Responsibilities
- Declare runtime API for bot navigation and AAS queries; expose functions for area cluster/ presence type lookup, bbox area collection, face plane retrieval, bounding box area enumeration, entity linking, and collision tracing of client bounding boxes.
- Provide internal helper prototypes guarded by AASINTERN for memory heap/ linkage initialization and cleanup.

## Key Types and Functions
- AAS_InitAASLinkHeap() — initializes the heap used for linking AAS areas to entities.
- AAS_InitAASLinkedEntities() — prepares linked lists for entities within areas.
- AAS_FreeAASLinkHeap() — frees memory allocated for the link heap.
- AAS_FreeAASLinkedEntities() — clears the entity link data structures.
- AAS_AreaGroundFace(int areanum, vec3_t point) — returns the lowest ground face in a specified area containing a point.
- AAS_TraceEndFace(aas_trace_t* trace) — gives the face a trace ends on after traversal.
- AAS_PlaneFromNum(int planenum) — retrieves a plane structure by its index.
- AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum) — links an entity’s bounding box to intersecting AAS areas.
- AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype) — generates links for a client‑style bounding box within the AAS.
- AAS_PointInsideFace(int facenum, vec3_t point, float epsilon) — tests point inclusion within a face with tolerance.
- AAS_InsideFace(aas_face_t* face, vec3_t pnormal, vec3_t point, float epsilon) — checks if a point lies inside a face given its normal.
- AAS_UnlinkFromAreas(aas_link_t* areas) — removes links from all areas to a given linked list.
- AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs) — copies preset mins/maxs for NORMAL or CROUCH presence types.
- AAS_AreaCluster(int areanum) — returns the cluster number or portal identifier for a given area.
- AAS_AreaPresenceType(int areanum) — obtains the presence‑type bitmask allowed in an area.
- AAS_PointPresenceType(vec3_t point) — determines which presence types apply to a world point.
- AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype, int passent) — traces a client‑style bounding box through the AAS and reports collision data.
- AAS_TraceAreas(vec3_t start, vec3_t end, int* areas, vec3_t* points, int maxareas) — collects all areas intersected by a line segment and optional entry points.
- AAS_PointAreaNum(vec3_t point) — returns the area number containing a point or zero if solid/unloaded.
- AAS_FacePlane(int facenum, vec3_t normal, float* dist) — retrieves the plane normal and distance for a face.
- AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int* areas, int maxareas) — enumerates all areas overlapping a bounding box and outputs their numbers.

## Important Control Flow
- AAS world data is queried through a binary space partition tree or linked lists; stack traversal or pointers walk nodes to determine areas, faces, and planes; bounding box linking occurs via AAS_AASLinkEntity, after which results are collected into caller provided arrays; trace functions use iterative stacks to walk the spatial hierarchy and accumulate collision information at runtime.

## External Dependencies
- None explicitly listed – the header assumes inclusion of core AAS types (aas_face_t, aas_link_t, aas_trace_t, vec3_t) and global world data structures defined elsewhere in the botlib.

## How It Fits
- Forms the contract between the bot runtime and the AAS subsystem; other bot modules call these functions to obtain area identifiers, presence data, and perform spatial queries; it is central to navigation, collision avoidance, and area-based state management.
