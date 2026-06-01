#pragma once

#include <unordered_map>
#include <unordered_set>
#include <GLFW/glfw3.h>      // must come before glad
#include <glad/glad.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "glm.hpp"
#include "matrix_transform.hpp"
#include "type_ptr.hpp"

#include "vector"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

#include "Renderer/ChunkMeshManager.h"
#include "World/Player/Player.h"
#include "Renderer/ChunkRenderer.h"
#include "Renderer/ChunkPool.h"
#include "Renderer/Camera.h"
#include "Core.h"




#include "Helpers/Config.h"

using ChunkCoord = glm::vec2;



class Renderer {
public:
    Renderer();
    
    void InitializeInput(void* appPointer);

    void Render(ChunkMeshManager& chunkManager, const PlayerTransform& playerTransform);


    void Cleanup(ChunkMeshManager& chunkManager);

    GLFWwindow* GetWindow() {
        return _window;
    }

    const glm::vec3& GetCameraPosition() {
        return _camera.GetPosition();
    }

    
private:
    Camera _camera;
    int _width = Config::Get().chunkWidth;
    int _height = Config::Get().chunkHeight;
    int _depth = Config::Get().chunkDepth;
    int _viewDistance = Config::Get().viewDistance;
    ChunkRenderer _chunkRenderer = ChunkRenderer(_width, _height, _depth, _viewDistance);

    glm::mat4 _identity;
    glm::mat4 _view;
    glm::mat4 _model;
    glm::mat3 _normalMatrix;
    float _prevTime = 0.0f;

    // FPS counter
    float _fpsAccum = 0.0f;
    int   _fpsFrameCount = 0;
    float _displayedFps = 0.0f;

    float _scale = 0.1f;
    float _amplitude = Config::Get().noiseAmplitude;
    float _frequency = Config::Get().noiseFrequency;
    int _octave = Config::Get().noiseOctaves;
    float _lacunarity = Config::Get().noiseLacunarity;
    float _persistance = Config::Get().noisePersistence;
    

    int _screenWidth = 1920;
    int _screenHeight = 1080;
    glm::mat4 _perspectiveMat;

    GLuint _shaderProgram;
    GLFWwindow* _window;

    GLint _widthLocation;
    GLint _heightLocation;
    GLint _timeLocation;
    GLint _projMLocation;
    GLint _modelMLocation;
    GLint _viewLoc;
    GLint _normalMatrixLocation;
    GLint _textureUniformLoc;
    GLuint textureID;

    // Per-frame multidraw scratch: one command list per pool, holding this frame's
    // visible chunks. A member so the storage is reused instead of realloc'd each frame.
    std::vector<std::vector<DrawElementsIndirectCommand>> _drawCommands;
    // Same for the transparent water pools (second blended pass).
    std::vector<std::vector<DrawElementsIndirectCommand>> _waterDrawCommands;

    void Init();
    std::string ReadFile(const std::string& filePath);
    GLuint CompileShader(GLenum type, const std::string& source);
    GLuint CreateShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);
    void DrawChunks(ChunkMeshManager& chunkManager);
    void ResetToStartValues();
    

};