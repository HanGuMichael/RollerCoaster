﻿#include "basicPipelineProgram.h"
#include "openGLHeader.h"
#include <iostream>
#include <cstring>

using namespace std;

int BasicPipelineProgram::Init(const char * shaderBasePath) 
{
  if (BuildShadersFromFiles(shaderBasePath, "basic.vertexShader.glsl", "basic.fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the basic pipeline program." << endl;
    return 1;
  }

  cout << "Successfully built the basic pipeline program." << endl;
  return 0;
}

int BasicPipelineProgram::Init(const char* shaderBasePath, const char* vertexshader, const char* fragmentShader) {
    if (BuildShadersFromFiles(shaderBasePath, vertexshader, fragmentShader) != 0)
    {
        cout << "Failed to build the pipeline program." << endl;
        return 1;
    }

    cout << "Successfully built the pipeline program." << endl;
    return 0;
}

void BasicPipelineProgram::SetModelViewMatrix(const float * m) 
{
  // Pass "m" to the pipeline program, as the modelview matrix.
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
}

void BasicPipelineProgram::SetProjectionMatrix(const float * m) 
{
  // Pass "m" to the pipeline program, as the projection matrix.
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, m);
}

//void BasicPipelineProgram::SetNormalMatrix(const float* m) 
//{
//   glUniformMatrix4fv(h_normalMatrix, 1, GL_FALSE, m);
//}
//
//void BasicPipelineProgram::SetMode(const int m)
//{
//    glUniform1i(h_mode, m);
//}

int BasicPipelineProgram::SetShaderVariableHandles() 
{
  // Set h_modelViewMatrix and h_projectionMatrix.
  SET_SHADER_VARIABLE_HANDLE(modelViewMatrix);
  SET_SHADER_VARIABLE_HANDLE(projectionMatrix);
  //SET_SHADER_VARIABLE_HANDLE(normalMatrix);
  //SET_SHADER_VARIABLE_HANDLE(mode);
  return 0;
}
