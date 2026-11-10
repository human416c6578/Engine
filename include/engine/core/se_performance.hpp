#pragma once

#include <cstddef>
#include <cstdint>

namespace se {
    struct PerformanceStats {
        double frameTimeMs = 0.0;
        double physicsTimeMsThisFrame = 0.0;
        double lastPhysicsStepMs = 0.0;
        double averagePhysicsStepMs = 0.0;
        double maxPhysicsStepMs = 0.0;
        std::uint64_t measuredSteps = 0;
        std::size_t physicsStepsThisFrame = 0;
        std::size_t colliderCount = 0;
        std::size_t dynamicBodyCount = 0;
        bool hasCompletedRenderSample = false;
        double completedFramePhysicsMs = 0.0;
        double renderBeginMs = 0.0;
        double renderRecordMs = 0.0;
        double renderSubmitPresentMs = 0.0;
    };

    class PerformanceMonitor {
    public:
        PerformanceMonitor() = default;
        PerformanceMonitor(const PerformanceMonitor&) = delete;
        PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
        PerformanceMonitor(PerformanceMonitor&&) = delete;
        PerformanceMonitor& operator=(PerformanceMonitor&&) = delete;

        const PerformanceStats& getStats() const noexcept { return stats; }
        void reset() noexcept { stats = {}; }

        void beginFrame(float frameTime, std::size_t colliders, std::size_t dynamicBodies) noexcept {
            stats.frameTimeMs = static_cast<double>(frameTime) * 1000.0;
            stats.physicsTimeMsThisFrame = 0.0;
            stats.physicsStepsThisFrame = 0;
            stats.colliderCount = colliders;
            stats.dynamicBodyCount = dynamicBodies;
        }

        void recordPhysicsStep(double elapsedMs) noexcept {
            stats.lastPhysicsStepMs = elapsedMs;
            stats.physicsTimeMsThisFrame += elapsedMs;
            ++stats.physicsStepsThisFrame;
            ++stats.measuredSteps;
            stats.averagePhysicsStepMs += (elapsedMs - stats.averagePhysicsStepMs) /
                                          static_cast<double>(stats.measuredSteps);
            if (elapsedMs > stats.maxPhysicsStepMs) stats.maxPhysicsStepMs = elapsedMs;
        }

        void recordRenderTimings(double beginMs, double recordMs, double submitPresentMs) noexcept {
            stats.completedFramePhysicsMs = stats.physicsTimeMsThisFrame;
            stats.renderBeginMs = beginMs;
            stats.renderRecordMs = recordMs;
            stats.renderSubmitPresentMs = submitPresentMs;
            stats.hasCompletedRenderSample = true;
        }

    private:
        PerformanceStats stats{};
    };
}
