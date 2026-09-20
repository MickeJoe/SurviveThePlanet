# Survive The Planet workspace instructions

- The canonical and only active project is `C:\UE5\SurviveThePlanet 5.8`.
- All source edits, Content changes, Unreal builds, generated project files, and diagnostics must target `C:\UE5\SurviveThePlanet 5.8\SurviveThePlanet.uproject`.
- Do not modify the legacy project at `C:\Users\qtxmj\Documents\Survive The Planet`.
- Before editing or building, verify that the working directory and target paths point to the canonical UE 5.8 project.
- Use Unreal Engine 5.8 from `C:\Program Files\Epic Games\UE_5.8` for builds.

# Survive the Planet

## Project

- Unreal Engine version: UE 5.8.
- Project root: `C:\UE5\SurviveThePlanet 5.8`.
- The game is primarily implemented using Unreal Engine C++ and Blueprints.
- Prefer existing project architecture and patterns over introducing new systems unnecessarily.

## MCP Tools

- Blender MCP is available on port 9876.
- Use Blender's own native MCP implementation. Do not use or assume a third-party Blender MCP server.
- Unreal Engine 5 MCP is available on port 8000.
- Use the MCP tools when appropriate for the task.

## Blender / Unreal Workflow

- Create or modify required 3D assets in Blender when appropriate.
- Use Blender's native MCP implementation for all Blender operations.
- After creating assets, continue implementation and integration in Unreal Engine 5.8.
- Use the UE5 MCP for Unreal Editor operations.
- Start Unreal Editor when needed if it is not already running.
- Do not ask the user to manually perform Blender or Unreal Editor steps that can reasonably be performed through the available MCP tools.

## UI

- Use Unreal Motion Graphics (UMG) Widget Blueprints (`WBP`) for all game UI components.
- Do not implement game UI using Slate unless explicitly requested.
- C++ may provide data, logic, events, and bindings used by the WBP UI.

## Code Style and Readability

- Follow Unreal Engine C++ coding standards and naming conventions.
- Match the existing coding style of the project when it differs only cosmetically from the UE standard.
- Write human-friendly code intended to be read and maintained by other developers.
- Prefer clear and explicit code over clever, overly compact, or unnecessarily abstract solutions.
- Use descriptive names for classes, functions, variables, and parameters.
- Keep functions focused on a single responsibility.
- Break large or complex functions into reasonably sized helper functions when this improves readability.
- Do not over-fragment simple logic into many tiny functions.
- A function should be understandable without requiring the reader to mentally decode a long sequence of unrelated operations.
- Extract meaningful steps into well-named functions when appropriate.
- Keep control flow easy to follow and avoid excessive nesting.
- Prefer early returns when they make the code easier to read.
- Keep related logic together.
- Make ownership, lifetime, and state changes clear from the code.
- Use `const` where appropriate.
- Use Unreal Engine types, containers, smart pointers, delegates, and object lifetime patterns appropriately.
- Respect Unreal reflection conventions for `UCLASS`, `USTRUCT`, `UENUM`, `UPROPERTY`, and `UFUNCTION`.
- Do not expose members to Blueprint unless there is a reason for them to be Blueprint-accessible.
- Use comments to explain intent, constraints, or non-obvious decisions.
- Do not add comments that merely restate obvious code.
- Avoid excessive comments and generated-looking documentation.
- Before implementing a complex solution, consider whether a simpler implementation would satisfy the same requirements.

## Reuse Existing Code

- Prefer reusing existing code over implementing the same or similar functionality again.
- Before implementing new functionality, search the relevant project code for existing functions, classes, components, subsystems, utilities, helpers, or patterns that already solve the same or a similar problem.
- Do not assume functionality is missing simply because it is not present in the currently open file.
- When similar functionality already exists, reuse or extend it rather than creating a parallel implementation.
- If two systems need the same logic, prefer extracting or using a shared implementation rather than maintaining duplicate logic in multiple places.
- Avoid copy-pasting an existing implementation into another class when the existing implementation can reasonably be reused.
- Prefer a single authoritative implementation for shared rules, calculations, validation, conversions, state transitions, and business/gameplay logic.
- Before adding a new helper function, check whether an equivalent or sufficiently similar helper already exists.
- Before adding a new class, component, subsystem, utility, enum, struct, or data type, search for an existing equivalent.
- When modifying functionality that appears in multiple places, determine whether the behavior should be centralized instead of applying the same change independently in several locations.
- If existing code is almost reusable, prefer making a small, clean generalization to that code rather than duplicating it.
- Do not create abstractions solely to eliminate trivial duplication. Reuse should make the code easier to understand and maintain.
- When there are multiple possible existing implementations to reuse, prefer the one that best matches the current architecture and ownership of the functionality.
- If intentionally choosing not to reuse an existing similar implementation, briefly explain why.

## C++ / Unreal

- Follow existing project architecture unless the requested task requires changing it.
- Inspect both `.h` and `.cpp` files before making structural changes to an existing class.
- Prefer straightforward Unreal/C++ solutions over custom infrastructure when Unreal already provides an appropriate pattern.
- Avoid unnecessary abstractions, wrappers, indirection, and generic frameworks.
- Do not modify generated files.

## Token Efficiency

- Optimize tool usage and repository exploration to minimize unnecessary context and token consumption.
- Prefer targeted `rg` searches over broad repository scans.
- Search project source before searching plugins or Unreal Engine source.
- Search only directories relevant to the current task.
- Do not recursively inspect generated, cached, or build directories unless specifically required:
  - `Binaries/`
  - `Intermediate/`
  - `DerivedDataCache/`
  - `Saved/`
  - `.vs/`
- Read only relevant portions of large files when possible.
- Do not dump entire large files into context when a targeted search or partial read is sufficient.
- Avoid repeatedly reading files that have already been inspected unless necessary.
- Avoid large directory listings when a targeted search can locate the required file.
- Keep build and test output concise; focus on errors, warnings, and information relevant to the task.
- Use RTK-compatible shell commands where appropriate so shell output can be compressed before entering model context.
- Prefer one well-targeted tool call over several exploratory calls when the required location is already known.
- Do not explore unrelated parts of the repository merely to gain additional context.

## Changes

- Keep changes scoped to the requested task.
- Do not modify unrelated files.
- Do not perform cleanup or refactoring unrelated to the requested change.
- Reuse existing systems and utilities instead of duplicating functionality.
- When modifying existing code, improve readability locally when useful, but do not perform unrelated refactoring.
- When finished, briefly summarize the files changed and the important implementation decisions.
- Mention existing functionality that was reused or generalized when relevant.

## Build / Verification

- Verify changes when practical.
- Prefer the smallest relevant build or verification step instead of rebuilding the entire project unnecessarily.
- When a build fails, focus first on the relevant compiler errors rather than processing the complete build log.
- Do not repeatedly rebuild without making a change or having a specific reason to expect a different result.