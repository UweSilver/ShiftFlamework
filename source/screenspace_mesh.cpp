#include "screenspace_mesh.hpp"

#include "engine.hpp"
#include "screenspace_mesh_renderer.hpp"

using namespace ShiftFlamework;

void ScreenSpaceMesh::on_register() {
  vertices = {
      ScreenSpaceVertex{.position = Math::Vector2f({-0.5, -0.5}),
                        .texture_coord = Math::Vector2f({0, 0})},
      ScreenSpaceVertex{.position = Math::Vector2f({+0.5, -0.5}),
                        .texture_coord = Math::Vector2f({1, 0})},
      ScreenSpaceVertex{.position = Math::Vector2f({+0.5, +0.5}),
                        .texture_coord = Math::Vector2f({1, 1})},
      ScreenSpaceVertex{.position = Math::Vector2f({-0.5, +0.5}),
                        .texture_coord = Math::Vector2f({0, 1})},
  };

  indices = {0, 1, 2, 0, 2, 3};
  create_gpu_buffer();
  Engine::get_module<ScreenSpaceMeshRenderer>()->register_mesh(
      entity->get_component<ScreenSpaceMesh>());
}

void ScreenSpaceMesh::create_gpu_buffer() {
  {
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "vertex buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = vertices.size() * sizeof(ScreenSpaceVertex),
        .mappedAtCreation = false};

    vertex_buffer = Engine::get_module<Graphics>()->create_buffer(buffer_desc);
    Engine::get_module<Graphics>()->update_buffer(vertex_buffer, vertices);
  }
  {
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "index buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .size = indices.size() * sizeof(uint32_t),
        .mappedAtCreation = false,
    };
    index_buffer = Engine::get_module<Graphics>()->create_buffer(buffer_desc);
    Engine::get_module<Graphics>()->update_buffer(index_buffer, indices);
  }
}