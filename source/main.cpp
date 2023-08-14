#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include <iostream>
#include <memory>

const uint32_t kWidth = 512;
const uint32_t kHeight = 512;
wgpu::Instance instance;
wgpu::Device device;
wgpu::SwapChain swapChain;
wgpu::RenderPipeline pipeline;

void SetupSwapChain(wgpu::Surface surface) {
  wgpu::SwapChainDescriptor scDesc{
      .usage = wgpu::TextureUsage::RenderAttachment,
      .format = wgpu::TextureFormat::BGRA8Unorm,
      .width = kWidth,
      .height = kHeight,
      .presentMode = wgpu::PresentMode::Fifo};
  swapChain = device.CreateSwapChain(surface, &scDesc);
}

const char shaderCode[] = R"(
    @vertex fn vertexMain(@builtin(vertex_index) i : u32) ->
      @builtin(position) vec4f {
        const pos = array(vec2f(0, 1), vec2f(-1, -1), vec2f(1, -1));
        return vec4f(pos[i], 0, 1);
    }
    @fragment fn fragmentMain() -> @location(0) vec4f {
        return vec4f(1, 0, 0, 1);
    }
)";

void CreateRenderPipeline() {
  wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
  wgslDesc.code = shaderCode;

  wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgslDesc};
  wgpu::ShaderModule shaderModule =
      device.CreateShaderModule(&shaderModuleDescriptor);

  wgpu::ColorTargetState colorTargetState{.format =
                                              wgpu::TextureFormat::BGRA8Unorm};

  wgpu::FragmentState fragmentState{.module = shaderModule,
                                    .entryPoint = "fragmentMain",
                                    .targetCount = 1,
                                    .targets = &colorTargetState};

  wgpu::RenderPipelineDescriptor descriptor{
      .vertex = {.module = shaderModule, .entryPoint = "vertexMain"},
      .fragment = &fragmentState};
  pipeline = device.CreateRenderPipeline(&descriptor);
}

void InitGraphics(wgpu::Surface surface) {
  SetupSwapChain(surface);
  CreateRenderPipeline();
}

void Render() {
  wgpu::RenderPassColorAttachment attachment{
      .view = swapChain.GetCurrentTextureView(),
      .loadOp = wgpu::LoadOp::Clear,
      .storeOp = wgpu::StoreOp::Store};

  wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1,
                                        .colorAttachments = &attachment};

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
  pass.SetPipeline(pipeline);
  pass.Draw(3);
  pass.End();
  wgpu::CommandBuffer commands = encoder.Finish();
  device.GetQueue().Submit(1, &commands);
}

void Start() {
  if (!glfwInit()) {
    return;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window =
      glfwCreateWindow(kWidth, kHeight, "WebGPU window", nullptr, nullptr);

  wgpu::Surface surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);

  InitGraphics(surface);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    Render();
    swapChain.Present();
  }
}

void GetDevice(void (*callback)(wgpu::Device)) {
  instance.RequestAdapter(
      nullptr,
      [](WGPURequestAdapterStatus status, WGPUAdapter cAdapter,
         const char* message, void* userdata) {
        if (status != WGPURequestAdapterStatus_Success) {
          exit(0);
        }
        wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
        adapter.RequestDevice(
            nullptr,
            [](WGPURequestDeviceStatus status, WGPUDevice cDevice,
               const char* message, void* userdata) {
              wgpu::Device device = wgpu::Device::Acquire(cDevice);
              reinterpret_cast<void (*)(wgpu::Device)>(userdata)(device);
            },
            userdata);
      },
      reinterpret_cast<void*>(callback));
}

int main() {
  instance = wgpu::CreateInstance();
  GetDevice([](wgpu::Device dev) {
    device = dev;
    Start();
  });
  wgpu::SupportedLimits limits;
  device.GetLimits(&limits);
  std::cout<< limits.limits.maxBindGroups << std::endl;
}