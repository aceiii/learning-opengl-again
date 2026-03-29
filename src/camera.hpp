#pragma once

#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


enum class CameraMovement {
  kMoveForward,
  kMoveBackward,
  kMoveLeft,
  kMoveRight,
};

class Camera {
public:
  inline static const float kDefaultYaw = -90.0f;
  inline static const float kDefaultPitch = 0.0f;
  inline static const float kDefaultSpeed = 2.5f;
  inline static const float kDefaultSensitivity = 0.1f;
  inline static const float kDefaultFov = 45.0f;
  inline static const float kMinFov = 1.0f;
  inline static const float kMaxFov = 90.0f;
  inline static const float kMinPitch = -89.0f;
  inline static const float kMaxPitch = 89.0f;
  inline static const float kMinYaw = -360.0f;
  inline static const float kMaxYaw = 360.0f;
  inline static const float kMinSpeed = 0.01f;
  inline static const float kMaxSpeed = 50.0f;

  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::vec3 right;
  glm::vec3 world_up;

  float fov;
  float yaw;
  float pitch;
  float movement_speed;
  float mouse_sensitivity;

  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = kDefaultYaw, float pitch = kDefaultPitch)
    : front(0.0f, 0.0f, -1.0f), movement_speed(kDefaultSpeed), mouse_sensitivity(kDefaultSensitivity),
      fov(kDefaultFov), position(position), world_up(up), yaw(yaw), pitch(pitch)
  {
    UpdateCameraVectors();
  }

  Camera(float pos_x, float pos_y, float pos_z, float up_x, float up_y, float up_z, float yaw, float pitch)
    : front(0.0f, 0.0f, -1.0f), movement_speed(kDefaultSpeed), mouse_sensitivity(kDefaultSensitivity),
      fov(kDefaultFov), position(pos_x, pos_y, pos_z), world_up(up_x, up_y, up_z), yaw(yaw), pitch(pitch)
  {
    UpdateCameraVectors();
  }

  glm::mat4 GetViewMatrix() {
    return glm::lookAt(position, position + front, up);
  }

  void ProcessKeyboard(CameraMovement direction, float delta_time) {
    float velocity = movement_speed * delta_time;
    if (direction == CameraMovement::kMoveForward) {
      position += front * velocity;
    }
    else if (direction == CameraMovement::kMoveBackward) {
      position -= front * velocity;
    }
    else if (direction == CameraMovement::kMoveLeft) {
      position -= right * velocity;
    }
    else if (direction == CameraMovement::kMoveRight) {
      position += right * velocity;
    }
  }

  void ProcessMouseMovement(float x_offset, float y_offset, bool constraint_pitch = true) {
    yaw += x_offset * mouse_sensitivity;
    pitch += y_offset * mouse_sensitivity;

    if (constraint_pitch) {
      pitch = std::clamp(pitch, kMinPitch, kMaxPitch);
    }

    UpdateCameraVectors();
  }

  void ProcessMouseScroll(float y_offset) {
    fov = std::clamp(fov - y_offset, kMinFov, kMaxFov);
  }

  void UpdateCameraVectors() {
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
    right = glm::normalize(glm::cross(front, world_up));
    up = glm::normalize(glm::cross(right, front));
  }
};
