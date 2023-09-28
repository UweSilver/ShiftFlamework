#include "graphics.hpp"

#include "engine.hpp"

using namespace ShiftFlamework;

void get_device(wgpu::Instance instance, void (*callback)(wgpu::Device)) {
  instance.RequestAdapter(
      nullptr,
      [](WGPURequestAdapterStatus status, WGPUAdapter c_adapter,
         const char* message, void* userdata) {
        if (status != WGPURequestAdapterStatus_Success) {
          std::cerr << "failed to get adapter" << std::endl;
          return;
        }
        wgpu::Adapter adapter = wgpu::Adapter::Acquire(c_adapter);
        Engine::get_module<Graphics>()->adapter = adapter;
        adapter.RequestDevice(
            nullptr,
            [](WGPURequestDeviceStatus status, WGPUDevice c_device,
               const char* message, void* userdata) {
              wgpu::Device device = wgpu::Device::Acquire(c_device);
              reinterpret_cast<void (*)(wgpu::Device)>(userdata)(device);
            },
            userdata);
      },
      reinterpret_cast<void*>(callback));
}

void on_device_request_ended(wgpu::Device device) {
  Engine::get_module<Graphics>()->device = device;
  Engine::get_module<Graphics>()->on_initialize_end();
}

void Graphics::initialize(std::function<void()> on_initialize_end) {
  instance = wgpu::CreateInstance();
  this->on_initialize_end = on_initialize_end;
  get_device(instance, on_device_request_ended);
}

void Graphics::create_render_pipeline() {
  vertex_data = {
      // x, y, r, g, b
      -0.5, -0.5, 1.0, 0.0, 0.0, +0.5, -0.5, 0.0, 1.0, 0.0,
      +0.5, +0.5, 0.0, 0.0, 1.0, -0.5, +0.5, 1.0, 1.0, 0.0,
  };
  index_data = {0, 1, 2, 0, 2, 3};

  // initialize vertex buffer
  {
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "vertex buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = vertex_data.size() * sizeof(float),
        .mappedAtCreation = false};
    vertex_buffer = device.CreateBuffer(&buffer_desc);
    device.GetQueue().WriteBuffer(vertex_buffer, 0, vertex_data.data(),
                                  buffer_desc.size);
  }

  // initialize index buffer
  {
    const wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "index buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
        .size = index_data.size() * sizeof(uint16_t),
        .mappedAtCreation = false};
    index_buffer = device.CreateBuffer(&buffer_desc);
    device.GetQueue().WriteBuffer(index_buffer, 0, index_data.data(),
                                  buffer_desc.size);
  }

  {
    wgpu::BufferDescriptor buffer_desc{
        .nextInChain = nullptr,
        .label = "uTime",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(float),
        .mappedAtCreation = false,
    };
    time_buffer = device.CreateBuffer(&buffer_desc);
  }

  std::vector<wgpu::VertexAttribute> vertex_attributes(2);
  vertex_attributes.at(0).format = wgpu::VertexFormat::Float32x2,
  vertex_attributes.at(0).offset = 0,
  vertex_attributes.at(0).shaderLocation = 0;

  vertex_attributes.at(1).format = wgpu::VertexFormat::Float32x3,
  vertex_attributes.at(1).offset = 2 * sizeof(float),
  vertex_attributes.at(1).shaderLocation = 1;

  wgpu::VertexBufferLayout vertex_buffer_layout{
      .arrayStride = 5 * sizeof(float),
      .stepMode = wgpu::VertexStepMode::Vertex,
      .attributeCount = static_cast<uint32_t>(vertex_attributes.size()),
      .attributes = vertex_attributes.data(),
  };

  wgpu::BindGroupLayoutEntry binding_layout{
      .binding = 0,
      .visibility = wgpu::ShaderStage::Vertex,
      .buffer = wgpu::BufferBindingLayout{
          .type = wgpu::BufferBindingType::Uniform,
          .minBindingSize = sizeof(float),
      }};

  wgpu::BindGroupLayoutDescriptor bind_group_layout_desc{
      .entryCount = 1, .entries = &binding_layout};

  wgpu::BindGroupLayout bind_group_layout =
      device.CreateBindGroupLayout(&bind_group_layout_desc);

  wgpu::PipelineLayoutDescriptor layout_desc{
      .bindGroupLayoutCount = 1, .bindGroupLayouts = &bind_group_layout};

  wgpu::PipelineLayout pipeline_layout =
      device.CreatePipelineLayout(&layout_desc);

  wgpu::BindGroupEntry binding{
      .binding = 0,
      .buffer = time_buffer,
      .offset = 0,
      .size = sizeof(float),
  };

  wgpu::BindGroupDescriptor bind_group_desc{
      .layout = bind_group_layout,
      .entryCount = bind_group_layout_desc.entryCount,
      .entries = &binding,
  };

  bind_group = device.CreateBindGroup(&bind_group_desc);

  wgpu::ShaderModuleWGSLDescriptor wgsl_desc{};
  wgsl_desc.code = R"(
      @group(0) @binding(0) var<uniform> uTime: f32;

      struct VertexInput{
        @location(0) position: vec2f,
        @location(1) color: vec3f,
      }

      struct VertexOutput{
        @builtin(position) position: vec4f,
        @location(0) color: vec3f,
      }

      @vertex fn vertexMain(in: VertexInput) -> VertexOutput {
        var out: VertexOutput;
        out.position = vec4f(in.position + 0.3 * vec2f(cos(uTime), sin(uTime)), 0.0, 1.0);
        out.color = in.color; 
        return out;
      }

      @fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f {
        return vec4f(in.color, 1.0);
      }
    )";

  wgpu::ShaderModuleDescriptor shader_module_desc{.nextInChain = &wgsl_desc};
  wgpu::ShaderModule shader_module =
      device.CreateShaderModule(&shader_module_desc);

  wgpu::ColorTargetState color_target_state{
      .format = wgpu::TextureFormat::BGRA8Unorm};
  wgpu::FragmentState fragment_state{.module = shader_module,
                                     .entryPoint = "fragmentMain",
                                     .targetCount = 1,
                                     .targets = &color_target_state};

  wgpu::RenderPipelineDescriptor render_pipeline_desc{
      .vertex = {.module = shader_module,
                 .entryPoint = "vertexMain",
                 .bufferCount = 1,
                 .buffers = &vertex_buffer_layout},
      .fragment = &fragment_state};

  pipeline = device.CreateRenderPipeline(&render_pipeline_desc);
}

void Graphics::render(wgpu::TextureView current_texture_view) {
  wgpu::RenderPassColorAttachment attachment{.view = current_texture_view,
                                             .loadOp = wgpu::LoadOp::Clear,
                                             .storeOp = wgpu::StoreOp::Store};

  wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1,
                                        .colorAttachments = &attachment};

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

  // updata constant
  time_data += 0.1f;
  encoder.WriteBuffer(time_buffer, 0,
                      reinterpret_cast<const uint8_t*>(&time_data),
                      sizeof(float));

  // render pass
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
  pass.SetPipeline(pipeline);
  pass.SetVertexBuffer(0, vertex_buffer, 0, vertex_data.size() * sizeof(float));
  pass.SetIndexBuffer(index_buffer, wgpu::IndexFormat::Uint16, 0,
                      index_data.size() * sizeof(uint16_t));
  pass.SetBindGroup(0, bind_group, 0, nullptr);
  pass.DrawIndexed(index_data.size(), 1, 0, 0);
  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();
  device.GetQueue().Submit(1, &commands);
}