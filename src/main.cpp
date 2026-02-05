#define STB_IMAGE_IMPLEMENTATION
#include "../dep/stb/stb_image.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <string>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 800;
const unsigned int NUM_PATCH_PTS = 4;
int useWireframe = 0;
int displayGrayscale = 0;
// Camera state
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Mouse
float yaw   = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Zoom
float fov = 45.0f;

const char* TCS =R"(
#version 450 core
layout (vertices=4) out;

uniform mat4 model;
uniform mat4 view;

in vec2 TexCoord[];
out vec2 TextureCoord[];

void main(){
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];

  if (gl_InvocationID == 0) {
    const int MIN_TESS_LEVEL = 4;
    const int MAX_TESS_LEVEL = 64;
    const float MIN_DISTANCE = 20;
    const float MAX_DISTANCE = 800;
    vec4 eyeSpacePos00 = view * model * gl_in[0].gl_Position;
    vec4 eyeSpacePos01 = view * model * gl_in[1].gl_Position;
    vec4 eyeSpacePos10 = view * model * gl_in[2].gl_Position;
    vec4 eyeSpacePos11 = view * model * gl_in[3].gl_Position;

    // "distance" from camera scaled between 0 and 1
    float distance00 = clamp( (abs(eyeSpacePos00.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
    float distance01 = clamp( (abs(eyeSpacePos01.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
    float distance10 = clamp( (abs(eyeSpacePos10.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
    float distance11 = clamp( (abs(eyeSpacePos11.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );

    float tessLevel0 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance10, distance00) );
    float tessLevel1 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01) );
    float tessLevel2 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11) );
    float tessLevel3 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance11, distance10) );

    gl_TessLevelOuter[0] = tessLevel0;
    gl_TessLevelOuter[1] = tessLevel1;
    gl_TessLevelOuter[2] = tessLevel2;
    gl_TessLevelOuter[3] = tessLevel3;

    gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
    gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
  }
}
)";
const char* TES = R"(
#version 450 core
layout(quads, fractional_odd_spacing, ccw) in;

uniform sampler2D heightMap;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec2 TextureCoord[];

out vec3 WorldPos;
out vec3 WorldNormal;
out float Height;
void main()
{
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 texCoord = (t1 - t0) * v + t0;

    Height = texture(heightMap, texCoord).y * 64.0 - 16.0;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    vec4 uVec = p01 - p00;
    vec4 vVec = p10 - p00;
    vec4 normal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) );

    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 p = (p1 - p0) * v + p0 + normal * Height;
    
    vec4 worldPos = model * p;
    WorldPos = worldPos.xyz;
    WorldNormal = normalize(mat3(model) * normal.xyz);

    gl_Position = projection * view * worldPos;
}
)";
const char* VS1 = R"(
#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec2 TexCoord;

void main(){
  gl_Position = vec4(aPos, 1.0f);
  TexCoord = aTex;
}
)";

const char* FS1 = R"(
#version 450 core
out vec4 FragColor;

in float Height;
in vec3 WorldPos;
in vec3 WorldNormal;

uniform samplerCube skybox;

void main()
{
    // Normalize height
    float h = clamp((Height + 16.0) / 32.0, 0.0, 1.0);

    // Base terrain color
    vec3 terrainColor = vec3(h);

    // --- Water mask ---
    float waterMask = smoothstep(0.15, 0.30, h);

    // --- Refraction-style sampling ---
    vec3 N = normalize(WorldNormal);

    // Base downward direction
    vec3 down = vec3(0.0, -1.0, 0.0);

    // Small distortion using normal (fake refraction)
    vec3 refractDir = normalize(down + N * 0.15);

    vec3 waterColor = texture(skybox, refractDir).rgb;

    // Final blend
    vec3 finalColor = mix(waterColor, terrainColor, waterMask);

    FragColor = vec4(finalColor, 1.0);
}
)";

const char* VS2 = R"(
#version 450 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoord;

uniform mat4 projection;
uniform mat4 view;

void main(){
  vec4 pos = projection * view * vec4(aPos * 100, 1.0f);
  gl_Position = pos.xyww;
  TexCoord = aPos;
}
)";

const char* FS2 = R"(
#version 450 core

out vec4 FragColor;

in vec3 TexCoord;

uniform samplerCube skybox;
void main(){
  FragColor = texture(skybox, TexCoord);
}
)";
void processInput(GLFWwindow* window)
{
    float cameraSpeed = 10.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)  fov = 1.0f;
    if (fov > 90.0f) fov = 90.0f;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
} 
int main(){
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* w=glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Graphics Pad", NULL, NULL);

  if (w == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(w);
  glfwSetFramebufferSizeCallback(w, framebuffer_size_callback);
  glfwSetCursorPosCallback(w, mouse_callback);
  glfwSetScrollCallback(w, scroll_callback);
  glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  int maxTessLevel;
  glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &maxTessLevel);
  glEnable(GL_DEPTH_TEST);
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  unsigned int vertexShader1;
  vertexShader1 = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader1, 1, &VS1, NULL);
  glCompileShader(vertexShader1);

  unsigned int fragmentShader1;
  fragmentShader1 = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader1, 1, &FS1, NULL);
  glCompileShader(fragmentShader1);

  unsigned int vertexShader2;
  vertexShader2 = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader2, 1, &VS2, NULL);
  glCompileShader(vertexShader2);

  unsigned int fragmentShader2;
  fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader2, 1, &FS2, NULL);
  glCompileShader(fragmentShader2);
  
  unsigned int tessControlShader;
  tessControlShader = glCreateShader(GL_TESS_CONTROL_SHADER);
  glShaderSource(tessControlShader, 1, &TCS, NULL);
  glCompileShader(tessControlShader);
  
  unsigned int tessEvaluationShader;
  tessEvaluationShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
  glShaderSource(tessEvaluationShader, 1, &TES, NULL);
  glCompileShader(tessEvaluationShader);
  
  unsigned int shaderProgram1;
  shaderProgram1 = glCreateProgram();
  glAttachShader(shaderProgram1, vertexShader1);
  glAttachShader(shaderProgram1, fragmentShader1);
  glAttachShader(shaderProgram1, tessControlShader);
  glAttachShader(shaderProgram1, tessEvaluationShader);
  glLinkProgram(shaderProgram1);
  
  unsigned int shaderProgram2;
  shaderProgram2 = glCreateProgram();
  glAttachShader(shaderProgram2, vertexShader2);
  glAttachShader(shaderProgram2, fragmentShader2);
  glLinkProgram(shaderProgram2);

  glPatchParameteri(GL_PATCH_VERTICES, 4);
  //stbi_set_flip_vertically_on_load(true);
  unsigned int texture;
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  int width, height, nChannels;
  unsigned char *data = stbi_load("src/iceland_heightmap.png", &width, &height, &nChannels, 0);
  if (data)
  {
    GLenum format = (nChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format,
                 width, height, 0,
                 format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  else
  {
    std::cout << "Failed to load heightmap\n";
  }

  std::vector<float> vertices;

  unsigned rez = 20;
  for(unsigned i = 0; i <= rez-1; i++)
  {
	  for(unsigned j = 0; j <= rez-1; j++)
	  {
		  vertices.push_back(-width/2.0f + width*i/(float)rez); // v.x
		  vertices.push_back(0.0f); // v.y
		  vertices.push_back(-height/2.0f + height*j/(float)rez); // v.z
		  vertices.push_back(i / (float)rez); // u
		  vertices.push_back(j / (float)rez); // v

		  vertices.push_back(-width/2.0f + width*(i+1)/(float)rez); // v.x
		  vertices.push_back(0.0f); // v.y
		  vertices.push_back(-height/2.0f + height*j/(float)rez); // v.z
		  vertices.push_back((i+1) / (float)rez); // u
		  vertices.push_back(j / (float)rez); // v

		  vertices.push_back(-width/2.0f + width*i/(float)rez); // v.x
		  vertices.push_back(0.0f); // v.y
		  vertices.push_back(-height/2.0f + height*(j+1)/(float)rez); // v.z
		  vertices.push_back(i / (float)rez); // u
		  vertices.push_back((j+1) / (float)rez); // v

		  vertices.push_back(-width/2.0f + width*(i+1)/(float)rez); // v.x
		  vertices.push_back(0.0f); // v.y
		  vertices.push_back(-height/2.0f + height*(j+1)/(float)rez); // v.z
		  vertices.push_back((i+1) / (float)rez); // u
		  vertices.push_back((j+1) / (float)rez); // v
	  }
  }
  stbi_image_free(data);
  std::cout << "Loaded " << rez*rez << " patches of 4 control points each" << std::endl;
  std::cout << "Processing " << rez*rez*4 << " vertices in vertex shader" << std::endl;

  
  std::vector<std::string> faces =
  {
    "skybox/right.jpg",
    "skybox/left.jpg",
    "skybox/top.jpg",
    "skybox/bottom.jpg",
    "skybox/front.jpg",
    "skybox/back.jpg"
  };
  unsigned int cubemapTexture = loadCubemap(faces);  
  std::vector<float> skyboxVertices = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
  };
  unsigned int VBO[2], VAO[2];
  glGenBuffers(2, VBO);
  glGenVertexArrays(2, VAO);
  glBindVertexArray(VAO[0]);
  glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),(void*) (3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(VAO[1]);
  glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
  glBufferData(GL_ARRAY_BUFFER, skyboxVertices.size() * sizeof(float), &skyboxVertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);
  
  while(!glfwWindowShouldClose(w)){
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    processInput(w);
    
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glClearColor(0.70, 0.81, 1.0, 1);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glUseProgram(shaderProgram1);
    
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 5000.0f);
    view = glm::lookAt(
      cameraPos,
      cameraPos + cameraFront,
      cameraUp
    );
    glm::mat4 model = glm::mat4(1.0f);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glUniform1i(glGetUniformLocation(shaderProgram1, "skybox"), 1);

    glUniform3fv(
      glGetUniformLocation(shaderProgram1, "cameraPos"),
      1,
      glm::value_ptr(cameraPos)
    );

    glUniform1i(glGetUniformLocation(shaderProgram1, "heightMap"), 0);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(VAO[0]);
    glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS * rez * rez);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glUseProgram(shaderProgram2);

// REMOVE translation from view
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

    glUniformMatrix4fv(
    glGetUniformLocation(shaderProgram2, "view"),
    1, GL_FALSE, glm::value_ptr(skyboxView)
    );
    glUniformMatrix4fv(
    glGetUniformLocation(shaderProgram2, "projection"),
    1, GL_FALSE, glm::value_ptr(projection)
    );
    glUniform1i(glGetUniformLocation(shaderProgram2, "skybox"), 0);

    glBindVertexArray(VAO[1]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glfwSwapBuffers(w);
    glfwPollEvents();
  }
  glDeleteVertexArrays(1, VAO);
  glDeleteBuffers(1, VBO);
  glDeleteProgram(shaderProgram1);

  glfwTerminate();
  return 0;
}
