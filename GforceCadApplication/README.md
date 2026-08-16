# GForce CAD Application

A C++20 / Qt 6 / OpenGL starter CAD application designed as the foundation for an AI-native construction/CAD platform.

## Technology

- C++20
- Qt 6 Widgets
- QOpenGLWidget
- OpenGL-backed viewport
- CMake
- JSON document persistence
- Undo/redo command stack
- 2D geometry engine
- Grid/snap
- Layers
- Interactive drawing tools

Qt's `QOpenGLWidget` is used as the rendering viewport. Qt documents `QOpenGLWidget` as the standard widget for integrating OpenGL rendering with Qt applications.

## Current features

- New drawing
- Open/save `.gfcad` JSON drawing files
- Line
- Circle
- Arc
- Polyline
- Rectangle
- Select
- Delete
- Undo / Redo
- Grid
- Snap-to-grid
- Orthographic toggle
- Zoom with mouse wheel
- Pan with middle mouse button
- Layer panel
- Properties panel
- Command line
- Coordinate/status display
- Dark CAD-style interface

## Build on Windows

Install:

1. Qt 6.x with Desktop C++ compiler support
2. CMake 3.21+
3. Visual Studio 2022 with Desktop C++ workload

Then:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
.\build\Release\GforceCadApplication.exe
```

If CMake cannot find Qt, configure with:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
```

Adjust the Qt path to your installation.

## Build with Qt Creator

1. Open `CMakeLists.txt`.
2. Select a Qt 6 Desktop kit.
3. Configure.
4. Build.
5. Run.

## Controls

| Action | Control |
|---|---|
| Select | Esc / toolbar |
| Line | Toolbar or command `LINE` |
| Circle | Toolbar or command `CIRCLE` |
| Arc | Toolbar or command `ARC` |
| Polyline | Toolbar or command `POLYLINE` |
| Rectangle | Toolbar or command `RECTANGLE` |
| Delete | Delete key |
| Pan | Middle mouse drag |
| Zoom | Mouse wheel |
| Snap | Toggle Snap |
| Grid | Toggle Grid |
| Cancel tool | Esc |

## Architecture

```text
GforceCadApplication
|
+-- UI
|   +-- MainWindow
|   +-- Toolbars
|   +-- Layers
|   +-- Properties
|   +-- Command line
|
+-- CAD Core
|   +-- Document
|   +-- Entities
|   +-- Geometry
|   +-- Layers
|   +-- Undo/Redo
|
+-- Rendering
|   +-- QOpenGLWidget
|   +-- Grid
|   +-- World/screen transform
|   +-- Selection
|
+-- Tools
    +-- Select
    +-- Line
    +-- Circle
    +-- Arc
    +-- Polyline
    +-- Rectangle
```

## Roadmap

### Phase 2
- DXF import/export
- Dimensions
- Text
- Blocks
- Layer colors
- Linetypes
- Object properties
- Better selection
- Trim/extend
- Offset
- Fillet/chamfer

### Phase 3
- 3D viewport
- Open CASCADE Technology integration
- B-Rep solids
- Extrude/revolve
- Boolean union/subtract/intersection
- STEP/IGES
- Surface modeling

### Phase 4
- AI CAD command interpreter
- Natural-language drawing
- Construction-plan assistant
- Automated quantity extraction
- AI drawing review
- RAG-based standards assistant
- Agentic CAD workflows

## Important

This is a functional CAD foundation, not an AutoCAD replacement. A production-grade 3D CAD kernel should use a mature geometry kernel such as Open CASCADE Technology rather than implementing B-Rep and Boolean geometry from scratch.
