#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>

namespace ShiftFlamework {
class Engine {
 private:
  Engine();

 public:
  static std::unordered_map<std::string, std::shared_ptr<void>> modules;

  template <typename T>
  static std::shared_ptr<T> get_module() {
    auto type = typeid(T).name();
    return std::static_pointer_cast<T>(Engine::modules.at(type));
  }

  template <typename T>
  static std::shared_ptr<T> add_module() {
    std::shared_ptr<T> ptr = std::make_shared<T>();
    std::string str = typeid(T).name();
    modules.emplace(str, ptr);
    return ptr;
  }

  template <typename T>
  static std::shared_ptr<T> add_module(std::shared_ptr<T> ptr) {
    std::string str = typeid(T).name();
    modules.emplace(str, ptr);
    return ptr;
  }
};
}  // namespace ShiftFlamework