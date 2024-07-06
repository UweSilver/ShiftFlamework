#include "reflection_renderer.hpp"

#include <stdint.h>

#include <initializer_list>

#include "engine.hpp"
#include "entity.hpp"
#include "graphics.hpp"
#include "matrix.hpp"
#include "mesh.hpp"
#include "transform.hpp"
#include "vector.hpp"

using namespace SF;

void ReflectionRenderer::initialize() {
  std::vector<wgpu::VertexAttribute> vertex_attributes(2);
  // position
  vertex_attributes.at(0).format = wgpu::VertexFormat::Float32x4;
  vertex_attributes.at(0).offset = 0;
  vertex_attributes.at(0).shaderLocation = 0;
  // texcoord
  vertex_attributes.at(1).format = wgpu::VertexFormat::Float32x2;
  vertex_attributes.at(1).offset = 4 * 4;  // 2 * sizeof(Float32)
  vertex_attributes.at(1).shaderLocation = 1;

  wgpu::VertexBufferLayout vertex_buffer_layout{
      .arrayStride = (4 + 2) * 4,  // 4 * sizeof(Float32) + 2 * sizeof(Float32)
      .stepMode = wgpu::VertexStepMode::Vertex,
      .attributeCount = static_cast<uint32_t>(vertex_attributes.size()),
      .attributes = vertex_attributes.data()};

  wgpu::ShaderModuleWGSLDescriptor wgsl_desc{};
  wgsl_desc.code = R"(
    @group(0) @binding(0) var<uniform> world_mat: mat4x4f;
    @group(1) @binding(0) var<uniform> view_proj_mat: mat4x4f;

    struct VertexInput{
      @location(0) position: vec4f,
      @location(1) texcoord0: vec2f,
    };

    struct VertexOutput{
        @builtin(position) position: vec4f,
        @location(0) texcoord0: vec2f,
    }

    @vertex fn vertexMain(in: VertexInput) -> VertexOutput{
        var out: VertexOutput;
        var p: vec4f;
        p = view_proj_mat * world_mat * in.position;
        p /= p.w;
        out.position = vec4f(p.xy, p.z * 0.5 + 0.5, 1.0);
        out.texcoord0 = in.texcoord0;
        return out;
    }

    @fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f{
        return vec4f(in.texcoord0, 0.0, 1.0);
    }
  )";

  wgpu::ShaderModuleDescriptor shader_module_desc{.nextInChain = &wgsl_desc};
  wgpu::ShaderModule shader_module =
      Engine::get_module<Graphics>()->get_device().CreateShaderModule(
          &shader_module_desc);

  wgpu::ColorTargetState color_target_state{
      .format = wgpu::TextureFormat::BGRA8Unorm,
  };

  wgpu::FragmentState fragment_state{
      .module = shader_module,
      .entryPoint = "fragmentMain",
      .targetCount = 1,
      .targets = &color_target_state,
  };

  auto binding_layout_entries = std::vector<wgpu::BindGroupLayoutEntry>(2);
  // mesh constant
  binding_layout_entries.at(0) =
      wgpu::BindGroupLayoutEntry{.binding = 0,
                                 .visibility = wgpu::ShaderStage::Vertex,
                                 .buffer = wgpu::BufferBindingLayout{
                                     .type = wgpu::BufferBindingType::Uniform,
                                     .hasDynamicOffset = false,
                                     .minBindingSize = sizeof(Math::Matrix4x4f),
                                 }};

  // camera constant
  binding_layout_entries.at(1) =
      wgpu::BindGroupLayoutEntry{.binding = 0,
                                 .visibility = wgpu::ShaderStage::Vertex,
                                 .buffer = wgpu::BufferBindingLayout{
                                     .type = wgpu::BufferBindingType::Uniform,
                                     .hasDynamicOffset = false,
                                     .minBindingSize = sizeof(Math::Matrix4x4f),
                                 }};

  std::vector<wgpu::BindGroupLayoutEntry> mesh_constant_layout_entries(
      binding_layout_entries.begin(), binding_layout_entries.begin() + 1);
  wgpu::BindGroupLayoutDescriptor mesh_constant_layout_desc = {
      .entryCount = static_cast<uint32_t>(mesh_constant_layout_entries.size()),
      .entries = mesh_constant_layout_entries.data(),
  };
  mesh_constant_bind_group_layout =
      Engine::get_module<Graphics>()->get_device().CreateBindGroupLayout(
          &mesh_constant_layout_desc);

  std::vector<wgpu::BindGroupLayoutEntry> camera_layout_entries(
      binding_layout_entries.begin() + 1, binding_layout_entries.begin() + 2);
  wgpu::BindGroupLayoutDescriptor camera_layout_desc = {
      .entryCount = static_cast<uint32_t>(camera_layout_entries.size()),
      .entries = camera_layout_entries.data(),
  };
  camera_constant_bind_group_layout =
      Engine::get_module<Graphics>()->get_device().CreateBindGroupLayout(
          &camera_layout_desc);

  std::vector<wgpu::BindGroupLayout> layouts = {
      mesh_constant_bind_group_layout, camera_constant_bind_group_layout};
  wgpu::PipelineLayoutDescriptor layout_desc{
      .bindGroupLayoutCount = 2,
      .bindGroupLayouts = layouts.data(),
  };

  wgpu::PipelineLayout pipeline_layout =
      Engine::get_module<Graphics>()->get_device().CreatePipelineLayout(
          &layout_desc);

  wgpu::DepthStencilState depth_stencil_state{
      .nextInChain = nullptr,
      .format = wgpu::TextureFormat::Depth24Plus,
      .depthWriteEnabled = true,
      .depthCompare = wgpu::CompareFunction::Less,
      .stencilFront =
          wgpu::StencilFaceState{.compare = wgpu::CompareFunction::Always,
                                 .failOp = wgpu::StencilOperation::Keep,
                                 .depthFailOp = wgpu::StencilOperation::Keep,
                                 .passOp = wgpu::StencilOperation::Keep},
      .stencilBack =
          wgpu::StencilFaceState{.compare = wgpu::CompareFunction::Always,
                                 .failOp = wgpu::StencilOperation::Keep,
                                 .depthFailOp = wgpu::StencilOperation::Keep,
                                 .passOp = wgpu::StencilOperation::Keep},
      .stencilReadMask = 0,
      .stencilWriteMask = 0,
      .depthBias = 0,
      .depthBiasSlopeScale = 0.0f,
      .depthBiasClamp = 0.0f,
  };

  wgpu::TextureDescriptor depthTextureDesc{
      .nextInChain = nullptr,
      .usage = wgpu::TextureUsage::RenderAttachment,
      .dimension = wgpu::TextureDimension::e2D,
      .size = {1080, 1080, 1},
      .format = wgpu::TextureFormat::Depth24Plus,
      .mipLevelCount = 1,
      .sampleCount = 1,
      .viewFormatCount = 1,
      .viewFormats = &depth_stencil_state.format,
  };

  wgpu::Texture depthTexture =
      Engine::get_module<Graphics>()->get_device().CreateTexture(
          &depthTextureDesc);

  wgpu::TextureViewDescriptor depthTextureViewDesc{
      .nextInChain = nullptr,
      .format = wgpu::TextureFormat::Depth24Plus,
      .dimension = wgpu::TextureViewDimension::e2D,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
      .baseArrayLayer = 0,
      .arrayLayerCount = 1,
      .aspect = wgpu::TextureAspect::DepthOnly,
  };

  depthTextureView = depthTexture.CreateView(&depthTextureViewDesc);

  wgpu::RenderPipelineDescriptor render_pipeline_desc{
      .layout = pipeline_layout,
      .vertex = {.module = shader_module,
                 .entryPoint = "vertexMain",
                 .bufferCount = 1,
                 .buffers = &vertex_buffer_layout},
      .depthStencil = &depth_stencil_state,
      .fragment = &fragment_state,
  };

  render_pipeline =
      Engine::get_module<Graphics>()->get_device().CreateRenderPipeline(
          &render_pipeline_desc);

  // set up sample data
  {
    struct Vertex {
      Math::Vector4f position;
      Math::Vector2f texcoord;
    };
    std::vector<Vertex> vertices = {
        Vertex{.position = Math::Vector4f({-0.5f, -0.5f, -0.3f, 1.0f}),
               .texcoord = Math::Vector2f({0.0f, 0.0f})},
        Vertex{.position = Math::Vector4f({0.5f, -0.5f, -0.3f, 1.0f}),
               .texcoord = Math::Vector2f({1.0f, 0.0f})},
        Vertex{.position = Math::Vector4f({0.5f, 0.5f, -0.3f, 1.0f}),
               .texcoord = Math::Vector2f({1.0f, 1.0f})},
        Vertex{.position = Math::Vector4f({-0.5f, 0.5f, -0.3f, 1.0f}),
               .texcoord = Math::Vector2f({0.0f, 1.0f})},
        Vertex{.position = Math::Vector4f({0.0f, 0.0f, 0.5f, 1.0f}),
               .texcoord = Math::Vector2f({0.5f, 0.5f})}};

    // mesh vertex buffer
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "vertex buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = vertices.size() * sizeof(Vertex),
        .mappedAtCreation = false};

    mesh_vertex_buffer =
        Engine::get_module<Graphics>()->create_buffer(buffer_desc);
    Engine::get_module<Graphics>()->update_buffer(mesh_vertex_buffer, vertices);
  }

  {
    struct Index {
      uint32_t value;
    };
    std::vector<Index> indices = {
        Index{0}, Index{1}, Index{2}, Index{0}, Index{2}, Index{3},
        Index{0}, Index{1}, Index{4}, Index{1}, Index{2}, Index{4},
        Index{2}, Index{3}, Index{4}, Index{3}, Index{0}, Index{4},
    };

    // mesh index buffer
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "index buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .size = indices.size() * sizeof(Index),
        .mappedAtCreation = false};

    mesh_index_buffer =
        Engine::get_module<Graphics>()->create_buffer(buffer_desc);
    Engine::get_module<Graphics>()->update_buffer(mesh_index_buffer, indices);
  }

  {
    auto world_mat = std::vector<float>{
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    };
    // mesh constant buffer
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "mesh constant buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(float) * 16,
        .mappedAtCreation = false};

    mesh_constant_buffer =
        Engine::get_module<Graphics>()->create_buffer(buffer_desc);
    Engine::get_module<Graphics>()->update_buffer(mesh_constant_buffer,
                                                  world_mat);

    // mesh constant bind group
    auto constant_binding = wgpu::BindGroupEntry{.binding = 0,
                                                 .buffer = mesh_constant_buffer,
                                                 .offset = 0,
                                                 .size = sizeof(float) * 16};

    wgpu::BindGroupDescriptor bind_group_desc{
        .layout = mesh_constant_bind_group_layout,
        .entryCount = 1,
        .entries = &constant_binding};

    mesh_constant_bind_group =
        Engine::get_module<Graphics>()->get_device().CreateBindGroup(
            &bind_group_desc);
  }

  {
    auto view_proj_mat = std::vector<float>{
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    };
    // camera buffer
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "camera buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(Math::Matrix4x4f),
        .mappedAtCreation = false};

    camera_buffer = Engine::get_module<Graphics>()->create_buffer(buffer_desc);
    Engine::get_module<Graphics>()->update_buffer(camera_buffer, view_proj_mat);

    // camera constant bind group
    auto constant_binding = wgpu::BindGroupEntry{.binding = 0,
                                                 .buffer = camera_buffer,
                                                 .offset = 0,
                                                 .size = sizeof(float) * 16};

    wgpu::BindGroupDescriptor bind_group_desc{
        .layout = camera_constant_bind_group_layout,
        .entryCount = 1,
        .entries = &constant_binding};

    camera_constant_bind_group =
        Engine::get_module<Graphics>()->get_device().CreateBindGroup(
            &bind_group_desc);
  }
}

void ReflectionRenderer::render(wgpu::TextureView render_target) {
  // update constants
  auto entity_list = Engine::get_module<EntityStore>()->get_all();

  // create gpu resources
  for (const auto& entity : entity_list) {
    const auto& mesh = entity->get_component<Mesh>();
    const auto& transform = entity->get_component<Transform>();

    auto entity_id = entity->get_id();
    if (mesh == nullptr || transform == nullptr) {
      dispose_gpu_resource(entity_id);
      continue;
    }

    if (!gpu_resources.contains(entity_id)) {
      GPUResource resource{};
      resource.mesh_buffer = create_mesh_buffer(entity_id);
      resource.transform_buffer = create_constant_buffer(entity_id);

      gpu_resources.insert_or_assign(entity_id, resource);
    }
  }

  // update constants
  int idx = 0;
  for (auto rendered : gpu_resources) {
    const auto entity = Engine::get_module<EntityStore>()->get(rendered.first);

    // update constant buffer
    auto& gpu_transform_buffer = rendered.second.transform_buffer;
    auto transform = entity->get_component<Transform>();
    const auto translate = Math::Matrix4x4f({{
        {1.0f, 0.0f, 0.0f, transform->get_position().x},
        {0.0f, 1.0f, 0.0f, transform->get_position().y},
        {0.0f, 0.0f, 1.0f, transform->get_position().z},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }});

    const auto scale = Math::Matrix4x4f({{
        {transform->get_scale().x, 0.0f, 0.0f, 0.0f},
        {0.0f, transform->get_scale().y, 0.0f, 0.0f},
        {0.0f, 0.0f, transform->get_scale().z, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }});

    const auto ax = transform->get_euler_angle().x;
    const auto ay = transform->get_euler_angle().y;
    const auto az = transform->get_euler_angle().z;
    const auto rotate_x = Math::Matrix4x4f({{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, cos(ax), -sin(ax), 0.0f},
        {0.0f, sin(ax), cos(ax), 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }});

    const auto rotate_y = Math::Matrix4x4f({{
        {cos(ay), 0.0f, sin(ay), 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {-sin(ay), 0.0f, cos(ay), 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }});

    const auto rotate_z = Math::Matrix4x4f({{
        {cos(az), -sin(az), 0.0f, 0.0f},
        {sin(az), cos(az), 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }});

    const auto world_mat = translate * rotate_z * rotate_y * rotate_x * scale;
    auto world_mat_vec = std::vector<float>();
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        world_mat_vec.push_back(world_mat.internal_data.at(j).at(i));
      }
    }

    Engine::get_module<Graphics>()->update_buffer(gpu_transform_buffer.buffer,
                                                  world_mat_vec);
  }

  // render
  wgpu::RenderPassColorAttachment attachment{
      .view = render_target,
      .loadOp = wgpu::LoadOp::Clear,
      .storeOp = wgpu::StoreOp::Store,
  };

  wgpu::RenderPassDepthStencilAttachment depth_stencil_attachment{
      .view = depthTextureView,
      .depthLoadOp = wgpu::LoadOp::Clear,
      .depthStoreOp = wgpu::StoreOp::Store,
      .depthClearValue = 1.0f,
      .depthReadOnly = false,
      .stencilLoadOp = wgpu::LoadOp::Undefined,
      .stencilStoreOp = wgpu::StoreOp::Undefined,
      .stencilClearValue = 0,
      .stencilReadOnly = true,
  };

  wgpu::RenderPassDescriptor renderpass_desc{
      .colorAttachmentCount = 1,
      .colorAttachments = &attachment,
      .depthStencilAttachment = &depth_stencil_attachment};

  wgpu::CommandEncoder encoder =
      Engine::get_module<Graphics>()->get_device().CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass_desc);
  pass.SetPipeline(render_pipeline);

  pass.SetVertexBuffer(0, mesh_vertex_buffer, 0, mesh_vertex_buffer.GetSize());
  pass.SetIndexBuffer(mesh_index_buffer, wgpu::IndexFormat::Uint32, 0,
                      mesh_index_buffer.GetSize());
  pass.SetBindGroup(0, mesh_constant_bind_group, 0, nullptr);
  pass.SetBindGroup(1, camera_constant_bind_group, 0, nullptr);
  pass.DrawIndexed(mesh_index_buffer.GetSize() / sizeof(uint32_t), 1, 0, 0, 0);

  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();

  Engine::get_module<Graphics>()->get_device().GetQueue().Submit(1, &commands);
}

GPUMeshBuffer ReflectionRenderer::create_mesh_buffer(EntityID id) {
  auto mesh = Engine::get_module<MeshStore>()->get(id);
  auto vertices = mesh->get_vertices();
  auto indices = mesh->get_indices();

  GPUMeshBuffer gpu_mesh_buffer{};
  {
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "vertex buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = vertices.size() * sizeof(Vertex),
        .mappedAtCreation = false};

    gpu_mesh_buffer.vertex_buffer =
        Engine::get_module<Graphics>()->create_buffer(buffer_desc);
    Engine::get_module<Graphics>()->update_buffer(gpu_mesh_buffer.vertex_buffer,
                                                  vertices);
  }

  {
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "index buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .size = indices.size() * sizeof(uint32_t),
        .mappedAtCreation = false};

    gpu_mesh_buffer.index_buffer =
        Engine::get_module<Graphics>()->create_buffer(buffer_desc);
    Engine::get_module<Graphics>()->update_buffer(gpu_mesh_buffer.index_buffer,
                                                  indices);
  }
  return gpu_mesh_buffer;
}

GPUTransformBuffer ReflectionRenderer::create_constant_buffer(
    EntityID entity_id) {
  auto transform = Engine::get_module<TransformStore>()->get(entity_id);
  auto position = transform->get_position();
  auto angle = transform->get_euler_angle();
  auto scale = transform->get_scale();

  GPUTransformBuffer gpu_transform_buffer{};
  wgpu::BufferDescriptor buffer_desc{
      .nextInChain = nullptr,
      .label = "constant",
      .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
      .size = Engine::get_module<Graphics>()->get_buffer_stride(
          sizeof(Math::Matrix4x4f)),
      .mappedAtCreation = false,
  };

  auto buffer = Engine::get_module<Graphics>()->create_buffer(buffer_desc);

  auto binding = wgpu::BindGroupEntry{.binding = 0,
                                      .buffer = buffer,
                                      .offset = 0,
                                      .size = sizeof(Math::Matrix4x4f)};
  wgpu::BindGroupDescriptor bind_group_desc{
      .layout = mesh_constant_bind_group_layout,
      .entryCount = 1,
      .entries = &binding};

  gpu_transform_buffer.buffer = buffer;
  gpu_transform_buffer.bindgroup =
      Engine::get_module<Graphics>()->get_device().CreateBindGroup(
          &bind_group_desc);

  return gpu_transform_buffer;
}