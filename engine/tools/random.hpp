#pragma once
#include <random>
#include "glm/fwd.hpp"

class Random {
   public:
    static int rand_range(int min, int max) {
        static thread_local std::mt19937 rng { std::random_device {}() };
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng);
    }

    static float rand_range(float min, float max) {
        static thread_local std::mt19937 rng { std::random_device {}() };
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }

    static glm::vec3 rand_unit_vec()
    {
        static thread_local std::mt19937 rng { std::random_device {}() };
        static thread_local std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        const float z = dist(rng) * 2.0f - 1.0f;
        const float a = dist(rng) * 2.0f * glm::pi<float>();

        const float r = std::sqrt(1.0f - z * z);

        return glm::vec3(r * std::cos(a), r * std::sin(a), z);
    }
};