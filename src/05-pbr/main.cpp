#include "../common/application.hpp"
#include "../common/framebuffer.hpp"
#include "../common/gltf.hpp"
#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/utils.hpp"
#include "material.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <sstream>
#include <tiny_gltf.h>
#include <vector>

class PbrApp final : public Application {
public:
  PbrApp() : Application("PBR", 800, 600) {}

private:
  void init() override {
    _camera = std::make_unique<ModelViewerCamera>();
    _scene = std::make_unique<Gltf>("FlightHelmet/FlightHelmet.gltf");
    _tone_mapping_material = std::make_unique<ToneMappingMaterial>();
    _renderer = std::make_unique<Renderer>();

    auto init_mat = [&](PbrMaterial *pbr_mat, Gltf::Material *mat) {
#define ASSIGN_TEXTURE(name)                                                   \
  pbr_mat->name = mat->name < 0 ? nullptr : _scene->textures[mat->name].get()
#define ASSIGN_FIELD(name) pbr_mat->name = mat->name
      ASSIGN_TEXTURE(base_color);
      ASSIGN_FIELD(base_color_factor);
      ASSIGN_FIELD(double_sided);
      ASSIGN_FIELD(metallic_factor);
      ASSIGN_FIELD(roughness_factor);
      ASSIGN_TEXTURE(metallic_roughness);
      ASSIGN_TEXTURE(normal);
      ASSIGN_FIELD(normal_scale);
      ASSIGN_TEXTURE(occlusion);
      ASSIGN_FIELD(occlusion_strength);
      ASSIGN_TEXTURE(emission);
      ASSIGN_FIELD(emission_factor);

#undef ASSIGN_TEXTURE
#undef ASSIGN_FIELD

      switch (mat->mode) {
      case Gltf::Material::Opaque:
        pbr_mat->mode = PbrMaterial::Opaque;
        break;
      case Gltf::Material::Blend:
        pbr_mat->mode = PbrMaterial::Blend;
        break;
      }
    };

    for (auto &mat : _scene->materials) {
      auto pbr_mat = std::make_unique<PbrMaterial>(false);
      init_mat(pbr_mat.get(), mat.get());
      auto base_color_mat = std::make_unique<PbrMaterial>(true);
      init_mat(base_color_mat.get(), mat.get());

      _pbr_materials.emplace_back(std::move(pbr_mat));
      _base_color_materials.emplace_back(std::move(base_color_mat));
    }
  }

  void draw_ui() {
    int id = 0;
    {
      std::stringstream ss;
      float frame_time = average_frame_time();
      ss << "FPS: ";
      if (frame_time == 0.0f) {
        ss << "NAN";
      } else {
        ss << 1.0f / frame_time;
      }
      ss << "(" << frame_time * 1000.0f << "ms)";
      ImGui::Text("%s", ss.str().c_str());
    }
    if (ImGui::Button("Screen Shot")) {
      request_screen_shot();
    }
    if (ImGui::CollapsingHeader("Camera")) {
      ImGui::PushID(id++);
      _camera->draw_ui();
      ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Tone Mapping")) {
      ImGui::PushID(id++);
      ImGui::SliderFloat(
          "Exposure", &_tone_mapping_material->exposure, 0.0f, 10.0f);
      ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Directional Light")) {
      ImGui::PushID(id++);
      ImGui::SliderAngle("Pitch", &_light_pitch, 0.0f, 180.0f);
      ImGui::SliderAngle("Yaw", &_light_yaw);
      ImGui::ColorEdit3("Color", (float *)&_light_color);
      ImGui::SliderFloat("Strength", &_light_strength, 0.0f, 10.0f);
      ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Environment Light")) {
      ImGui::PushID(id++);
      ImGui::ColorEdit3("Color", (float *)&_env_color);
      ImGui::SliderFloat("Strength", &_env_strength, 0.0f, 10.0f);
      ImGui::PopID();
    }
  }

  void draw_scene() {
    glm::vec3 env_irradiance = _env_color * _env_strength;
    glClearColor(env_irradiance.x, env_irradiance.y, env_irradiance.z, 1.0);
    glViewport(0, 0, _screen_fb_width, _screen_fb_height);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)_screen_fb_width / (float)_screen_fb_height;

    glm::mat4 view = _camera->view();
    glm::mat4 projection = _camera->projection(aspect);

    auto draw_mode =
        [&](PbrMaterial::Mode mode,
            const std::vector<std::unique_ptr<PbrMaterial>> &materials) {
          for (auto &draw : _scene->draws) {
            for (auto &prim : _scene->meshes[draw.index]) {
              auto *mat = materials[prim.material].get();
              if (mat->mode != mode) {
                continue;
              }
              mat->model = draw.transform;
              mat->view = view;
              mat->projection = projection;

              glm::vec3 light_dir_ws =
                  polar_to_cartesian(_light_yaw, _light_pitch);
              glm::vec3 light_dir_vs = view * glm::vec4(light_dir_ws, 0.0f);

              mat->light_dir_vs = glm::normalize(light_dir_vs);
              mat->light_radiance = _light_color * _light_strength;
              mat->env_irradiance = env_irradiance;

              mat->use();
              prim.mesh->draw();
            }
          }
        };

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    draw_mode(PbrMaterial::Opaque, _pbr_materials);
    glEnable(GL_BLEND);
    // disable z-write for transparent objects
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    // tint objects covered by transparent ones
    draw_mode(PbrMaterial::Blend, _base_color_materials);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    draw_mode(PbrMaterial::Blend, _pbr_materials);
  }

  void draw() {
    // render scene to texture
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer->get());
    draw_scene();

    // blit texture to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    _renderer->bilt(_color_attachment.get(), _tone_mapping_material.get());
  }

  void update_frame_buffer() {
    glfwGetFramebufferSize(_window, &_screen_fb_width, &_screen_fb_height);
    // make the size of offscreen buffer matches the screen's
    if (_color_attachment != nullptr &&
        _color_attachment->width() == _screen_fb_width &&
        _color_attachment->height() == _screen_fb_height) {
      return;
    }
    // use HDR
    _color_attachment = std::make_unique<Texture2D>(nullptr,
                                                    GL_FLOAT,
                                                    _screen_fb_width,
                                                    _screen_fb_height,
                                                    GL_RGBA16F,
                                                    GL_RGBA);
    _depth_stencil_attachment =
        std::make_unique<Texture2D>(nullptr,
                                    GL_UNSIGNED_INT_24_8,
                                    _screen_fb_width,
                                    _screen_fb_height,
                                    GL_DEPTH24_STENCIL8,
                                    GL_DEPTH_STENCIL);
    Texture2D *color_attachments[] = {_color_attachment.get()};
    _framebuffer =
        std::make_unique<Framebuffer>(color_attachments,
                                      std::size(color_attachments),
                                      _depth_stencil_attachment.get());
  }

  void update() override {
    update_frame_buffer();
    draw_ui();
    draw();
  }

  float _light_yaw = glm::radians(60.0f);
  float _light_pitch = glm::radians(60.0f);
  float _light_strength = 1.0f;
  glm::vec3 _light_color = glm::vec3(1.0, 1.0, 1.0);
  float _env_strength = 1.0f;
  glm::vec3 _env_color = glm::vec3(1.0, 1.0, 1.0);

  std::vector<std::unique_ptr<PbrMaterial>> _pbr_materials;
  std::vector<std::unique_ptr<PbrMaterial>> _base_color_materials;
  std::unique_ptr<ToneMappingMaterial> _tone_mapping_material{};

  int _screen_fb_width, _screen_fb_height;
  std::unique_ptr<Texture2D> _color_attachment{};
  std::unique_ptr<Texture2D> _depth_stencil_attachment{};
  std::unique_ptr<Framebuffer> _framebuffer{};

  std::unique_ptr<Renderer> _renderer;
  std::unique_ptr<ModelViewerCamera> _camera;
  std::unique_ptr<Gltf> _scene;
};

int main() {
  try {
    PbrApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}