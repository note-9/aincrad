#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

static const float CAMERA_YAW = -90.0f;
static const float CAMERA_PITCH = 0.0f;
static const float CAMERA_SPEED = 2.5f;
static const float CAMERA_SENSITIVITY = 0.1f;
static const float CAMERA_ZOOM = 45.0f;

Camera camera_create(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
{
    Camera cam = {};

    cam.Position = position;
    cam.WorldUp = up;

    cam.Yaw = yaw;
    cam.Pitch = pitch;

    cam.Front = glm::vec3(0.0f, 0.0f, -1.0f);

    cam.MovementSpeed = CAMERA_SPEED;
    cam.MouseSensitivity = CAMERA_SENSITIVITY;
    cam.Zoom = CAMERA_ZOOM;

    camera_update_vectors(&cam);

    return cam;
}

glm::mat4 camera_get_view_matrix(Camera* cam)
{
    return glm::lookAt(cam->Position, cam->Position + cam->Front, cam->Up);
}

void camera_process_keyboard(Camera* cam, Camera_Movement direction, float deltaTime)
{
    float velocity = cam->MovementSpeed * deltaTime;

    if (direction == CAMERA_FORWARD)
        cam->Position += cam->Front * velocity;
    if (direction == CAMERA_BACKWARD)
        cam->Position -= cam->Front * velocity;
    if (direction == CAMERA_LEFT)
        cam->Position -= cam->Right * velocity;
    if (direction == CAMERA_RIGHT)
        cam->Position += cam->Right * velocity;
}

void camera_process_mouse(Camera* cam, float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= cam->MouseSensitivity;
    yoffset *= cam->MouseSensitivity;

    cam->Yaw += xoffset;
    cam->Pitch += yoffset;

    if (constrainPitch) {
        if (cam->Pitch > 89.0f) cam->Pitch = 89.0f;
        if (cam->Pitch < -89.0f) cam->Pitch = -89.0f;
    }

    camera_update_vectors(cam);
}

void camera_process_scroll(Camera* cam, float yoffset)
{
    cam->Zoom -= yoffset;
    if (cam->Zoom < 1.0f) cam->Zoom = 1.0f;
    if (cam->Zoom > 45.0f) cam->Zoom = 45.0f;
}

void camera_update_vectors(Camera* cam)
{
    glm::vec3 front;
    front.x = cos(glm::radians(cam->Yaw)) * cos(glm::radians(cam->Pitch));
    front.y = sin(glm::radians(cam->Pitch));
    front.z = sin(glm::radians(cam->Yaw)) * cos(glm::radians(cam->Pitch));

    cam->Front = glm::normalize(front);
    cam->Right = glm::normalize(glm::cross(cam->Front, cam->WorldUp));
    cam->Up = glm::normalize(glm::cross(cam->Right, cam->Front));
}

