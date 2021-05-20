/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: <type your USC username here>
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cstring>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

//merged from startercode 2

// represents one control point along the spline 
struct Point
{
    double x;
    double y;
    double z;

    double length(Point b) {
        double ret = sqrt(pow(x - b.x, 2) + pow(y - b.y, 2) + pow(z - b.z, 2));
        return ret;
    }

    Point() {
        x = 0;
        y = 0;
        z = 0;
    }

    Point(double a, double b, double c) {
        x = a;
        y = b;
        z = c;
    }

    Point cross(Point b) {
        Point ret;
        ret.x = y * b.z - z * b.y;
        ret.y = z * b.x - x * b.z;
        ret.z = x * b.y - y * b.x;
        return ret;
    }

    Point operator+(const Point& b)
    {
        return Point(x + b.x, y + b.y, z + b.z);
    }
    Point operator-(const Point& b)
    {
        return Point(x - b.x, y - b.y, z - b.z);
    }


    Point operator*(double b) {
        return Point(b * x, b * y, b * z);
    }

    
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
    int numControlPoints;
    Point* points;
};

// the spline array 
Spline* splines;
// total number of splines 
int numSplines;


int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

ImageIO * heightmapImage;

GLuint splineVertexBuffer, splineColorVertexBuffer;
GLuint splineVertexArray;
int sizeSpline;
GLuint crossVertexBuffer, crossColorVertexBuffer;
GLuint crossVertexArray;
int sizeCross;

GLuint texVertexBuffer, texCoordVertexBuffer;
GLuint texVertexArray;
GLuint texHandle;
int sizeTex;

GLuint skyVertexBuffer, skyCoordVertexBuffer;
GLuint skyVertexArray;
GLuint skyHandle;
int sizeSky;








GLuint pointVertexBuffer, pointColorVertexBuffer;
GLuint lineVertexBuffer, lineColorVertexBuffer;
GLuint triVertexBuffer, triColorVertexBuffer;
GLuint smoothVertexBuffer, smoothColorVertexBuffer;

GLuint SmoothLeftBuffer, SmoothRightBuffer, SmoothUpBuffer, SmoothDownBuffer;

GLuint pointVertexArray;
GLuint lineVertexArray;
GLuint triVertexArray;
GLuint smoothVertexArray;

int sizePoint;
int sizeLine;
int sizeTri;
int sizeSmooth;

OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;
BasicPipelineProgram* texturePipeline;
BasicPipelineProgram* skyPipeline;

int mode = 1;
int AnimationFrameCount = 0;

int currentPosition = 0;
int velocity = 0;
//int acceleration = 0;

vector<Point> splinePoints; //vector for storing points on spline.
vector<Point> splineTangents; //vector for storing tangent at each point; correspond to the index of splinePoints
vector<Point> splineNormals;
vector<Point> splineBinormals;

int numVerticesCross = 0;
vector<float> vertCross;
vector<float> colCross;

int numVerticesTex = 6;
vector<float> vertTex;
vector<float> coordTex;

int numVerticesSky = 30;
vector<float> vertSky;
vector<float> coordSky;
//glm::vec3* vertCross = new glm::vec3[numVerticesCross];
//glm::vec4* colCross = new glm::vec4[numVerticesCross];

//GLint h_mode; //handle to mode variable in shader
//SET_SHADER_VARIABLE_HANDLE(mode);

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

int loadSplines(char* argv)
{
    char* cName = (char*)malloc(128 * sizeof(char));
    FILE* fileList;
    FILE* fileSpline;
    int iType, i = 0, j, iLength;

    // load the track file 
    fileList = fopen(argv, "r");
    if (fileList == NULL)
    {
        printf("can't open file\n");
        exit(1);
    }

    // stores the number of splines in a global variable 
    fscanf(fileList, "%d", &numSplines);

    splines = (Spline*)malloc(numSplines * sizeof(Spline));

    // reads through the spline files 
    for (j = 0; j < numSplines; j++)
    {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");

        if (fileSpline == NULL)
        {
            printf("can't open file\n");
            exit(1);
        }

        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);

        // allocate memory for all the points
        splines[j].points = (Point*)malloc(iLength * sizeof(Point));
        splines[j].numControlPoints = iLength;

        // saves the data to the struct
        while (fscanf(fileSpline, "%lf %lf %lf",
            &splines[j].points[i].x,
            &splines[j].points[i].y,
            &splines[j].points[i].z) != EOF)
        {
            i++;
        }
    }

    free(cName);

    return 0;
}

int initTexture(const char* imageFilename, GLuint textureHandle)
{
    // read the texture image
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(imageFilename, &imgFormat);

    if (err != ImageIO::OK)
    {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }

    // check that the number of bytes is a multiple of 4
    if (img.getWidth() * img.getBytesPerPixel() % 4)
    {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }

    // allocate space for an array of pixels
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

    // fill the pixelsRGBA array with the image pixels
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++)
        {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }

    // bind the texture
    glBindTexture(GL_TEXTURE_2D, textureHandle);

    // initialize the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

    // generate the mipmaps for this texture
    glGenerateMipmap(GL_TEXTURE_2D);

    // set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // query support for anisotropic texture filtering
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // set anisotropic texture filtering
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

    // query for any errors
    GLenum errCode = glGetError();
    if (errCode != 0)
    {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }

    // de-allocate the pixel array -- it is no longer needed
    delete[] pixelsRGBA;

    return 0;
}

double distance(Point p) {
    double ret = sqrt(pow(p.x, 2) + pow(p.y, 2) + pow(p.y, 2));
    return ret;
}

Point normalize(Point p) {
    Point ret = p;
    Point origin;
    origin.x = 0;
    origin.y = 0;
    origin.z = 0;
    double dis = origin.length(p);
    ret.x = ret.x / dis;
    ret.y = ret.y / dis;
    ret.z = ret.z / dis;
    return ret;
}

void setCam() {
    //if (currentPosition >= splinePoints.size()) {
        //currentPosition = splinePoints.size() - 1;
    //}

    if (currentPosition < 0) {
        currentPosition = 0;
    }
    else if (currentPosition >= splinePoints.size()) {
        currentPosition = splinePoints.size() - 1;
    }
    //printf("currentPosition: %d\n", currentPosition);
    Point eye = splinePoints[currentPosition];
    eye = eye + splineBinormals[currentPosition] * 0.1;
    Point n = splineNormals[currentPosition];
    //Point n;
    //n.x = 0;
    //n.y = 1;
    //n.z = 0;
    float dis = 0.2; //distance from track
    eye.x += n.x * dis;
    eye.y += n.y * dis;
    eye.z += n.z * dis;
    Point center = eye;
    center.x += splineTangents[currentPosition].x;
    center.y += splineTangents[currentPosition].y;
    center.z += splineTangents[currentPosition].z;
    
    
    matrix.LookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, n.x, n.y, n.z);
}

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();

  setCam();


  //matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  //matrix.Rotate(landRotate[0], 1.0f, 0.0f, 0.0f);
  //matrix.Rotate(landRotate[1], 0.0f, 1.0f, 0.0f);
  //matrix.Rotate(landRotate[2], 0.0f, 0.0f, 1.0f);
  //matrix.Scale(landScale[0], landScale[1], landScale[2]);


  float m[16];
  //matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);
  float n[16];
  matrix.GetNormalMatrix(n); // get normal matrix


  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);

  
  //
  // bind shader
  pipelineProgram->Bind();

  // set variable
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);
  //pipelineProgram->SetNormalMatrix(n);  //set normal matrix
  glUniformMatrix4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "normalMatrix"), 1, GL_FALSE, n);
  

  //glBindVertexArray(triVertexArray);
  //glDrawArrays(GL_TRIANGLES, 0, sizeTri);

  texturePipeline->Bind();
  texturePipeline->SetModelViewMatrix(m);
  texturePipeline->SetProjectionMatrix(p);

  skyPipeline->Bind();
  skyPipeline->SetModelViewMatrix(m);
  skyPipeline->SetProjectionMatrix(p);

  
  pipelineProgram->Bind();
   glBindVertexArray(crossVertexArray);
   glDrawArrays(GL_TRIANGLES, 0, sizeCross);
   //pipelineProgram->SetMode(1);
   //texture
   texturePipeline->Bind();
   glBindTexture(GL_TEXTURE_2D, texHandle);
   glBindVertexArray(texVertexArray);
   glDrawArrays(GL_TRIANGLES, 0, sizeTex);
   //sky texture
   skyPipeline->Bind();
   glBindTexture(GL_TEXTURE_2D, skyHandle);
   glBindVertexArray(skyVertexArray);
   glDrawArrays(GL_TRIANGLES, 0, sizeSky);


 
   

  glutSwapBuffers();
}

void idleFunc()
{
  // do some stuff... 

  // for example, here, you can save the screenshots to disk (to make the animation)

  // make the screen update 
  //clock()
  currentPosition += 1;
  
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 100.0f);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'w':
        currentPosition += 10;
    break;

    case 's':
        currentPosition -= 10;
    break;

    case 'x': {
        // take a screenshot
        string zeros = "";
        if (AnimationFrameCount < 10) {
            zeros = "00";
        }
        else if (AnimationFrameCount < 100) {
            zeros = "0";
        }
        else {
            zeros = "";
        }
        string screenshotname = "Animation\\"+ zeros + std::to_string(AnimationFrameCount) + ".jpg";
        AnimationFrameCount++;
        saveScreenshot(const_cast<char*>(screenshotname.c_str()));
        break; }

    case '1':
        //points
        //glUniform1i(GLint location,GLint v0);
        mode = 1;
            
    break;
    case '2':
        //lines
        mode = 2;
        break;
    case '3':
        //triangles
        mode = 3;
        break;
    case '4':
        //smooth
        mode = 4;
        break;
    /*
    case 'r':
        
        controlState = ROTATE;
        break;

    case 't':
        //cout << "RRR" << endl;
        controlState = TRANSLATE;
        break;

    case 's':
        controlState = SCALE;
        break;
        */
  }
}

//helper for matrix calculation

float* VectorMultiply4(float matrix[4][4], float vector[4])
{
    float* ret = new float[4];
    for (int i = 0; i < 4; i++)
    {
        float current = 0;
        for (int j = 0; j < 4; j++)
        {
            current += vector[j] * matrix[j][i];
        }
        ret[i] = current;
    }
    return ret;
}

float* VectorMultiply3(float matrix[4][3], float vector[4])
{
    float* ret = new float[3];
    for (int i = 0; i < 3; i++)
    {
        float current = 0;
        for (int j = 0; j < 4; j++)
        {
            current += vector[j] * matrix[j][i];
        }
        ret[i] = current;
    }
    return ret;
}



Point CatmullRom(Point p1, Point p2, Point p3, Point p4, float u) {
    
    float basis[4][4] = {
        {-0.5,1.5,-1.5,0.5},
        {1.0,-2.5,2.0,-0.5},
        {-0.5,0,0.5,0},
        {0,1,0,0}
    };
    float control[4][3] = {
        {p1.x,p1.y,p1.z},
        {p2.x,p2.y,p2.z},
        {p3.x,p3.y,p3.z},
        {p4.x,p4.y,p4.z}
    };
    float vec[4] = { pow(u,3),pow(u,2) ,pow(u,1) ,1 };
    float* step1 = VectorMultiply4(basis, vec);
    float vec2[4] = { step1[0],step1[1], step1[2], step1[3] };
    float* step2 = VectorMultiply3(control, vec2);
    Point ret;
    ret.x = step2[0];
    ret.y = step2[1];
    ret.z = step2[2];
    return ret;
    //float4 umatrix
}

Point CatmullTangent(Point p1, Point p2, Point p3, Point p4, float u) {
    float basis[4][4] = {
        {-0.5,1.5,-1.5,0.5},
        {1.0,-2.5,2.0,-0.5},
        {-0.5,0,0.5,0},
        {0,1,0,0}
    };
    float control[4][3] = {
        {p1.x,p1.y,p1.z},
        {p2.x,p2.y,p2.z},
        {p3.x,p3.y,p3.z},
        {p4.x,p4.y,p4.z}
    };
    float vec[4] = { 3*pow(u,2),2*pow(u,1) ,1 ,0 };
    float* step1 = VectorMultiply4(basis, vec);
    float vec2[4] = { step1[0],step1[1], step1[2], step1[3] };
    float* step2 = VectorMultiply3(control, vec2);
    Point ret;
    ret.x = step2[0];
    ret.y = step2[1];
    ret.z = step2[2];
    return ret;
}

void setPhongValue() {
    pipelineProgram->Bind();

    float La[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float Ld[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float Ls[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    float ka[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float kd[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
    float ks[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    float alpha = 0.1;

    Point viewLight = { 1,-1,1 };
    viewLight = normalize(viewLight);
    float viewLightDirection[3] = { viewLight.x, viewLight.y, viewLight.z };
    glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "La"), 1, La);
    glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ld"), 1, Ld);
    glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ls"), 1, Ls);
    glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ka"), 1, ka);
    glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "kd"), 1, kd);
    glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ks"), 1, ks);
    glUniform3fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "viewLightDirection"), 1, viewLightDirection);
    glUniform1f(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "alpha"), alpha);
}


void subdivide(float u0, float u1, float maxlinelength, int i, int splineNum) {
    float umid = (u0 + u1) / 2;
    //splinePoints.push_back(p1);
    Point x0 = CatmullRom(splines[splineNum].points[i], splines[splineNum].points[i + 1], splines[splineNum].points[i + 2], splines[splineNum].points[i + 3], u0);
    Point x1 = CatmullRom(splines[splineNum].points[i], splines[splineNum].points[i + 1], splines[splineNum].points[i + 2], splines[splineNum].points[i + 3], u1);
    Point tan = CatmullTangent(splines[splineNum].points[i], splines[splineNum].points[i + 1], splines[splineNum].points[i + 2], splines[splineNum].points[i + 3], u0);

    if (x0.length(x1) > maxlinelength) {
        
        subdivide(u0, umid, maxlinelength, i, splineNum);
        subdivide(umid, u1, maxlinelength, i, splineNum);
    }
    else {
    //draw the line

        splinePoints.push_back(x0);
        splineTangents.push_back(normalize(tan));
        if (splineNormals.size() == 0) {
            Point b; //arbitrary vector
            b.x = 0;
            b.y = 1;
            b.z = 0;
            //splineNormals.push_back(normalize(splineTangents[0].cross(b)));
            splineNormals.push_back(b);
            splineBinormals.push_back(normalize(splineTangents[0].cross(splineNormals[0])));
        }
        else {
            int i = splineTangents.size()-1;
            splineNormals.push_back(normalize(splineBinormals[i-1].cross(splineTangents[i])));
            splineBinormals.push_back(normalize(splineTangents[i].cross(splineNormals[i])));
        }
    }
}

void drawSpline(int num) {
    //int numVerticesSpline = num * 2;
    //glm::vec3* vertSpline = new glm::vec3[numVerticesSpline];
    //glm::vec4* colSpline = new glm::vec4[numVerticesSpline];
    int lines = splines[num].numControlPoints - 3;
    for (int i = 0; i < lines; i++) {
        subdivide(0.0, 1.0, 0.01, i, num);
        //splinePoints.push_back(splines[num].points[i]);
    }
    

}

void addTriangle(Point a, Point b, Point c, Point n) {
    vertCross.push_back(a.x); vertCross.push_back(a.y); vertCross.push_back(a.z);
    vertCross.push_back(b.x); vertCross.push_back(b.y); vertCross.push_back(b.z);
    vertCross.push_back(c.x); vertCross.push_back(c.y); vertCross.push_back(c.z);
    //Point normal = normalize((a - c).cross(b - c));
    Point normal = n;
    for (int i = 0; i < 3; i++) {
       colCross.push_back((float)normal.x);
       colCross.push_back((float)normal.y);
       colCross.push_back((float)normal.z);
       //colCross.push_back(0);
    }
    
}

void drawCrossSection() {
    for (int i = 0; i < splinePoints.size()-1; i++) {
        Point p0 = splinePoints[i];
        Point n0 = splineNormals[i];
        Point b0 = splineBinormals[i];
        Point p1 = splinePoints[i+1];
        Point n1 = splineNormals[i+1];
        Point b1 = splineBinormals[i+1];
        Point v0, v1, v2, v3, v4, v5, v6, v7;
        double size = 0.03;
        v0 = p0 + ((b0 - n0) * size);
        v1 = p0 + ((b0 + n0) * size);
        v2 = p0 + ((n0 - b0) * size);
        v3 = p0 + ((Point(0,0,0) -n0 - b0) * size);

        v4 = p1 + ((b1 - n1) * size);
        v5 = p1 + ((b1 + n1) * size);
        v6 = p1 + ((n1 - b1) * size);
        v7 = p1 + ((Point(0, 0, 0) - n1 - b1) * size);
        addTriangle(v0,v4,v5 , splineBinormals[i]);
        addTriangle(v0, v5, v1, splineBinormals[i]);

        addTriangle(v1, v5, v2, splineNormals[i]);
        addTriangle(v2, v5, v6, splineNormals[i]);

        addTriangle(v3, v2, v6, Point(0,0,0) - splineBinormals[i]);
        addTriangle(v3, v6, v7, Point(0, 0, 0) - splineBinormals[i]);

        addTriangle(v0, v3, v7, Point(0, 0, 0) - splineNormals[i]);
        addTriangle(v0, v7, v4, Point(0, 0, 0) - splineNormals[i]);
    }

    for (int i = 0; i < splinePoints.size() - 1; i++) {
        Point p0 = splinePoints[i];
        Point n0 = splineNormals[i];
        Point b0 = splineBinormals[i];
        Point p1 = splinePoints[i + 1];
        Point n1 = splineNormals[i + 1];
        Point b1 = splineBinormals[i + 1];
        p0 = p0 + b0 * 0.2;
        p1 = p1 + b1 * 0.2;
        Point v0, v1, v2, v3, v4, v5, v6, v7;
        double size = 0.03;
        v0 = p0 + ((b0 - n0) * size);
        v1 = p0 + ((b0 + n0) * size);
        v2 = p0 + ((n0 - b0) * size);
        v3 = p0 + ((Point(0, 0, 0) - n0 - b0) * size);

        v4 = p1 + ((b1 - n1) * size);
        v5 = p1 + ((b1 + n1) * size);
        v6 = p1 + ((n1 - b1) * size);
        v7 = p1 + ((Point(0, 0, 0) - n1 - b1) * size);
        addTriangle(v0, v4, v5, splineBinormals[i]);
        addTriangle(v0, v5, v1, splineBinormals[i]);

        addTriangle(v1, v5, v2, splineNormals[i]);
        addTriangle(v2, v5, v6, splineNormals[i]);

        addTriangle(v3, v2, v6, Point(0, 0, 0) - splineBinormals[i]);
        addTriangle(v3, v6, v7, Point(0, 0, 0) - splineBinormals[i]);

        addTriangle(v0, v3, v7, Point(0, 0, 0) - splineNormals[i]);
        addTriangle(v0, v7, v4, Point(0, 0, 0) - splineNormals[i]);
    }


}

void drawGround() {
    float size = 300.0f;
    float height = -10.0f;
    float coord = 10.0f;
    vertTex.push_back(size); vertTex.push_back(height); vertTex.push_back(size);
    vertTex.push_back(-size); vertTex.push_back(height); vertTex.push_back(size);
    vertTex.push_back(-size); vertTex.push_back(height); vertTex.push_back(-size);
    vertTex.push_back(size); vertTex.push_back(height); vertTex.push_back(size);
    vertTex.push_back(-size); vertTex.push_back(height); vertTex.push_back(-size);
    vertTex.push_back(size); vertTex.push_back(height); vertTex.push_back(-size);
    coordTex.push_back(2*coord); coordTex.push_back(2*coord);
    coordTex.push_back(0); coordTex.push_back(2 * coord);
    coordTex.push_back(0); coordTex.push_back(0);
    coordTex.push_back(2 * coord); coordTex.push_back(2 * coord);
    coordTex.push_back(0); coordTex.push_back(0);
    coordTex.push_back(2 * coord); coordTex.push_back(0);
}

void drawSky() {
    float size = 50.0f;
    float height1 = 50.0f;
    float height2 = -10.0f;
    float coord = 0.5f;
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(-size);
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(-size);
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(-size);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(0);

    //front
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(-size); vertSky.push_back(height2); vertSky.push_back(size);
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(-size); vertSky.push_back(height2); vertSky.push_back(size);
    vertSky.push_back(size); vertSky.push_back(height2); vertSky.push_back(size);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(0);

    //right
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(-size);
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(size); vertSky.push_back(height2); vertSky.push_back(size);
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(-size);
    vertSky.push_back(size); vertSky.push_back(height2); vertSky.push_back(size);
    vertSky.push_back(size); vertSky.push_back(height2); vertSky.push_back(-size);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(0);

    //back
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(-size);
    vertSky.push_back(size); vertSky.push_back(height1); vertSky.push_back(-size);
    vertSky.push_back(size); vertSky.push_back(height2); vertSky.push_back(-size);
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(-size);
    vertSky.push_back(size); vertSky.push_back(height2); vertSky.push_back(-size);
    vertSky.push_back(-size); vertSky.push_back(height2); vertSky.push_back(-size);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(0);

    //left
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(-size);
    vertSky.push_back(-size); vertSky.push_back(height2); vertSky.push_back(-size);
    vertSky.push_back(-size); vertSky.push_back(height1); vertSky.push_back(size);
    vertSky.push_back(-size); vertSky.push_back(height2); vertSky.push_back(-size);
    vertSky.push_back(-size); vertSky.push_back(height2); vertSky.push_back(size);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(2 * coord);
    coordSky.push_back(0); coordSky.push_back(0);
    coordSky.push_back(2 * coord); coordSky.push_back(0);
}

void initScene(int argc, char *argv[])
{
  //// load the image from a jpeg disk file to main memory
  //heightmapImage = new ImageIO();

  //if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  ////if (heightmapImage->loadJPEG("heightmap/OhioPyle-128.jpg") != ImageIO::OK)
  //{
  //  cout << "Error reading image " << argv[1] << "." << endl;
  //  exit(EXIT_FAILURE);
  //}

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
 
  // modify the following code accordingly
  
  //int imageHeight = heightmapImage->getHeight();
  //int imageWidth = heightmapImage->getWidth();
  ////points
  //int numVertices = imageHeight * imageWidth;
  //glm::vec3 * vertPoints = new glm::vec3[imageHeight * imageWidth];
  //glm::vec4 * colPoints = new glm::vec4[imageHeight * imageWidth];
  //// (… fill the “vertices” array …)
  //float scale = 1;
  //float imageScale = 4.5;
  //for (int i = 0; i < imageHeight; i++) {
  //    for (int j = 0; j < imageWidth; j++) {
  //      float height = scale * heightmapImage->getPixel(i, j, 0);
  //      //cout << height << endl;
  //      vertPoints[i * imageWidth + j] = glm::vec3(-(float)(i - imageHeight / 2) / imageHeight * imageScale,(float)height/256 , -(float)(j - imageWidth / 2) / imageWidth * imageScale);
  //      colPoints[i * imageWidth + j] = glm::vec4((float)height/ 256, (float)height / 256, (float)height / 256, 0);
  //    }
  //}
  //// create the VBO:
  ////GLuint vbo;

  //glGenBuffers(1, &pointVertexBuffer);
  //glBindBuffer(GL_ARRAY_BUFFER, pointVertexBuffer);
  //glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * numVertices,
  //    vertPoints, GL_STATIC_DRAW);

  //glGenBuffers(1, &pointColorVertexBuffer);
  //glBindBuffer(GL_ARRAY_BUFFER, pointColorVertexBuffer);
  //glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * numVertices, colPoints, GL_STATIC_DRAW);

  //pipelineProgram = new BasicPipelineProgram;
  //int ret = pipelineProgram->Init(shaderBasePath);
  //if (ret != 0) abort();

  //glGenVertexArrays(1, &pointVertexArray);
  //glBindVertexArray(pointVertexArray);
  //glBindBuffer(GL_ARRAY_BUFFER, pointVertexBuffer);

  //GLuint loc =
  //    glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  //glEnableVertexAttribArray(loc);
  //glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  //glBindBuffer(GL_ARRAY_BUFFER, pointColorVertexBuffer);
  //loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  //glEnableVertexAttribArray(loc);
  //glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  //glEnable(GL_DEPTH_TEST);

  //sizePoint = numVertices;
  

  drawSpline(0);
  //draw a basic spline:
  /*
  

  int numVerticesSpline = (splinePoints.size() - 1) * 2;
  glm::vec3* vertSpline = new glm::vec3[numVerticesSpline];
  glm::vec4* colSpline = new glm::vec4[numVerticesSpline];
  for (int i = 0; i < splinePoints.size() - 1; i++) {
      vertSpline[2 * i] = glm::vec3(splinePoints[i].x, splinePoints[i].y, splinePoints[i].z);
      vertSpline[2 * i + 1] = glm::vec3(splinePoints[i + 1].x, splinePoints[i + 1].y, splinePoints[i + 1].z);
      colSpline[2 * i] = glm::vec4(1.0, 1.0, 1.0, 0);
      colSpline[2 * i + 1] = glm::vec4(1.0, 1.0, 1.0, 0);
  }

  glGenBuffers(1, &splineVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, splineVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * numVerticesSpline,
      vertSpline, GL_STATIC_DRAW);

  glGenBuffers(1, &splineColorVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, splineColorVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * numVerticesSpline, colSpline, GL_STATIC_DRAW);

  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  glGenVertexArrays(1, &splineVertexArray);
  glBindVertexArray(splineVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, splineVertexBuffer);

  GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, splineColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  glEnable(GL_DEPTH_TEST);

  sizeSpline = numVerticesSpline;
  */

  //draw
  drawCrossSection();

  numVerticesCross = (splinePoints.size() - 1) * 24;

  glGenBuffers(1, &crossVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, crossVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertCross.size()*sizeof(float) , vertCross.data(),
      GL_STATIC_DRAW);

  glGenBuffers(1, &crossColorVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, crossColorVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, colCross.size() * sizeof(float) , colCross.data(), GL_STATIC_DRAW);

  glGenVertexArrays(1, &crossVertexArray);
  glBindVertexArray(crossVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, crossVertexBuffer);

  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  GLuint loc =
      glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, crossColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  glEnable(GL_DEPTH_TEST);


  sizeCross = numVerticesCross*2;

  //set phong value
  setPhongValue();

  //render ground
  glGenTextures(1, &texHandle);
  int code = initTexture("water.jpg", texHandle);
  if (code != 0)
  {
      printf("Error loading the texture image.\n");
      exit(EXIT_FAILURE);
  }

  code = initTexture("sky2.jpg", skyHandle);
  if (code != 0)
  {
      printf("Error loading the texture image.\n");
      exit(EXIT_FAILURE);
  }

  drawGround();
  drawSky();

  glGenBuffers(1, &texVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, texVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertTex.size() * sizeof(float), vertTex.data(),
      GL_STATIC_DRAW);

  glGenBuffers(1, &texCoordVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, texCoordVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, coordTex.size() * sizeof(float), coordTex.data(), GL_STATIC_DRAW);

  texturePipeline = new BasicPipelineProgram;
  ret = texturePipeline->Init(shaderBasePath, "texture.vertexShader.glsl", "texture.fragmentShader.glsl");
  if (ret != 0) abort();


  glGenVertexArrays(1, &texVertexArray);
  glBindVertexArray(texVertexArray);
  
  glBindBuffer(GL_ARRAY_BUFFER, texVertexBuffer);
  loc =glGetAttribLocation(texturePipeline->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, texCoordVertexBuffer);
  loc = glGetAttribLocation(texturePipeline->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc);
  const void* offset = (const void*)(sizeof(float)*sizeTex*3); 
  GLsizei stride = 0;
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);
  //glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  glEnable(GL_DEPTH_TEST);

  sizeTex = numVerticesTex;


  // render sky
  glGenBuffers(1, &skyVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, skyVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, vertSky.size() * sizeof(float), vertSky.data(),
      GL_STATIC_DRAW);

  glGenBuffers(1, &skyCoordVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, skyCoordVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, coordSky.size() * sizeof(float), coordSky.data(), GL_STATIC_DRAW);

  skyPipeline = new BasicPipelineProgram;
  ret = skyPipeline->Init(shaderBasePath, "texture.vertexShader.glsl", "texture.fragmentShader.glsl");
  if (ret != 0) abort();


  glGenVertexArrays(1, &skyVertexArray);
  glBindVertexArray(skyVertexArray);

  glBindBuffer(GL_ARRAY_BUFFER, skyVertexBuffer);
  loc = glGetAttribLocation(skyPipeline->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, skyCoordVertexBuffer);
  loc = glGetAttribLocation(skyPipeline->GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(loc);
  //const void* offset = (const void*)(sizeof(float) * sizeTex * 3);
  //GLsizei stride = 0;
  glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);
  //glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

  glEnable(GL_DEPTH_TEST);

  sizeSky = 30;

  //TODO: at display draw array; edit shaders;check for replace mistake

  //texturePipeline = new BasicPipelineProgram;
  //ret = texturePipeline->Init(shaderBasePath, "texture.vertexShader.glsl", "texture.fragmentShader.glsl");
  //if (ret != 0) abort();

  
  

  //initial triangle for reference.
  /*
  glm::vec3 triangle[3] = {
    glm::vec3(0, 0, 0), 
    glm::vec3(0, 1, 0),
    glm::vec3(1, 0, 0)
  };

  glm::vec4 color[3] = {
    {0, 0, 1, 1},
    {1, 0, 0, 1},
    {0, 1, 0, 1},
  };
  glGenBuffers(1, &triVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 3, triangle,
               GL_STATIC_DRAW);

  glGenBuffers(1, &triColorVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * 3, color, GL_STATIC_DRAW);


  //pipelineProgram = new BasicPipelineProgram;
  //int ret = pipelineProgram->Init(shaderBasePath);
  //if (ret != 0) abort();

  glGenVertexArrays(1, &triVertexArray);
  glBindVertexArray(triVertexArray);
  glBindBuffer(GL_ARRAY_BUFFER, triVertexBuffer);

  loc =
      glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer);
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glEnable(GL_DEPTH_TEST);
  
  sizeTri = 3;
  */

  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }

    // load the splines from the provided filename
    loadSplines(argv[1]);

    printf("Loaded %d spline(s).\n", numSplines);
    for (int i = 0; i < numSplines; i++)
        printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);




  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


