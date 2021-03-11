#include "application.hpp"
#include "data.hpp"
#include "utils.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ctime>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <iostream>
#include <sstream>
#include <stb_image_write.h>
#include <stdexcept>

GLFWwindow *
Application::create_window(const char *name, int width, int height) {
  if (glfwInit() == GLFW_FALSE) {
    throw std::runtime_error("failed to init glfw");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__ // for macos
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  GLFWwindow *window = glfwCreateWindow(width, height, name, nullptr, nullptr);
  if (!window) {
    throw std::runtime_error("failed to create window");
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetKeyCallback(window, window_key_callback);
  glfwSetCharCallback(window, window_character_callback);
  glfwSetCursorPosCallback(window, window_cursor_position_callback);
  glfwSetCursorEnterCallback(window, window_cursor_enter_callback);
  glfwSetMouseButtonCallback(window, window_mouse_button_callback);

  glfwMakeContextCurrent(window);

  GLenum e = glewInit();
  if (e != GLEW_OK) {
    glfwDestroyWindow(window);
    std::stringstream ss;
    ss << glewGetErrorString(e);
    throw std::runtime_error("failed to init glew: " + ss.str());
  }

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 150");

  return window;
}

Application::Application(const char *name, int width, int height)
    : _frame_time_samples(30) {
  _window = create_window(name, width, height);
}

Application::~Application() {
  ImGui::DestroyContext();
  glfwDestroyWindow(_window);
  glfwTerminate();
}

float Application::average_frame_time() {
  if (_frame_time_samples.size() <= 1) {
    return 0.0f;
  }
  using Duration = std::chrono::duration<float>;
  auto delta = std::chrono::duration_cast<Duration>(
      _frame_time_samples.back() - _frame_time_samples.front());
  return delta.count() / (float)(_frame_time_samples.size() - 1);
}

void Application::run() {
  init();

  while (!glfwWindowShouldClose(_window)) {
    _frame_time_samples.push(Clock::now());
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    update();
    if (_need_screen_shot) {
      screen_shot();
      _need_screen_shot = false;
    }

    ImGui::Render();
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    glViewport(0, 0, width, height);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(_window);
  }
}

void Application::window_key_callback(
    GLFWwindow *window, int key, int scancode, int action, int mods) {
  auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  app->key_callback(key, scancode, action, mods);
}

void Application::window_character_callback(GLFWwindow *window,
                                            unsigned int codepoint) {
  auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  app->character_callback(codepoint);
}

void Application::window_cursor_position_callback(GLFWwindow *window,
                                                  double xpos,
                                                  double ypos) {

  auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  app->cursor_position_callback(xpos, ypos);
}

void Application::window_scroll_callback(GLFWwindow *window,
                                         double xoffset,
                                         double yoffset) {
  auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  app->scroll_callback(xoffset, yoffset);
}

void Application::window_mouse_button_callback(GLFWwindow *window,
                                               int button,
                                               int action,
                                               int mods) {
  auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  app->mouse_button_callback(button, action, mods);
}

void Application::window_cursor_enter_callback(GLFWwindow *window,
                                               int entered) {
  auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
  app->cursor_enter_callback(entered);
}

static const std::string current_data_time() {
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d-%X", &tstruct);

  return buf;
}

static const std::string get_screen_shot_filename() {
  auto t = current_data_time();
  for (auto &c : t) {
    if (c == ':') {
      c = '-';
    }
  }
  return "ogl-screen-shot-" + t + ".png";
}

void Application::request_screen_shot() {
  _need_screen_shot = true;
}

void Application::screen_shot() {
  int width, height;
  glfwGetFramebufferSize(_window, &width, &height);
  std::vector<uint8_t> data;
  glFlush();
  glReadBuffer(GL_BACK);
  data.resize(width * height * 4);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

  auto output_path = fs::absolute(get_screen_shot_filename()).string();

  stbi_flip_vertically_on_write(true);
  stbi_write_png(output_path.c_str(), width, height, 4, data.data(), 0);
  std::cout << "screen shot written to " << output_path << std::endl;
}

void ModelViewerCamera::draw_ui() {
  ImGui::SliderAngle("Field Of View", &_field_of_view, 0.1f, 180.0f);
  ImGui::SliderAngle("Pitch", &_pitch, 10.0f, 170.0f);
  ImGui::SliderAngle("Yaw", &_yaw);
  ImGui::SliderFloat("Focus Height", &_focus_height, -3.0f, 3.0f);
  ImGui::SliderFloat("Distance", &_distance, 0.5f, 5.0f);
}

glm::vec3 ModelViewerCamera::position() const {
  return _distance * polar_to_cartesian(_yaw, _pitch) +
         glm::vec3(0.0f, _focus_height, 0.0f);
}

glm::mat4 ModelViewerCamera::projection(float aspect) const {
  return glm::perspective(_field_of_view, aspect, 0.01f, 100.0f);
}

glm::mat4 ModelViewerCamera::view() const {
  glm::vec3 pos = position();
  glm::mat4 view =
      glm::lookAt(pos, glm::vec3(0, _focus_height, 0), glm::vec3(0, 1, 0));
  return view;
}
