#pragma once

#include "utils.hpp"
#include <chrono>
#include <glm/glm.hpp>
#include <vector>

struct GLFWwindow;

class Application {
public:
  Application(const char *name, int width, int height);
  virtual ~Application();

  void run();
  float average_frame_time();
  void request_screen_shot();

protected:
  virtual void init() {}
  virtual void update() {}
  virtual void key_callback(int key, int scancode, int action, int mods) {}
  virtual void character_callback(unsigned int codepoint) {}
  virtual void cursor_position_callback(double xpos, double ypos) {}
  virtual void scroll_callback(double xoffset, double yoffset) {}
  virtual void mouse_button_callback(int button, int action, int mods) {}
  virtual void cursor_enter_callback(bool entered) {}

  GLFWwindow *_window{};

private:
  GLFWwindow *create_window(const char *name, int width, int height);

  static void window_key_callback(
      GLFWwindow *window, int key, int scancode, int action, int mods);

  static void window_character_callback(GLFWwindow *window,
                                        unsigned int codepoint);

  static void
  window_cursor_position_callback(GLFWwindow *window, double xpos, double ypos);

  static void
  window_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

  static void window_mouse_button_callback(GLFWwindow *window,
                                           int button,
                                           int action,
                                           int mods);

  static void window_cursor_enter_callback(GLFWwindow *window, int entered);

  void screen_shot();

  bool _need_screen_shot = false;
  using Clock = std::chrono::high_resolution_clock;
  using TimeSample = Clock::time_point;
  FixSizeQueue<TimeSample> _frame_time_samples;
};

class ModelViewerCamera {
public:
  void draw_ui();

  glm::mat4 view() const;
  glm::mat4 projection(float aspect) const;
  glm::vec3 position() const;

private:
  float _focus_height = 0.25f;
  float _field_of_view = glm::radians(25.0f);
  float _pitch = glm::radians(60.0f);
  float _yaw = glm::radians(60.0f);
  float _distance = 3.0f;
};
