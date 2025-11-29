#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "../camera/camera.hpp"
#include "../shaders/shader.hpp"
#include "../textures/texture.hpp"
#include "../renderer/vertex_array.hpp"
#include "../renderer/vertex_buffer.hpp"
#include "../renderer/element_buffer.hpp"

const unsigned int SCR_WIDTH  = 1600;
const unsigned int SCR_HEIGHT = 1000;

// ----------------------------
// Global Camera State
// ----------------------------
Camera camera = camera_create(
    glm::vec3(0.0f, 0.0f, 3.0f),   // Position
    glm::vec3(0.0f, 1.0f, 0.0f),   // Up
    -90.0f,                        // Yaw
    0.0f                           // Pitch
);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ----------------------------
// Function Declarations
// ----------------------------
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// ======================================================
//                       MAIN
// ======================================================
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Aincrad", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to load GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // ---------------------------
    // Shader
    // ---------------------------
    Shader shader = shader_from_files("shaders/default.vert",
                                      "shaders/default.frag");

    // ---------------------------
    // Geometry
    // ---------------------------
    float vertices[] = 
    { 
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 
        
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 
        
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 
        -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 
        -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 
        
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 
        0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 
         0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 
         
         -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
          0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 
          0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 
          0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 
          -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 
          -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 
          
          -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 
          0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 
          0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 
          0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 
          -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 
          -0.5f, 0.5f, -0.5f, 0.0f, 1.0f 
    };

    VertexArray vao;
    vao.init();
    vao.bind();

    VertexBuffer vbo;

    vbo.init(vertices, sizeof(vertices));
    vao.enableAttrib(0, 3, GL_FLOAT, false, 5 * sizeof(float), (void*)0);
    vao.enableAttrib(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    vbo.unbind();
    vao.unbind();


    // ---------------------------
    // Textures
    // ---------------------------
    Texture tex1 = texture_load("src/wall.jpg", GL_TEXTURE0);
    Texture tex2 = texture_load("src/awesomeface.png", 1);
    shader_use(&shader);
    shader_set_int(&shader, "texture1", 0);
    glm::vec3 cubePositions[] = 
    { 
        glm::vec3( 0.0f, 0.0f, 0.0f), 
        glm::vec3( 2.0f, 5.0f, -15.0f), 
        glm::vec3(-1.5f, -2.2f, -2.5f), 
        glm::vec3(-3.8f, -2.0f, -12.3f), 
        glm::vec3( 2.4f, -0.4f, -3.5f), 
        glm::vec3(-1.7f, 3.0f, -7.5f), 
        glm::vec3( 1.3f, -2.0f, -2.5f), 
        glm::vec3( 1.5f, 2.0f, -2.5f), 
        glm::vec3( 1.5f, 0.2f, -1.5f), 
        glm::vec3(-1.3f, 1.0f, -1.5f) 
    };
    float texCoords[] = { 
        0.0f, 0.0f, // lower-left corner 
        1.0f, 0.0f, // lower-right corner 
        0.5f, 1.0f // top-center corner 
    };
    // ======================================================
    //                    RENDER LOOP
    // ======================================================
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader_use(&shader);

        // matrices
        glm::mat4 model = glm::mat4(1.0f); model = glm::rotate(model, (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float)SCR_WIDTH / SCR_HEIGHT,
                                                0.1f, 100.0f);

        glm::mat4 view = camera_get_view_matrix(&camera);
        shader_set_mat4(&shader, "projection", projection);
        shader_set_mat4(&shader, "view", view);
        shader_set_mat4(&shader, "model", model);

        texture_bind(&tex1, 0);
        texture_bind(&tex2, 1);
        glUniform1i(glGetUniformLocation(shader.id, "texture1"), 0);
        glUniform1i(glGetUniformLocation(shader.id, "texture2"), 1);
        vao.bind();
        for(unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]); float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            shader_set_mat4(&shader, "model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    vao.destroy();
    vbo.destroy();
    texture_destroy(&tex1);

    glfwTerminate();
    return 0;
}

// ======================================================
//               INPUT + CALLBACKS
// ======================================================
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera_process_keyboard(&camera, CAMERA_FORWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera_process_keyboard(&camera, CAMERA_BACKWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera_process_keyboard(&camera, CAMERA_LEFT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera_process_keyboard(&camera, CAMERA_RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera_process_mouse(&camera, xoffset, yoffset, true);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera_process_scroll(&camera, yoffset);
}
