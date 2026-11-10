#include "engine/physics/se_physics.hpp"
#include "engine/physics/se_collider.hpp"
#include "engine/physics/se_rigidbody.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <tuple>
#include <glm/geometric.hpp>

namespace {
    float inverseMass(const se::SERigidbody *body) {
        if (!body || body->isStatic)
            return 0.0f;
        assert(std::isfinite(body->mass) && body->mass > 0.0f);
        return body->getInverseMass();
    }

    se::WorldSphere worldShape(const se::SphereShape &shape,
                               const glm::vec3 &center) {
        return {center, shape.radius};
    }

    se::WorldBox worldShape(const se::BoxShape &shape,
                            const glm::vec3 &center) {
        return {center, shape.halfExtents};
    }

    se::WorldBox worldShape(const se::CylinderShape &shape,
                            const glm::vec3 &center) {
        return {center, glm::vec3{shape.radius, shape.halfHeight, shape.radius}};
    }

    se::WorldBox boundingBox(const se::WorldSphere &sphere) {
        return {sphere.center, glm::vec3{sphere.radius}};
    }

    se::WorldBox boundingBox(const se::WorldBox &box) {
        return box;
    }

    bool findCellRange(const se::WorldBox &bounds, float cellSize,
                       glm::ivec3 &minimumCell, glm::ivec3 &maximumCell) {
        constexpr std::size_t maxCellsPerBody = 4096;
        std::size_t cellCount = 1;
        for (int axis = 0; axis < 3; ++axis) {
            const double low = std::floor((static_cast<double>(bounds.center[axis]) -
                                           bounds.halfExtents[axis]) / cellSize);
            const double high = std::floor((static_cast<double>(bounds.center[axis]) +
                                            bounds.halfExtents[axis]) / cellSize);
            if (!std::isfinite(low) || !std::isfinite(high) || low > high ||
                low < std::numeric_limits<int>::min() ||
                high >= std::numeric_limits<int>::max())
                return false;
            const double width = high - low + 1.0;
            if (width > maxCellsPerBody) return false;
            cellCount *= static_cast<std::size_t>(width);
            if (cellCount > maxCellsPerBody) return false;
            minimumCell[axis] = static_cast<int>(low);
            maximumCell[axis] = static_cast<int>(high);
        }
        return true;
    }

    void resolveContact(se::PhysicsBodyView &a, se::PhysicsBodyView &b,
                        const se::Contact &contact) {
        const float invA = inverseMass(a.rigidbody);
        const float invB = inverseMass(b.rigidbody);
        const float invSum = invA + invB;
        if (invSum <= 0.0f)
            return;

        // Leave a small tolerance to reduce jitter. Correct gradually so
        // repeated pair passes can accommodate neighboring contacts without
        // large jumps.
        constexpr float slop = 0.001f;
        constexpr float correctionFraction = 0.8f;
        const glm::vec3 correction =
            contact.normal *
            (correctionFraction * std::max(contact.penetration - slop, 0.0f) /
             invSum);
        if (invA > 0.0f)
            a.position -= correction * invA;
        if (invB > 0.0f)
            b.position += correction * invB;

        // Static bodies have zero velocity even if their stored field is
        // nonzero.
        const glm::vec3 velocityA =
            invA > 0.0f ? a.rigidbody->velocity : glm::vec3{0.0f};
        const glm::vec3 velocityB =
            invB > 0.0f ? b.rigidbody->velocity : glm::vec3{0.0f};
        const float normalSpeed =
            glm::dot(velocityB - velocityA, contact.normal);
        if (normalSpeed >= 0.0f)
            return; // Already separating: don't pull together.

        // Zero restitution: cancel approach speed without adding bounce.
        const float normalImpulse = -normalSpeed / invSum;
        const glm::vec3 impulse = contact.normal * normalImpulse;
        if (invA > 0.0f)
            a.rigidbody->velocity -= impulse * invA;
        if (invB > 0.0f)
            b.rigidbody->velocity += impulse * invB;

        // A normal impulse does not change tangential velocity, so the
        // pre-impulse relative velocity is sufficient to compute sliding.
        const glm::vec3 relativeVelocity = velocityB - velocityA;
        const glm::vec3 tangentVelocity =
            relativeVelocity - normalSpeed * contact.normal;
        const float tangentSpeedSquared = glm::dot(tangentVelocity, tangentVelocity);
        if (tangentSpeedSquared <= 1e-12f)
            return; // Avoid normalizing zero (or negligible) sliding speed.

        const float tangentSpeed = std::sqrt(tangentSpeedSquared);
        const glm::vec3 tangent = tangentVelocity / tangentSpeed;
        // One coefficient models both sticking and sliding. Clamp negative
        // inputs defensively; a zero coefficient on either collider disables it.
        const float friction = std::sqrt(std::max(a.collider.friction, 0.0f)) *
                               std::sqrt(std::max(b.collider.friction, 0.0f));
        const float tangentImpulse =
            std::min(tangentSpeed / invSum, friction * normalImpulse);
        const glm::vec3 frictionImpulse = -tangent * tangentImpulse;
        if (invA > 0.0f)
            a.rigidbody->velocity -= frictionImpulse * invA;
        if (invB > 0.0f)
            b.rigidbody->velocity += frictionImpulse * invB;
    }
} // namespace

bool se::intersect(const WorldSphere &a, const WorldSphere &b,
                   Contact &contact) {

    if (a.radius <= 0.0f || b.radius <= 0.0f)
        return false;

    const glm::vec3 delta = b.center - a.center;
    const float distanceSquared = glm::dot(delta, delta);
    const float radiusSum = a.radius + b.radius;

    if (distanceSquared >= radiusSum * radiusSum)
        return false;

    const float distance = std::sqrt(distanceSquared);

    contact.normal =
        distance > 0.0f ? delta / distance : glm::vec3{1.0f, 0.0f, 0.0f};
    contact.penetration = radiusSum - distance;

    const glm::vec3 pointA = a.center + contact.normal * a.radius;
    const glm::vec3 pointB = b.center - contact.normal * b.radius;
    contact.point = (pointA + pointB) * 0.5f;
    return true;
}

bool se::intersect(const WorldBox &a, const WorldBox &b, Contact &contact) {
    if (glm::any(glm::lessThanEqual(a.halfExtents, glm::vec3{0.0f})) ||
        glm::any(glm::lessThanEqual(b.halfExtents, glm::vec3{0.0f})))
        return false;

    const glm::vec3 delta = b.center - a.center;
    const glm::vec3 overlap = a.halfExtents + b.halfExtents - glm::abs(delta);
    if (glm::any(glm::lessThanEqual(overlap, glm::vec3{0.0f})))
        return false;

    int axis = 0;
    if (overlap.y < overlap[axis])
        axis = 1;
    if (overlap.z < overlap[axis])
        axis = 2;
    contact.normal = glm::vec3{0.0f};
    contact.normal[axis] = delta[axis] >= 0.0f ? 1.0f : -1.0f;
    contact.penetration = overlap[axis];

    const glm::vec3 overlapMin =
        glm::max(a.center - a.halfExtents, b.center - b.halfExtents);
    const glm::vec3 overlapMax =
        glm::min(a.center + a.halfExtents, b.center + b.halfExtents);
    contact.point = (overlapMin + overlapMax) * 0.5f;
    return true;
}

bool se::intersect(const WorldSphere &a, const WorldBox &b, Contact &contact) {
    if (a.radius <= 0.0f ||
        glm::any(glm::lessThanEqual(b.halfExtents, glm::vec3{0.0f})))
        return false;

    const glm::vec3 localCenter = a.center - b.center;
    const glm::vec3 closest =
        glm::clamp(localCenter, -b.halfExtents, b.halfExtents);
    const glm::vec3 delta = closest - localCenter; // Sphere toward box.
    const float distanceSquared = glm::dot(delta, delta);
    if (distanceSquared >= a.radius * a.radius)
        return false;

    if (distanceSquared > 0.0f) {
        const float distance = std::sqrt(distanceSquared);
        contact.normal = delta / distance;
        contact.penetration = a.radius - distance;
        contact.point = b.center + closest;
    } else {
        const glm::vec3 faceDistance = b.halfExtents - glm::abs(localCenter);
        int axis = 0;
        if (faceDistance.y < faceDistance[axis])
            axis = 1;
        if (faceDistance.z < faceDistance[axis])
            axis = 2;
        const float outwardSign = localCenter[axis] >= 0.0f ? 1.0f : -1.0f;
        contact.normal = glm::vec3{0.0f};
        contact.normal[axis] = -outwardSign;
        contact.penetration = a.radius + faceDistance[axis];
        contact.point = a.center;
        contact.point[axis] =
            b.center[axis] + outwardSign * b.halfExtents[axis];
    }
    return true;
}

bool se::intersect(const WorldBox &a, const WorldSphere &b, Contact &contact) {
    if (!intersect(b, a, contact))
        return false;
    contact.normal = -contact.normal;
    return true;
}

void se::SEPhysics::buildCandidatePairs(std::span<PhysicsBodyView> bodies) {
    gridEntries.clear();
    oversizedBodies.clear();
    candidatePairs.clear();
    minimumCells.resize(bodies.size());

    for (const auto index : dynamicBodies) {
        const auto &body = bodies[index];
        const glm::vec3 center = body.position + body.collider.localOffset;
        const WorldBox bounds = std::visit([&](const auto &shape) {
            return boundingBox(worldShape(shape, center));
        }, body.collider.shape);
        glm::ivec3 minimumCell, maximumCell;
        if (!findCellRange(bounds, cellSize, minimumCell, maximumCell)) {
            oversizedBodies.push_back(index);
            continue;
        }
        minimumCells[index] = minimumCell;
        for (int z = minimumCell.z; z <= maximumCell.z; ++z)
            for (int y = minimumCell.y; y <= maximumCell.y; ++y)
                for (int x = minimumCell.x; x <= maximumCell.x; ++x)
                    gridEntries.push_back({{x, y, z}, index});
    }

    std::sort(gridEntries.begin(), gridEntries.end(), [](const auto &a, const auto &b) {
        return std::tie(a.cell.x, a.cell.y, a.cell.z, a.bodyIndex) <
               std::tie(b.cell.x, b.cell.y, b.cell.z, b.bodyIndex);
    });
    const auto addPair = [&](std::size_t a, std::size_t b) {
        if (a != b) candidatePairs.emplace_back(std::min(a, b), std::max(a, b));
    };
    for (std::size_t first = 0; first < gridEntries.size();) {
        std::size_t end = first + 1;
        while (end < gridEntries.size() && gridEntries[end].cell == gridEntries[first].cell)
            ++end;
        for (std::size_t i = first; i < end; ++i) {
            for (std::size_t j = i + 1; j < end; ++j) {
                const auto a = gridEntries[i].bodyIndex;
                const auto b = gridEntries[j].bodyIndex;
                const glm::ivec3 firstSharedCell = glm::max(minimumCells[a], minimumCells[b]);
                if (gridEntries[first].cell == firstSharedCell)
                    addPair(a, b);
            }
        }
        first = end;
    }
    for (const auto dynamic : dynamicBodies)
        for (const auto fixed : staticBodies)
            addPair(dynamic, fixed);
    for (const auto large : oversizedBodies)
        for (const auto dynamic : dynamicBodies)
            addPair(large, dynamic);

    std::sort(candidatePairs.begin(), candidatePairs.end());
    candidatePairs.erase(std::unique(candidatePairs.begin(), candidatePairs.end()), candidatePairs.end());
}

void se::SEPhysics::step(std::span<PhysicsBodyView> bodies, float dt) {
    dynamicBodies.clear();
    staticBodies.clear();
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const auto *body = bodies[i].rigidbody;
        if (!body || body->isStatic) staticBodies.push_back(i);
        else dynamicBodies.push_back(i);
    }
    for (auto &body : bodies) {
        auto *rigidbody = body.rigidbody;

        if (!rigidbody || rigidbody->isStatic)
            continue;

        if (rigidbody->useGravity)
            rigidbody->velocity += gravity * dt;

        body.position += rigidbody->velocity * dt;
    }

    constexpr int solverIterations = 4;
    for (int iteration = 0; iteration < solverIterations; ++iteration) {
        buildCandidatePairs(bodies);
        for (const auto &[i, j] : candidatePairs) {
            auto &a = bodies[i];
            auto &b = bodies[j];

            // Initial scope: unit scale, axis-aligned boxes (no rotation).
            const glm::vec3 centerA = a.position + a.collider.localOffset;
            const glm::vec3 centerB = b.position + b.collider.localOffset;
            Contact contact{};
            const bool colliding = std::visit(
                [&](const auto &shapeA, const auto &shapeB) {
                    return intersect(worldShape(shapeA, centerA),
                                     worldShape(shapeB, centerB), contact);
                },
                a.collider.shape, b.collider.shape);
            if (colliding)
                resolveContact(a, b, contact);
        }
    }
}