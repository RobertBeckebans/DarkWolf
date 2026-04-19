<!-- doxygenix: sha256=845f19b21f5c00f6b16acc1891d6565bb1ea15c5b6d157ba837800e366dc26a5 -->

# engine/botlib/aasfile.h

## File Purpose
- Defines the data structures and constants for the Area Aware System (AAS) file format.
- Specifies the spatial partitioning structure used for AI navigation and pathfinding.
- Provides the schema for the 'lump-based' binary format used to store navigation geometry in the engine.

## Core Responsibilities
- Representing the navigation mesh through convex areas (areas) and boundaries (faces/edges).
- Defining traversable movement types (e.g., walking, jumping, swimming, teleporting).
- Encoding physical properties of navigation volumes, such as liquid contents or ladder presence.
- Specifying the connectivity between different navigation clusters via portals.
- Establishing the connectivity logic (reachability) between adjacent navigation areas.

## Key Types and Functions
- AASID — Magic number used to identify valid AAS files.
- aas_bbox_t — An axis-aligned bounding box used for presence and collision queries within the AAS.
- aas_reachability_t — Defines a traversable link between two areas, including the required travel type and traversal time.
- aas_areasettings_t — Contains metadata for a specific area, including contents, flags, and connectivity indices.
- aas_portal_t — Describes the transition between two different clusters of areas.
- aas_cluster_t — A grouping of navigation areas used to manage large-scale connectivity.
- aas_plane_t — A 3D plane definition used as a primitive to bound convex areas.
- aas_face_t — A polygon defined by edges that separates two adjacent areas or serves as a boundary.
- aas_area_t — A convex unit of navigable space defined by vertices, faces, and bounding boxes.
- aas_node_t — A node in a tree structure (similar to a BSP) representing the spatial hierarchy of the navigation system.
- aas_header_t — The primary file header containing the version and an array of offsets (lumps) to the actual data segments.

## Important Control Flow
- The engine reads the `aas_header_t` to validate the file version and checksum.
- The system uses the `lumps` array in the header to resolve file offsets for vertices, planes, edges, faces, and areas.
- Pathfinding algorithms traverse the `aas_area_t` graph by evaluating `aas_reachability_t` connections.
- Movement logic checks `traveltype` flags against the bot's current capabilities to determine if a path is valid.

## External Dependencies
- vec3_t — Assumed to be defined in a primitive math header (e.g., `qvel.h` or similar).

## How It Fits
- This file provides the foundational data definitions for the `botlib` navigation system.
- It serves as the data contract between the map-processing tools (which generate AAS files) and the runtime engine (which parses them for AI pathfinding).
- It acts as the geometric backend for the Bot AI, allowing it to understand the traversable topology of the game world independently of the full physics/collision geometry.
