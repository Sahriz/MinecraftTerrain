# MinecraftTerrain

A high-performance, GPU-driven voxel terrain generation engine written in C++ and modern OpenGL (4.3+). 

Inspired by Minecraft, this project pushes the boundaries of standard voxel rendering by offloading almost the entire generation and rendering pipeline—including 3D noise generation, surface culling, geometry assembly, and draw calls—directly to the GPU using **Compute Shaders** and **Indirect Drawing**.

## Key Features

* **100% GPU-Accelerated Pipeline:** Terrain generation operates exclusively on the GPU. The CPU never touches vertex data.
* **Indirect Drawing (`glDrawElementsIndirect`):** Drastically reduces CPU overhead by allowing the Compute Shader to populate draw commands. Thousands of chunks can be rendered with a single API call.
* **Smart VRAM Management:** Uses a two-pass generation system (Atomic Counters/AppendBuffers) to tightly pack and allocate VBOs and IBOs, eliminating wasted memory padding often found in voxel engines.
* **Procedural 3D Noise:** Utilizes Fractal Brownian Motion (FBM) with adjustable octaves, lacunarity, and persistence.
* **Spline-Based Terrain Shaping:** Integrates 2D splines to control terrain height and drop-offs, allowing for distinct biomes and island-like structures.
* **Greedy / Surface Culling:** Compute shaders cull hidden interior voxel faces before geometry is even generated, keeping vertex counts incredibly low.
* **Day/Night Lighting:** Custom vertex/fragment shaders featuring Lambertian reflectance (Ambient + Diffuse) tied to a dynamic sun direction.

## Tech Stack

* **Language:** C++17 (or newer)
* **Graphics API:** OpenGL 4.3+ (Required for Compute Shaders and SSBOs)
* **Window/Input:** [GLFW](https://www.glfw.org/)
* **OpenGL Loader:** [GLAD](https://glad.dav1d.de/)
* **Mathematics:** [GLM](https://github.com/g-truc/glm)
* **UI:** [Dear ImGui](https://github.com/ocornut/imgui)

## How It Works (The GPU Pipeline)

This engine avoids the classic CPU-bottleneck of voxel games by keeping data on the Graphics Card. The chunk generation follows a strict sequential pipeline:

1. **`3DNoise.comp`:** Generates the raw block IDs based on procedural noise and spline height-maps.
2. **`SurfaceCulling.comp`:** Checks neighboring voxels. If a voxel is completely surrounded, it is discarded. Only visible "surface" voxels are added to an `AppendBuffer`.
3. **`CountTriangles.comp`:** Reads the active surface voxels and tallies up exactly how many vertices and indices are required for the chunk.
4. **Tight Allocation (CPU Intervention):** The CPU reads the final count and allocates a perfectly sized VBO and IBO.
5. **`GeometryInit.comp`:** Generates the actual quad vertices, UVs, and Normals. Simultaneously, it uses `atomicAdd` to fill out the `IndirectDrawCommand` buffer.
6. **Render Loop:** The CPU calls `glDrawElementsIndirect()`. The GPU reads its own generated commands and draws the terrain.

## Getting Started

### Prerequisites
Ensure your graphics card drivers support **OpenGL 4.3** or higher. You will need a C++ compiler (MSVC, GCC, or Clang) and CMake (if applicable to your setup).

### Building (Visual Studio)
TODO
