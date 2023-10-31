#include "material.hpp"

#include "engine.hpp"
using namespace ShiftFlamework;

void Material::create_gpu_buffer(uint32_t height, uint32_t width,
                                 const uint8_t *data) {
  wgpu::TextureDescriptor texture_desc{
      .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
      .dimension = wgpu::TextureDimension::e2D,
      .size = {width, height, 1},
      .format = wgpu::TextureFormat::RGBA8Unorm,
      .mipLevelCount = 1,
      .sampleCount = 1,
      .viewFormatCount = 0,
      .viewFormats = nullptr};

  texture = Engine::get_module<Graphics>()->device.CreateTexture(&texture_desc);

  std::vector<uint8_t> pixels(4 * width * height, 0);

  uint32_t idx = 0;
  for (uint32_t i = 0; i < width; i++) {
    for (uint32_t j = 0; j < height; j++) {
      std::array<uint8_t, 4> raw = {};
      raw.at(0) = (uint8_t)data[idx];
      raw.at(1) = (uint8_t)data[idx + 1];
      raw.at(2) = (uint8_t)data[idx + 2];
      raw.at(3) = (uint8_t)data[idx + 3];
      idx += 4;

      pixels.at(4 * ((width - 1 - i) * height + j) + 0) =
          (((raw.at(0) - 33) << 2) | ((raw.at(1) - 33) >> 4));
      pixels.at(4 * ((width - 1 - i) * height + j) + 1) =
          ((((raw.at(1) - 33) & 0xF) << 4) | ((raw.at(2) - 33) >> 2));
      pixels.at(4 * ((width - 1 - i) * height + j) + 2) =
          ((((raw.at(2) - 33) & 0x3) << 6) | ((raw.at(3) - 33)));
    }
  }

  wgpu::ImageCopyTexture destination{
      .texture = texture,
      .mipLevel = 0,
      .origin = {0, 0, 0},
      .aspect = wgpu::TextureAspect::All,
  };

  wgpu::TextureDataLayout source{
      .offset = 0,
      .bytesPerRow = 4 * width,
      .rowsPerImage = height,
  };

  Engine::get_module<Graphics>()->device.GetQueue().WriteTexture(
      &destination, pixels.data(), pixels.size(), &source, &texture_desc.size);

  auto texture_view_desc = wgpu::TextureViewDescriptor{
      .format = texture_desc.format,
      .dimension = wgpu::TextureViewDimension::e2D,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
      .baseArrayLayer = 0,
      .arrayLayerCount = 1,
      .aspect = wgpu::TextureAspect::All,
  };

  texture_view = texture.CreateView(&texture_view_desc);

  auto sampler_decs =
      wgpu::SamplerDescriptor{.addressModeU = wgpu::AddressMode::ClampToEdge,
                              .addressModeV = wgpu::AddressMode::ClampToEdge,
                              .addressModeW = wgpu::AddressMode::ClampToEdge,
                              .magFilter = wgpu::FilterMode::Linear,
                              .minFilter = wgpu::FilterMode::Linear,
                              .mipmapFilter = wgpu::MipmapFilterMode::Linear,
                              .lodMinClamp = 0.0f,
                              .lodMaxClamp = 1.0f,
                              .compare = wgpu::CompareFunction::Undefined,
                              .maxAnisotropy = 1};

  sampler = Engine::get_module<Graphics>()->device.CreateSampler(&sampler_decs);

  bindgroup =
      Engine::get_module<ScreenSpaceMeshRenderer>()->create_texture_bind_group(
          texture_view, sampler);
}