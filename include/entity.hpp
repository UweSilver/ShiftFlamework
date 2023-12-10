#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace ShiftFlamework {
class Component;
class Entity;
class Component {
 private:
 public:
  Component(){};
  std::shared_ptr<Entity> entity = nullptr;
  virtual void on_register(){};
};

class Entity : public std::enable_shared_from_this<Entity> {
 public:
  Entity() { component.clear(); }
  std::unordered_map<std::string, std::shared_ptr<Component>> component{};

  template <class T>
  std::shared_ptr<T> get_component() {
    auto name = typeid(T).name();
    auto ptr = component.at(name);
    auto r = std::reinterpret_pointer_cast<T>(ptr);
    return r;
  }

  template <class T,
            typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
  std::shared_ptr<T> add_component() {
    std::shared_ptr<T> ptr = std::make_shared<T>();
    std::string str = typeid(T).name();
    std::shared_ptr<Component> casted =
        std::static_pointer_cast<Component>(ptr);
    component.emplace(str, casted);
    ptr->entity = shared_from_this();
    ptr->on_register();
    return ptr;
  }
};
}  // namespace ShiftFlamework