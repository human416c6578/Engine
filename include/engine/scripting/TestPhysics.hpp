#pragma once

#include "engine/core/se_logger.hpp"
#include "engine/platform/se_input_system.hpp"
#include "engine/resources/se_resource_manager.hpp"
#include "engine/scene/se_scene.hpp"
#include "engine/scripting/se_script_component.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <random>
#include <vector>

namespace se {
    class TestPhysics : public ScriptComponent {
    public:
        void onUpdate(float, ScriptContext &context) override {
            if (!owner) return;
            auto &input = context.input;

            if (input.isKeyJustPressed(GLFW_KEY_1)) {
                requestTest(context, 1);
            } else if (input.isKeyJustPressed(GLFW_KEY_2)) {
                requestTest(context, 2);
            } else if (input.isKeyJustPressed(GLFW_KEY_3)) {
                requestTest(context, 3);
            } else if (input.isKeyJustPressed(GLFW_KEY_4)) {
                requestSpawn(context, Shape::Sphere);
            } else if (input.isKeyJustPressed(GLFW_KEY_5)) {
                requestSpawn(context, Shape::Cube);
            } else if (input.isKeyJustPressed(GLFW_KEY_6)) {
                requestSpawn(context, Shape::Cylinder);
            } else if (input.isKeyJustPressed(GLFW_KEY_R)) {
                if (currentTest != 0) requestTest(context, currentTest);
            } else if (input.isKeyJustPressed(GLFW_KEY_K)) {
                kickBodies();
            }
        }

        std::string getName() const override { return "TestPhysics"; }

    private:
        enum class Shape { Sphere, Cube, Cylinder };

        void testOne(ScriptContext &context) {
            spawnObjects(context, 100, 250, 500);
        }

        void testTwo(ScriptContext &context) {
            spawnObjects(context, 250, 500, 1000);
        }

        void testThree(ScriptContext &context) {
            spawnObjects(context, 500, 1000, 2500);
        }

        void requestTest(ScriptContext &context, int test) {
            const auto managerId = owner->getId();
            context.scene.defer([this, context, managerId, test]() mutable {
                auto *manager = context.scene.getGameObjectById(static_cast<int>(managerId));
                if (!manager || manager->getScript().get() != this) return;

                if (test == 1) testOne(context);
                else if (test == 2) testTwo(context);
                else if (test == 3) testThree(context);
                currentTest = test;
            });
        }

        void requestSpawn(ScriptContext &context, Shape shape) {
            const auto managerId = owner->getId();
            context.scene.defer([this, context, managerId, shape]() mutable {
                auto *manager = context.scene.getGameObjectById(static_cast<int>(managerId));
                if (!manager || manager->getScript().get() != this) return;

                auto mesh = shape == Shape::Sphere ? context.resources.createSphere("Sphere") :
                            shape == Shape::Cube ? context.resources.createCube("Cube") :
                            context.resources.createCylinder("Cylinder");
                auto material = owner->getMaterial();
                if (!material) material = context.resources.createMaterial("Physics Bodies");
                if (!mesh || !material) return;

                const glm::vec3 position = owner->getTransform().translation +
                    glm::vec3{random(-6.0f, 6.0f), -8.0f, random(-6.0f, 6.0f)};
                spawnObject(context, shape, mesh, material, position);
            });
        }

        void clearBodies() {
            for (const auto &handle : bodies) {
                if (auto *object = handle.get()) {
                    object->onDestroy();
                    object->getParticleEmitter().release();
                    handle.destroy();
                }
            }
            bodies.clear();
        }

        void spawnObjects(ScriptContext &context, int spheres, int cubes, int cylinders) {
            auto sphereMesh = context.resources.createSphere("Sphere");
            auto cubeMesh = context.resources.createCube("Cube");
            auto cylinderMesh = context.resources.createCylinder("Cylinder");
            auto material = owner->getMaterial();
            if (!material) material = context.resources.createMaterial("Physics Bodies");
            if (!material || !sphereMesh || !cubeMesh || !cylinderMesh) return;

            clearBodies();
            generator.seed(12345u);
            std::vector<Shape> shapes;
            shapes.insert(shapes.end(), spheres, Shape::Sphere);
            shapes.insert(shapes.end(), cubes, Shape::Cube);
            shapes.insert(shapes.end(), cylinders, Shape::Cylinder);
            std::shuffle(shapes.begin(), shapes.end(), generator);
            bodies.reserve(shapes.size());

            for (Shape shape : shapes) {
                auto mesh = shape == Shape::Sphere ? sphereMesh :
                            shape == Shape::Cube ? cubeMesh : cylinderMesh;
                const std::size_t index = bodies.size();
                const glm::vec3 position = owner->getTransform().translation + glm::vec3{
                    (static_cast<int>(index % 20) - 9.5f) * .65f + random(-.04f, .04f),
                    1.5f - static_cast<int>(index / 400) * .65f + random(-.04f, .04f),
                    (static_cast<int>((index / 20) % 20) - 9.5f) * .65f + random(-.04f, .04f)};
                spawnObject(context, shape, mesh, material, position);
            }
            log::info("PhysicsStress", "%d spheres, %d cubes, %d cylinders (%zu total)",
                      spheres, cubes, cylinders, bodies.size());
        }

        void spawnObject(ScriptContext &context, Shape shape,
                         const std::shared_ptr<SEMesh> &mesh,
                         const std::shared_ptr<SEMaterial> &material,
                         const glm::vec3 &position) {
            const std::size_t index = bodies.size();
            auto object = context.scene.createGameObject("PhysicsBody_" + std::to_string(index));
            object->setMesh(mesh);
            object->setMaterial(material);

            object->getTransform().translation = position;

            const float radius = random(.13f, .18f);
            const float halfHeight = shape == Shape::Cylinder ? random(.18f, .25f) : radius;
            SECollider collider{};
            if (shape == Shape::Sphere) collider.shape = SphereShape{radius};
            else if (shape == Shape::Cube) collider.shape = BoxShape{glm::vec3{radius}};
            else collider.shape = CylinderShape{radius, halfHeight};
            object->setCollider(collider);

            // Sphere primitive radius is 1; cube/cylinder half-size is 0.5.
            object->getTransform().scale = shape == Shape::Sphere ? glm::vec3{radius} :
                glm::vec3{2 * radius, 2 * halfHeight, 2 * radius};
            SERigidbody body{};
            body.mass = random(.5f, 2.0f);
            body.velocity = {random(-.15f, .15f), 0, random(-.15f, .15f)};
            object->setRigidbody(body);
            bodies.push_back(object);
        }

        void kickBodies() {
            for (const auto &handle : bodies) {
                auto *object = handle.get();
                auto *body = object ? object->getRigidbody() : nullptr;
                if (body && !body->isStatic)
                    body->velocity += glm::vec3{random(-2, 2), random(-6, -3), random(-2, 2)};
            }
        }

        float random(float low, float high) {
            return std::uniform_real_distribution<float>{low, high}(generator);
        }

        int currentTest = 0;
        std::vector<GameObjectHandle> bodies;
        std::mt19937 generator{12345u};
    };
}
