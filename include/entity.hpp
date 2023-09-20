#pragma once
#include <memory>
#include <string>
#include <vector>

namespace ShiftFlamework {
class Component;
class Entity;
class Component {
 private:
 public:
  Component(std::shared_ptr<Entity> parent) : entity(parent){};
  std::shared_ptr<Entity> entity;
  std::shared_ptr<Entity> get_entity() { return entity; }
};

class Transform : public Component {
 public:
  Transform(std::shared_ptr<Entity> parent) : Component(parent){}
  float value;
};

class Mesh : public Component {
 public:
  Mesh(std::shared_ptr<Entity> parent) : Component(parent){}
  std::string value;
};

class Entity : public std::enable_shared_from_this<Entity>{
 public:
  Entity() {
    component.clear();
  }
  std::vector<std::shared_ptr<Component>> component{};

  template <class T>
  std::shared_ptr<T> get_component() {
    for (auto& ptr : component) {
      auto r = std::reinterpret_pointer_cast<T>(ptr);
      if (r != nullptr) return r;
    }

    return nullptr;
  }

  template <class T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
  std::shared_ptr<T> add_component() {
    std::shared_ptr<T> ptr = std::make_shared<T>(shared_from_this());
    std::shared_ptr<Component> casted = std::static_pointer_cast<Component>(ptr);
    component.push_back(casted);
    return ptr;
  }
};
}  // namespace ShiftFlamework