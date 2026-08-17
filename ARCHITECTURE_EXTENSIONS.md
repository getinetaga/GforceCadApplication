# Additive Architecture Extensions

The current Qt/C++ CAD application is preserved as the active implementation. The following directories are additive boundaries for future platform layers:

```text
CAD Application
|
+-- Existing Qt/C++ desktop application
|   +-- GforceCadApplication/src/app       UI shell and commands
|   +-- GforceCadApplication/src/cad       geometry and modeling
|   +-- GforceCadApplication/src/render    rendering and viewport
|
+-- dotnet
|   +-- User interface orchestration
|   +-- Commands
|   +-- Project management
|   +-- Application services
|
+-- python
    +-- AI
    +-- Automation
    +-- Data processing
```

No existing source file is replaced by these extensions. The .NET and Python layers should be introduced through stable contracts and explicit interop with the C++ engine.
