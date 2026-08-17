# .NET Application Layer

This additive layer is reserved for the future managed application shell around the existing Qt/C++ CAD application.

## Responsibilities

- User interface orchestration
- Commands and command routing
- Project management
- Application services

The existing Qt UI remains the active desktop UI. This layer should communicate with the C++ CAD engine through an explicit interop boundary when implementation begins.
