# Supercolony

Real-time strategy game about Argentine ant supercolony wars. The player steers the colony with pheromones and colony priorities, never by selecting units. Full design doc: https://claude.ai/code/artifact/cfe4018b-36eb-4862-9229-3109b07703f7

## Stack

- Unreal Engine 5.8, C++ project, Windows (Win64)
- Agents: Mass Entity (MassEntity, MassGameplay, MassAI). Never use one Actor per ant.
- Visuals: instanced static meshes near the camera, Niagara at distance
- UI: Blueprints + UMG
- Editor access: Epic's Unreal Engine plugin for Claude Code (unreal-mcp)

## Current milestone

**M0 Setup → M1 Swarm.** M1 target: 5,000 Mass agents, a pheromone grid, foraging between one nest and two food sources. Coloured dots only, no art.

M1 passes when: 60 fps with 5,000 agents, and trails form on their own without scripting.

Do not start M2 systems (combat, brood, economy) until M1 passes.

## Architecture rules

- Simulation logic lives in C++ Mass processors: movement, pheromone sampling and deposit, roles, combat.
- Pheromone grid is a plain C++ grid first (channels: food, home, alarm, rally, avoid). Each channel evaporates and diffuses per tick. Mirror to a texture for display. Move to GPU only if profiling says so.
- Keep tunable numbers (evaporation rate, cell size, ant speed, costs) in a data asset or config, not hard-coded.
- Ants of the same colony never fight each other.
- Prefer simple, observable systems: the player must be able to see why ants behave as they do.

## Code conventions

- Follow Epic's coding standard: F for structs, U for UObjects, A for Actors, E for enums, b prefix for bools.
- Source lives under `Source/Supercolony/`. Group by system: `Pheromones/`, `Agents/`, `Colony/`, `UI/`.
- Write automation tests for grid math and economy formulas.

## Building

Close the editor or disable Live Coding before building from the command line, or the build will fail.

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" SupercolonyEditor Win64 Development -Project="C:\Dev\Supercolony\Supercolony.uproject" -WaitMutex
```

Adjust paths if the engine or project live elsewhere.

## Safety rules

- Assume Willem has committed before the session; remind him if the working tree has uncommitted changes before large edits.
- Never delete or move assets in `Content/` without asking first.
- Never run with skip-permissions. Ask before any bulk editor operation.
- Keep changes small and explain what changed after each step.
