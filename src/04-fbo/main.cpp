#include "../common/application.hpp"
#include "../common/framebuffer.hpp"
#include "../common/gltf.hpp"
#include "../common/shader.hpp"
#include "material.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <tiny_gltf.h>
#include <vector>

class FboApp final : public Application {
public:
  FboApp() : Application("FBO", 800, 600) {}

private:
  void init() override {
    _camera = std::make_unique<ModelViewerCamera>();
    _scene = std::make_unique<Gltf>("FlightHelmet/FlightHelmet.gltf");

    // draw a big triangle that cover the full screen
    std::vector<Mesh::Vertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, {}, {}, {0.0f, 0.0f}}, // left-down
        {{3.0f, -1.0f, 0.0f}, {}, {}, {2.0f, 0.0f}},  // right-down
        {{-1.0f, 3.0f, 0.0f}, {}, {}, {0.0f, 2.0f}},  // left-up
    };

    _full_screen_triangle =
        std::make_unique<Mesh>(vertices.data(), vertices.size(), nullptr, 0);
    _base_color_material = std::make_unique<BaseColorMaterial>();
    _chromatic_aberration_material =
        std::make_unique<ChromaticAberrationMaterial>();
  }

  void draw_ui() {
    ImGui::Text("Camera");
    _camera->draw_ui();
    ImGui::Text("Chromatic Aberration");
    ImGui::SliderFloat(
        "Strength", &_chromatic_aberration_material->strength, 0.0f, 0.1f);
  }

  void draw_scene() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glViewport(0, 0, _screen_fb_width, _screen_fb_height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)_screen_fb_width / (float)_screen_fb_height;

    glm::mat4 view = _camera->view();
    glm::mat4 projection = _camera->projection(aspect);

    for (auto &draw : _scene->draws) {
      auto transform = projection * view * draw.transform;
      _base_color_material->transform = transform;
      for (auto &prim : _scene->meshes[draw.index]) {
        auto *mat = _scene->materials[prim.material].get();
        _base_color_material->base_tex =
            _scene->textures[mat->base_color].get();
        _base_color_material->use();
        prim.mesh->draw();
      }
    }
  }

  void draw() {
    // render scene to texture
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer->get());
    draw_scene();

    // blit texture to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    _chromatic_aberration_material->transform = glm::identity<glm::mat4>();
    _chromatic_aberration_material->base_tex = _color_attachment.get();
    _chromatic_aberration_material->use();
    _full_screen_triangle->draw();
  }

  void update_frame_buffer() {
    glfwGetFramebufferSize(_window, &_screen_fb_width, &_screen_fb_height);
    // make the size of offscreen buffer matches the screen's
    if (_color_attachment != nullptr &&
        _color_attachment->width() == _screen_fb_width &&
        _color_attachment->height() == _screen_fb_height) {
      return;
    }
    _color_attachment = std::make_unique<Texture2D>(nullptr,
                                                    _screen_fb_width,
                                                    _screen_fb_height,
                                                    GL_RGBA,
                                                    GL_RGBA,
                                                    GL_UNSIGNED_BYTE);
    _depth_stencil_attachment =
        std::make_unique<Texture2D>(nullptr,
                                    _screen_fb_width,
                                    _screen_fb_height,
                                    GL_DEPTH24_STENCIL8,
                                    GL_DEPTH_STENCIL,
                                    GL_UNSIGNED_INT_24_8);
    Texture2D *color_attachments[] = {_color_attachment.get()};
    _framebuffer = std::make_unique<Framebuffer>(
        color_attachments, 1, _depth_stencil_attachment.get());
  }

  void update() override {
    update_frame_buffer();
    draw_ui();
    draw();
  }

  std::unique_ptr<BaseColorMaterial> _base_color_material{};
  std::unique_ptr<ChromaticAberrationMaterial> _chromatic_aberration_material{};

  int _screen_fb_width, _screen_fb_height;
  std::unique_ptr<Texture2D> _color_attachment{};
  std::unique_ptr<Texture2D> _depth_stencil_attachment{};
  std::unique_ptr<Framebuffer> _framebuffer{};

  std::unique_ptr<Mesh> _full_screen_triangle;
  std::unique_ptr<ModelViewerCamera> _camera;
  std::unique_ptr<Gltf> _scene;
};

int main() {
  try {
    FboApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}