#include "screenspace_mesh.hpp"

#include "engine.hpp"
#include "screenspace_mesh_renderer.hpp"

using namespace ShiftFlamework;

const std::vector<ScreenSpaceVertex> ScreenSpaceMesh::get_vertices() {
  return vertices;
}

const std::vector<uint32_t> ScreenSpaceMesh::get_indices() { return indices; }

std::shared_ptr<ScreenSpaceMeshStore> ScreenSpaceMesh::get_store() {
  return Engine::get_module<ScreenSpaceMeshStore>();
}