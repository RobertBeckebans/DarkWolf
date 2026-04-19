<!-- doxygenix: sha256=d4e18b19b1233467fac88d0a3d2b4b90a8ac0f5f7cc911259998468c32462667 -->

# engine/botlib/be_aas_sample.cpp

## File Purpose
- Implements the sample‑size AAS navigation logic used by the bot library.
- Provides traversal, querying, and collision routines on the AAS spatial partition.
- Manages dynamic allocation of link structures linking entities to areas.

## Core Responsibilities
- Compute presence‑type bounding boxes for normal and crouch agents.
- Allocate, link, and free a global heap of aas_link_t used to track entity‑area associations.
- Provide area queries: area number (AAS_PointAreaNum), cluster (AAS_AreaCluster), presence type (AAS_AreaPresenceType, AAS_PointPresenceType).
- Trace client‑sized bounding boxes through the AAS, detecting solid geometry, entity collision, and area transitions (AAS_TraceClientBBox).
- Record the sequence of areas intersected by a segment (AAS_TraceAreas).
- Test point containment in faces and bounding boxes (AAS_InsideFace, AAS_PointInsideFace).
- Retrieve face planes and ground faces (AAS_FacePlane, AAS_AreaGroundFace).
- Manage entity link lists: allocate links to intersecting areas, unlink on removal, and query linked areas (AAS_AASLinkEntity, AAS_LinkEntityClientBBox, AAS_UnlinkFromAreas).

## Key Types and Functions
- aas_node_t – tree node in the AAS spatial partition.
- aas_link_t – represents a link between an entity and an area.
- aas_trace_t – result of tracing a bounding box through the AAS.
- AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs) – fills mins/maxs for a given presence type.
- AAS_InitAASLinkHeap() – allocates and links the global link heap.
- AAS_FreeAASLinkHeap() – frees the link heap memory.
- aas_link_t *AAS_AllocAASLink() – retrieves a link from the free pool.
- void AAS_DeAllocAASLink(aas_link_t *link) – returns a link to the free pool.
- void AAS_InitAASLinkedEntities() – allocates per‑area entity link array.
- void AAS_FreeAASLinkedEntities() – releases that array.
- int AAS_PointAreaNum(vec3_t point) – returns the area containing a point.
- int AAS_AreaCluster(int areanum) – cluster ID for an area.
- int AAS_AreaPresenceType(int areanum) – allowed presence types in an area.
- int AAS_PointPresenceType(vec3_t point) – presence mask valid at a point.
- qboolean AAS_AreaEntityCollision(int areanum, vec3_t start, vec3_t end, int presencetype, int passent, aas_trace_t *trace) – tests a segment for entity collisions.
- aas_trace_t AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype, int passent) – full trace routine for a client bbox.
- int AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas) – collects intersected areas from a segment.
- qboolean AAS_InsideFace(aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon) – test point inside polygon.
- qboolean AAS_PointInsideFace(int facenum, vec3_t point, float epsilon) – same test using face index.
- aas_face_t *AAS_AreaGroundFace(int areanum, vec3_t point) – finds the ground face beneath a point.
- void AAS_FacePlane(int facenum, vec3_t normal, float *dist) – retrieves face plane.
- aas_face_t *AAS_TraceEndFace(aas_trace_t *trace) – finds face containing trace end.
- int AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, aas_plane_t *p) – tests a bbox against a plane.
- void AAS_UnlinkFromAreas(aas_link_t *areas) – removes an entity’s area links.
- aas_link_t *AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum) – links entity’s bbox to all intersecting areas.
- aas_link_t *AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype) – wrapper that expands bbox per presence type.
- aas_plane_t *AAS_PlaneFromNum(int planenum) – fetch a plane by index.
- int AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas) – fills array with intersected area numbers.

## Important Control Flow
- Functions first check if the AAS world is loaded, then use explicit stacks to traverse the AAS node tree.
- AAS_TraceClientBBox starts with the full segment on the trace stack, popping segments until the stack is empty or a hit occurs.
- For each node, the trace pushes child nodes depending on the signed distances to the node plane, and splits the segment when the line straddles the plane.
- On reaching an area node, the trace tests presence type compatibility and optionally checks entity collisions before recording the trace outcome.
- Leaf nodes that are solid simply set startsolid/fraction and return.

## External Dependencies
- "q_shared.h" – core math & vector utilities.
- "l_memory.h" – engine memory allocation helpers.
- "l_script.h", "l_precomp.h", "l_struct.h" – scripting and preload subsystem headers.
- "aasfile.h" – definitions of AAS world data structures.
- "botshared/botlib.h", "botshared/be_aas.h", "be_aas_funcs.h", "be_aas_def.h" – bot library interfaces and macros.

## How It Fits
- Forms the core of bot navigation calculations performed by higher‑level path‑finding code.
- Operates on the global AAS world loaded from disk, exposing functions for area lookup, collision detection, and entity‑area bookkeeping.
- Enables bots to sense terrain, avoid obstacles, and move through the level.
