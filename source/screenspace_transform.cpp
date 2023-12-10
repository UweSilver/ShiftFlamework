#include "screenspace_transform.hpp"

#include "engine.hpp"
#include "graphics.hpp"
#include "matrix.hpp"
#include "screenspace_mesh_renderer.hpp"
using namespace ShiftFlamework;

void ScreenSpaceTransform::create_gpu_buffer() {
  wgpu::BufferDescriptor buffer_desc{
      .nextInChain = nullptr,
      .label = "constant",
      .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
      .size = Engine::get_module<Graphics>()->get_buffer_stride(
          sizeof(Math::Matrix4x4f)),
      .mappedAtCreation = false,
  };

  constant_buffer = Engine::get_module<Graphics>()->create_buffer(buffer_desc);

  bindgroup =
      Engine::get_module<ScreenSpaceMeshRenderer>()->create_constant_bind_group(
          constant_buffer);
}

void ScreenSpaceTransform::update_gpu_buffer() {
  const auto rotate_mat =
      Math::Matrix4x4f({{{std::cos(angle), -std::sin(angle), 0, 0},
                         {std::sin(angle), std::cos(angle), 0, 0},
                         {0, 0, 1, 0},
                         {0, 0, 0, 1}}});
  const auto translate_mat =
      Math::Matrix4x4f({{{1, 0, 0, position.internal_data.at(0)},
                         {0, 1, 0, position.internal_data.at(1)},
                         {0, 0, 1, 0},
                         {0, 0, 0, 1}}});
  const auto scale_mat =
      Math::Matrix4x4f({{{scale.internal_data.at(0), 0, 0, 0},
                         {0, scale.internal_data.at(1), 0, 0},
                         {0, 0, 1, 0},
                         {0, 0, 0, 1}}});
  const auto matrix = translate_mat * rotate_mat * scale_mat;
  Engine::get_module<Graphics>()->update_buffer(
      constant_buffer, std::vector(1, transposed(matrix)));
}

void ScreenSpaceTransform::on_register() {
  create_gpu_buffer();
  update_gpu_buffer();
}