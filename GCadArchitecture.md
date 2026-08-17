# GForce CAD Application Architecture

## Overview
This project is a desktop CAD application built with Qt 6 and C++. It follows a simple model-view-controller style structure centered around:
- a main window UI,
- a document model,
- a tool controller,
- and a rendering viewport.

The current application is designed for interactive 2D drawing operations such as lines, circles, ellipses, rectangles, polygons, offsets, trim, extend, fillet, and chamfer. Its geometry layer also defines the interfaces and lightweight helpers needed to grow toward 3D CAD, mesh processing, and engineering file interoperability.

## Advanced CAD capability extension

The architecture also includes a forward-looking engineering CAD layer for the following concerns:

- Boolean operations for solid and surface logic
- Solid modeling support with volume-aware entity workflows
- Mesh generation using triangle primitives and tessellation-ready structures
- Rendering support for richer visualization and shading pipelines
- Coordinate transformations for 2D and 3D modeling operations
- Collision and intersection tests for geometric validation
- Large drawing and assembly support through scalable document and layer design
- File format compatibility planning for DWG, DXF, STEP, and IGES

The advanced capability layer is intentionally staged. The current code provides data structures and helper functions for transforms, meshes, and geometric intersection checks. Full B-Rep solids, robust Boolean operations, production tessellation, and native DWG/DXF/STEP/IGES translation remain integration work for a dedicated geometry kernel and format libraries.

## Project Structure

- `src/main.cpp`  
  Entry point for the app. Creates and shows the main window.

- `src/app/MainWindow.h` / `MainWindow.cpp`  
  Main Qt window and application shell. Sets up toolbars, dock panels, menus, status bar, and command handling.

- `src/render/CadViewport.h` / `CadViewport.cpp`  
  Rendering surface for the CAD canvas. Handles drawing, preview generation, zoom/pan behavior, snapping, and user interaction.

- `src/cad/Document.h` / `Document.cpp`  
  Core document model. Tracks entities, layers, active layer, file state, and drawing operations.

- `src/cad/Entity.h` / `Entity.cpp`  
  Base entity system and factory logic. Each shape is built as a CAD entity with a shared interface for drawing, JSON export, hit testing, and movement.

- `src/cad/Entities.h` / `Entities.cpp`  
  Concrete entity implementations such as line, circle, ellipse, arc, rectangle, polyline, polygon, and triangle entities.

- `src/cad/Geometry.h`  
  Shared 2D/3D geometry types and helpers. It contains shape calculations, 2D and 3D coordinate transforms, mesh triangle structures, polygon area calculations, circle/segment/triangle intersection helpers, and the Boolean operation contract used by future solid and surface algorithms.

- `src/cad/Tools.h` / `Tools.cpp`  
  CAD tool state machine. Defines tool types and handles drawing workflow for each tool, including previews and command input behavior.

- `CMakeLists.txt`  
  Build definition for the Qt Widgets and OpenGL application. Future 3D and exchange-format integrations should be added here as optional, isolated dependencies.

## Main Components

### 1. Main Window
The `MainWindow` class is the central UI shell.

Responsibilities:
- create the application window,
- build toolbars,
- add dock widgets for layers and properties,
- manage command input,
- connect drawing actions to the viewport,
- handle file open/save and document updates.

### 2. CAD Document
The `Document` class stores the current design state.

Responsibilities:
- list of CAD entities,
- active layer,
- layer management,
- saving/loading drawing data,
- selection and property updates.

The document model is the natural ownership point for future assemblies, external references, object metadata, and spatial indexing needed by large drawings.

### 3. Entities
Each entity represents a drawable CAD object.

Base class: `Entity`
- id
- type
- layer
- draw()
- hitTest()
- toJson()
- moveBy()
- properties()

Concrete shapes include:
- circle,
- ellipse,
- rectangle,
- polyline,
- and other geometry entities.

### 4. Viewport
The `CadViewport` is the drawing surface.

Responsibilities:
- paint entities on a 2D canvas,
- render previews for current tool actions,
- support mouse events,
- show grid and snap behavior,
- transform world coordinates to screen coordinates.

The current renderer is a Qt/OpenGL-backed 2D viewport. A future 3D renderer can reuse the document and geometry contracts while adding camera state, mesh buffers, lighting, depth testing, and selection IDs.

### 5. Tool Controller
The `ToolController` manages current editing behavior.

Responsibilities:
- active tool selection,
- interaction flow for each tool,
- point collection for creating shapes,
- geometry calculations for previews,
- command-driven editing operations.

## Tooling Model
The app uses a `ToolType` enum for the editing state, including:
- Select
- Line
- Circle
- Ellipse
- Arc
- Polyline
- Polygon
- Rectangle
- Offset
- Trim
- Extend
- Fillet
- Chamfer

This allows the UI and command parser to switch between drawing modes cleanly.

## Geometry Model
The geometry utilities centralize mathematical formulas so calculations are consistent across the app.

Examples:
- circle diameter = 2 × radius
- circle circumference = 2πr
- circle area = πr²
- ellipse area = πab
- ellipse perimeter approximation = Ramanujan-based approximation
- triangle perimeter and area
- polygon area
- 2D affine-style transforms and 3D rotation/scale/translation transforms
- segment, triangle, and circle intersection tests
- triangle mesh data for tessellation and rendering pipelines

This supports property display for generated shapes and establishes the common vocabulary for later solid, mesh, and collision systems.

## Advanced Capability Roadmap

### Boolean operations

The geometry API identifies union, difference, intersection, and XOR operations. The next production step is to implement these operations on validated closed profiles or B-Rep solids, including tolerance handling, coplanar edges, self-intersections, and topology repair.

### Solid modeling

Solid support should be built around a mature B-Rep or boundary-representation kernel. Planned operations include extrude, revolve, sweep, shell, fillet, chamfer, volume calculation, and solid validation.

### Mesh generation and rendering

Closed profiles and solids should be tessellated into indexed triangle meshes with normals and material data. The viewport can then support wireframe, shaded, and selected-object rendering modes while preserving the existing drafting workflow.

### Coordinates and collision detection

World, object, and screen coordinate systems should be explicit. Intersection services should use shared tolerances and support broad-phase filtering for large drawings before detailed segment, curve, mesh, or solid tests.

### Large drawings and assemblies

Large projects require spatial indexing, lazy rendering, document partitioning, external references, instance transforms, and assembly metadata. These should be added behind the `Document` API so existing tools remain compatible.

### File formats

The native `.gfcad` JSON format remains the application persistence format. DWG and DXF support should be provided through a dedicated translator, while STEP and IGES should be connected through a 3D geometry kernel or exchange library. Import/export should be isolated from the core entity model and report unsupported entities instead of silently dropping them.

## UI Layout
The main window is organized with:
- a top toolbar for CAD tools,
- left dock for layers,
- right dock for properties,
- bottom dock for commands,
- status bar for coordinates and app status.

This gives the app a classic CAD-style workspace.

## Data Flow
A typical interaction works like this:
1. User selects a tool from the toolbar or command line.
2. `MainWindow` calls the current tool in `ToolController`.
3. `ToolController` collects mouse points and creates preview geometry.
4. The viewport draws the preview and final entities.
5. The document stores the finalized geometry.
6. Properties and layer information update in the UI.

## Build System
The project uses CMake with Qt 6.

Key build configuration includes:
- `CMAKE_AUTOMOC ON`
- `CMAKE_AUTOUIC ON`
- `CMAKE_AUTORCC ON`
- Qt components: `Widgets`, `OpenGL`, `OpenGLWidgets`

## Summary
The architecture is a compact CAD desktop app with:
- Qt UI shell,
- document model,
- shape entity system,
- viewport renderer,
- tool controller,
- geometry and transformation helper library,
- mesh and intersection foundations,
- a staged path toward Boolean operations, solids, assemblies, and engineering file formats.

This is a solid foundation for a 2D CAD application with interactive editing tools and a clear migration path toward a full engineering CAD platform. Production-grade 3D modeling and format interoperability should use established libraries such as Open CASCADE Technology rather than a custom B-Rep implementation in the application layer.
