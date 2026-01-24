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

const char* VS = R"(
#version 450 core
layout (location=0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out float Height;
out vec3 Position;

void main(){
  Height = aPos.y;
  Position = (model * vec4(aPos, 1.0)).xyz;
  gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
)";

const char* FS1 = R"(
#version 450 core

out vec4 FragColor;

in float Height;

void main(){
  float h = (Height + 16)/32.0f;
  FragColor = vec4(h, h, h, 1.0);
}
)";
const char* FS2 = R"(
#version 450 core
out vec4 FragColor;

void main(){
  FragColor = vec4(1.0, 1.0, 0.0, 1.0);
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

    // Prevent screen flip
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

  glEnable(GL_DEPTH_TEST);
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &VS, NULL);
  glCompileShader(vertexShader);
  // After glCompileShader(vertexShader)

  unsigned int fragmentShader1;
  fragmentShader1 = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader1, 1, &FS1, NULL);
  glCompileShader(fragmentShader1);
  unsigned int fragmentShader2;
  fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader2, 1, &FS2, NULL);
  glCompileShader(fragmentShader2);

  unsigned int shaderProgram1;
  shaderProgram1 = glCreateProgram();
  glAttachShader(shaderProgram1, vertexShader);
  glAttachShader(shaderProgram1, fragmentShader1);
  glLinkProgram(shaderProgram1);
  //unsigned int shaderProgram2;
  //shaderProgram2 = glCreateProgram();
  //glAttachShader(shaderProgram2, vertexShader);
  //glAttachShader(shaderProgram2, fragmentShader2);
  //glLinkProgram(shaderProgram2);


  stbi_set_flip_vertically_on_load(true);
  int width, height, nChannels;
  unsigned char *data = stbi_load("src/iceland_heightmap.png", &width, &height, &nChannels, 0);
  
  std::vector<float> vertices;
  float yScale = 64.0f / 256.0f, yShift = 16.0f;  // apply a scale+shift to the height data
  for(unsigned int i = 0; i < height; i++)
  {
    for(unsigned int j = 0; j < width; j++)
    {
        // retrieve texel for (i,j) tex coord
      unsigned char* texel = data + (j + width * i) * nChannels;
        // raw height at coordinate
      unsigned char y = texel[0];

        // vertex
      vertices.push_back( -height/2.0f + height*i/(float)height );        // v.x
      vertices.push_back( (int)y * yScale - yShift); // v.y
      vertices.push_back( -width/2.0f + width*j/(float)width );        // v.z
    }
  }
  std::cout << "Loaded " << vertices.size() / 3 << " vertices" << std::endl;

  stbi_image_free(data);
  
  std::vector<unsigned int> indices;
  for(unsigned int i = 0; i < height-1; i++)       // for each row a.k.a. each strip
  {
    for(unsigned int j = 0; j < width; j++)      // for each column
    {
      for(unsigned int k = 0; k < 2; k++)      // for each side of the strip
      {
        indices.push_back(j + width * (i + k));
      }
    }
  }
  std::cout << "Loaded " << indices.size() << " indices" << std::endl;

  
  /*float cube[] = {
    -0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    
    +0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, +0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    
    -0.5f, +0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, +0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, +0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,

    -0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, +0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    
    -0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, +0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, +0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    -0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, +0.5f, +0.5f, 1.0f, 0.0f, 0.0f,
    +0.5f, +0.5f, -0.5f, 1.0f, 0.0f, 0.0f
  };
  glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
  };*/
  const unsigned int NUM_STRIPS = height-1;
  const unsigned int NUM_VERTS_PER_STRIP = width*2;

  unsigned int VBO[1], VAO[1], EBO[1];
  glGenBuffers(1, VBO);
  glGenVertexArrays(1, VAO);
  glBindVertexArray(VAO[0]);
  glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glEnableVertexAttribArray(0);
  
  glGenBuffers(1, EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);


  while(!glfwWindowShouldClose(w)){
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    processInput(w);
    
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glClearColor(0.70, 0.81, 1.0, 1);
    glUseProgram(shaderProgram1);
    
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100000.0f);
    view = glm::lookAt(
      cameraPos,
      cameraPos + cameraFront,
      cameraUp
    );
    glm::mat4 model = glm::mat4(1.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


    glBindVertexArray(VAO[0]);
    /*for (unsigned int i = 0; i < 10; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 20.0f * i;
      model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }*/
   // glUseProgram(shaderProgram2);
    //glBindVertexArray(VAO[1]);
    //glDrawArrays(GL_TRIANGLES, 0, 3);
    for (unsigned int strip = 0; strip < NUM_STRIPS; strip++) {
      glDrawElements(GL_TRIANGLE_STRIP, NUM_VERTS_PER_STRIP, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * NUM_VERTS_PER_STRIP * strip));
    }
    glfwSwapBuffers(w);
    glfwPollEvents();
  }
  glDeleteVertexArrays(1, VAO);
  glDeleteBuffers(1, VBO);
  glDeleteBuffers(1, EBO);
  glDeleteProgram(shaderProgram1);
  //glDeleteProgram(shaderProgram2);

  glfwTerminate();
  return 0;
}
