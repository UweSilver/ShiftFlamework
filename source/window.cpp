#include "window.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#include <webgpu/webgpu_glfw.h>
#endif

using namespace SF;

Window::Window(const std::string window_name, const uint32_t width,
               const uint32_t height)
    : name(window_name), width(width), height(height) {
  if (!glfwInit()) {
    std::cerr << "failed to initialize glfw" << std::endl;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
}

bool Window::initialize_swap_chain(const wgpu::Instance& instance,
                                   const wgpu::Device& device) {
#if defined(__EMSCRIPTEN__)
  wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvas_desc{};
  canvas_desc.selector = "#canvas";
  wgpu::SurfaceDescriptor surface_desc{.nextInChain = &canvas_desc};
  surface = instance.CreateSurface(&surface_desc);
#else
  surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
#endif

  wgpu::SwapChainDescriptor sc_desc{
      .usage = wgpu::TextureUsage::RenderAttachment,
      .format = wgpu::TextureFormat::BGRA8Unorm,
      .width = width,
      .height = height,
      .presentMode = wgpu::PresentMode::Fifo};
  swap_chain = device.CreateSwapChain(surface, &sc_desc);
  return true;
}

void main_loop_internal(void* arg) {
  static_cast<Window*>(arg)->call_main_loop();
}

void Window::start_main_loop(std::function<void()> main_loop) {
  this->main_loop = main_loop;
#if defined(__EMSCRIPTEN__)
  emscripten_set_main_loop_arg(main_loop_internal, this, 0, false);
#else
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    main_loop();
    swap_chain.Present();
  }
#endif
}