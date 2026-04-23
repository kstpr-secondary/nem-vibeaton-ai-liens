#include <catch2/catch_test_macros.hpp>

#include "engine.h"

// ---------------------------------------------------------------------------
// Fixture: init/shutdown around every TEST_CASE
// ---------------------------------------------------------------------------

struct EngineFixture {
    EngineFixture()  { engine_init(EngineConfig{}); }
    ~EngineFixture() { engine_shutdown(); }
};

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------

TEST_CASE("create entity is valid", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    REQUIRE(engine_registry().valid(e));
}

TEST_CASE("null entity is not valid", "[ecs]") {
    EngineFixture f;
    REQUIRE_FALSE(engine_registry().valid(entt::null));
}

TEST_CASE("multiple entities are all valid", "[ecs]") {
    EngineFixture f;
    std::vector<entt::entity> entities;
    for (int i = 0; i < 10; ++i)
        entities.push_back(engine_create_entity());
    for (auto e : entities)
        REQUIRE(engine_registry().valid(e));
}

TEST_CASE("destroy_entity defers deletion until engine_tick", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    engine_destroy_entity(e);
    REQUIRE(engine_registry().valid(e));   // still alive before tick
    engine_tick(0.016f);
    REQUIRE_FALSE(engine_registry().valid(e));  // gone after tick
}

TEST_CASE("destroy invalid entity is silently ignored", "[ecs]") {
    EngineFixture f;
    REQUIRE_NOTHROW(engine_destroy_entity(entt::null));
}

// ---------------------------------------------------------------------------
// Component operations
// ---------------------------------------------------------------------------

TEST_CASE("add and get component", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    auto& t = engine_add_component<Transform>(e);
    t.position = {1.f, 2.f, 3.f};
    REQUIRE(engine_get_component<Transform>(e).position == glm::vec3(1.f, 2.f, 3.f));
}

TEST_CASE("has_component returns false before add, true after", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    REQUIRE_FALSE(engine_has_component<Transform>(e));
    engine_add_component<Transform>(e);
    REQUIRE(engine_has_component<Transform>(e));
}

TEST_CASE("remove_component clears the component", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    engine_add_component<Transform>(e);
    REQUIRE(engine_has_component<Transform>(e));
    engine_remove_component<Transform>(e);
    REQUIRE_FALSE(engine_has_component<Transform>(e));
}

TEST_CASE("try_get returns nullptr when component absent", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    REQUIRE(engine_try_get_component<Transform>(e) == nullptr);
}

TEST_CASE("try_get returns valid pointer when component present", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    engine_add_component<Transform>(e);
    REQUIRE(engine_try_get_component<Transform>(e) != nullptr);
}

TEST_CASE("get_component returns mutable reference", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    engine_add_component<Transform>(e);
    engine_get_component<Transform>(e).position = {5.f, 0.f, 0.f};
    REQUIRE(engine_get_component<Transform>(e).position.x == 5.f);
}

TEST_CASE("multiple component types on same entity", "[ecs]") {
    EngineFixture f;
    auto e = engine_create_entity();
    engine_add_component<Transform>(e).position = {1.f, 0.f, 0.f};
    engine_add_component<RigidBody>(e).mass = 2.f;
    REQUIRE(engine_has_component<Transform>(e));
    REQUIRE(engine_has_component<RigidBody>(e));
    REQUIRE(engine_get_component<RigidBody>(e).mass == 2.f);
}

TEST_CASE("engine_registry view iterates matching entities", "[ecs]") {
    EngineFixture f;
    // create 3 entities with Transform, 1 without
    for (int i = 0; i < 3; ++i)
        engine_add_component<Transform>(engine_create_entity());
    engine_create_entity();  // no Transform

    int count = 0;
    for (auto e : engine_registry().view<Transform>()) {
        (void)e;
        ++count;
    }
    REQUIRE(count == 3);
}
