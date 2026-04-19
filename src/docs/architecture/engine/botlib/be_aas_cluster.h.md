<!-- doxygenix: sha256=f7cb60e12a8d0961618d95846dc270a291c190c667a183372054f93088b5f322 -->

# engine/botlib/be_aas_cluster.h

## File Purpose
- Provide internal function prototypes for AAS clustering initialization within the bot navigation system.

## Core Responsibilities
- Declare internal functions for initializing AAS clustering and setting up cluster portals; enforce compile-time conditional inclusion via AASINTERN.

## Key Types and Functions
- AAS_InitClustering() — initializes clustering structures for AAS.
- AAS_SetViewPortalsAsClusterPortals() — configures view portals as cluster portals.

## Important Control Flow
- During engine startup, if AASINTERN is defined, AAS_InitClustering() is invoked to initialize clustering data structures; subsequently AAS_SetViewPortalsAsClusterPortals() configures portal-to-cluster relationships for visibility calculations.

## External Dependencies
- This header has no explicit includes; it relies on the surrounding AAS implementation (likely other botlib headers) for definitions of AAS data structures.

## How It Fits
- Part of the bot navigation subsystem (AAS) that handles spatial partitioning; these functions are called during navigation graph preparation to establish clusters and portal visibility.
