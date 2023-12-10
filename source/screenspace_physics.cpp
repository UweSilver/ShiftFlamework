#include "screenspace_physics.hpp"

#include "entity.hpp"
#include "screenspace_transform.hpp"
using namespace ShiftFlamework;

void ScreenSpacePhysics::register_body(std::shared_ptr<RigidBody> body) {
  body_list.push_back(body);
};

void ScreenSpacePhysics::unregister_body(std::shared_ptr<RigidBody> body) {
  auto ptr = std::begin(body_list);
  while (ptr->lock() != body && ptr != std::end(body_list)) ptr++;
  body_list.erase(ptr);
}

void ScreenSpacePhysics::initialize() {}

void ScreenSpacePhysics::update() {
  for (auto body_wptr : body_list) {
    if (std::shared_ptr<RigidBody> body = body_wptr.lock()) {
      auto old_position =
          body->entity->get_component<ScreenSpaceTransform>()->position;

      // gravity
      // if (old_position.internal_data[1] - body->radius > -1.0f + 0.001f)
      body->velocity += body->gravity;

      // collide with wall
      if ((old_position + body->velocity).internal_data[0] + body->radius >
          1.0f) {
        body->velocity =
            Math::Vector2f({std::min(0.0f, -body->velocity.internal_data[0] *
                                               body->restitution),
                            body->velocity.internal_data[1]});
      }
      if ((old_position + body->velocity).internal_data[0] - body->radius <
          -1.0f) {
        body->velocity =
            Math::Vector2f({std::max(0.0f, -body->velocity.internal_data[0] *
                                               body->restitution),
                            body->velocity.internal_data[1]});
      }
      if ((old_position + body->velocity).internal_data[1] + body->radius >
          1.0) {
        body->velocity =
            Math::Vector2f({body->velocity.internal_data[0],
                            std::min(0.0f, -body->velocity.internal_data[1] *
                                               body->restitution)});
      }
      if ((old_position + body->velocity).internal_data[1] - body->radius <
          -1.0f) {
        body->velocity =
            Math::Vector2f({body->velocity.internal_data[0],
                            std::max(0.0f, -body->velocity.internal_data[1] *
                                               body->restitution)});
      }

      // collide with other circle
      for (const auto circle : body_list) {
        if (circle.lock() == body) break;
        if (std::shared_ptr<RigidBody> other_body = circle.lock()) {
          const auto pos_diff =
              (old_position + body->velocity) -
              (other_body->entity->get_component<ScreenSpaceTransform>()
                   ->position +
               other_body->velocity);

          if (Math::Length(pos_diff) < (body->radius + other_body->radius)) {
            body->velocity -= body->gravity;
            other_body->velocity -= other_body->gravity;
            const auto normal = Math::GetNormalized(pos_diff);
            body->velocity += -2.0 * Math::Dot(body->velocity, normal) *
                              body->velocity * body->restitution;
            other_body->velocity +=
                -2.0 * Math::Dot(other_body->velocity, -normal) *
                other_body->velocity * other_body->restitution;

            body->entity->get_component<ScreenSpaceTransform>()->position +=
                -Math::GetNormalized(pos_diff) *
                (body->radius + other_body->radius - Math::Length(pos_diff)) *
                0.5 * 1.1;
            other_body->entity->get_component<ScreenSpaceTransform>()
                ->position +=
                Math::GetNormalized(pos_diff) *
                (body->radius + other_body->radius - Math::Length(pos_diff)) *
                0.5 * 1.1;

            old_position =
                body->entity->get_component<ScreenSpaceTransform>()->position;
          }
        }
      }

      // update transform
      body->entity->get_component<ScreenSpaceTransform>()->position +=
          body->velocity;
      auto transform = body->entity->get_component<ScreenSpaceTransform>();
      const auto before_fix_pos = transform->position;
      transform->position.internal_data[0] =
          std::min(1.0f - body->radius, transform->position.internal_data[0]);
      transform->position.internal_data[0] =
          std::max(-1.0f + body->radius, transform->position.internal_data[0]);
      transform->position.internal_data[1] =
          std::min(1.0f - body->radius, transform->position.internal_data[1]);
      transform->position.internal_data[1] =
          std::max(-1.0f + body->radius, transform->position.internal_data[1]);
      if (transform->position.internal_data[1] !=
          transform->position.internal_data[1]) {
        body->velocity += body->gravity;
      }

      body->velocity =
          (transform->position - old_position) * body->deceleration;
    }
  }
}