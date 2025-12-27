# Raketic - Project Context

## Overview

Raketic is a 64kB demoscene game written in C featuring a 2D space environment with wireframe graphics and realistic gravitational physics simulation using a custom engine. This is approximately the twentieth iteration of a game concept being used as a learning project for game development.

## Core Concept

- 2D space game with wireframe graphics
- Realistic gravitational physics simulation
- Mouse-based spaceship controls
- Physics-accurate universe where momentum is preserved
- Dramatic gameplay possibilities like "shooting a moon out of orbit"
- 64kB size constraint (demoscene)

## Technical Constraints & Priorities

- **Size**: Must fit in 64kB (demoscene constraint)
- **Target**: Primarily Windows, potential Linux support
- **Priority**: Technical excellence over broad compatibility
- **Memory**: Static memory management only (no dynamic allocation)
- **Assets**: Compiled directly into executable

## Architecture Principles

- Clean separation of concerns in engine design
- Modular design with clean interfaces
- Abstracted implementation details
- Performance optimization through dense arrays and SIMD operations
- Branchless code execution where possible
- Canonical C directory structure with minimal cross-includes

## Technology Stack

- **Language**: C
- **Platform Layer**: SDL2 (not SDL3)
- **Graphics**: OpenGL 1.1
- **Package Manager**: vcpkg
- **Build System**: Visual Studio with vcxproj files (not CMake)
- **Debugging**: ETW (Event Tracing for Windows) in debug builds

## Engine Systems (Raketic Engine)

- Dense arrays for data storage
- Yoshida integrator for physics
- Simple Euler integrator for particles
- 120Hz fixed timestep
- Message bus (async) for communication
- OpenGL 1.1 for rendering

## Data Pipeline (Raketic Data)

- Lua DSL → C structs for data definition
- Orbit validation using Hill sphere calculations
- Precomputed tables at init (sin/cos/sound)
- 6-segment font for text rendering

## Combat System (Raketic Boj)

- Slot system for ship components
- Shield: circle around ship
- Armor/hull: per-component damage model
- Laser: passes through shields, applies heat damage

## Economy System (Raketic Ekonomika)

- Commodities with dynamic pricing
- Production chains
- NPCs influence market
- Planets can die/collapse
- Communication network affects information flow

## Other Systems (Raketic Ostatní)

- Tractor beam: collection, towing, caravans
- On-rails orbits with corrections
- Faction and reputation system
- WinMM sound synthesis

## Current Development Phase

Foundational phase with architectural decisions established:

1. Game loop with timing
2. Minimal wireframe renderer
3. Basic physics (position/velocity integration)
4. Input handling for controllable ship

## Planned Development Phases

1. **Next**: Gravitational physics, particle effects, multiple celestial bodies
2. **Later**: Entity management, collision detection
3. **Goal**: Working prototype before complex architecture

## Code Style Guidelines

- Clean interfaces with abstracted implementation
- Minimal codebase size
- Performance-critical code uses SIMD
- Branchless execution patterns
- Organized modular architecture around core engine systems
- Minimal cross-includes between modules

## Platform Layer Responsibilities

- SDL2/OpenGL initialization
- Event polling
- Timing management
- Separated from game logic
