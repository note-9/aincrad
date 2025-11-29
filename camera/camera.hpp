#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

enum Camera_Movement {
    CAMERA_FORWARD,
    CAMERA_BACKWARD,
    CAMERA_LEFT,
    CAMERA_RIGHT
};

struct Camera {
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
};

// Initialization
Camera camera_create(glm::vec3 position, glm::vec3 up, float yaw, float pitch);

// Matrix
glm::mat4 camera_get_view_matrix(Camera* cam);

// Movement + Input
void camera_process_keyboard(Camera* cam, Camera_Movement direction, float deltaTime);
void camera_process_mouse(Camera* cam, float xoffset, float yoffset, bool constrainPitch = true);
void camera_process_scroll(Camera* cam, float yoffset);

// Internal update vector function
void camera_update_vectors(Camera* cam);

#endif

