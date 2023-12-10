#include "rigid_body.hpp"

#include "engine.hpp"
#include "screenspace_physics.hpp"
using namespace ShiftFlamework;

RigidBody::RigidBody(float radius, Math::Vector2f velocity)
    : radius(radius),
      velocity(velocity){

      };

void RigidBody::on_register() {
  Engine::get_module<ScreenSpacePhysics>()->register_body(
      entity->get_component<RigidBody>());
};