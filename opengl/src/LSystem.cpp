#include "LSystem.h"

LSystem::LSystem(std::string axiom, RuleSet rules)
    : m_axiom(std::move(axiom)), m_rules(std::move(rules)) {}

std::string LSystem::generate(int iterations, std::mt19937& rng) const {
    std::string current = m_axiom;

    for (int i = 0; i < iterations; i++) {
        std::string next;
        next.reserve(current.size() * 2);

        for (char c : current) {
            auto it = m_rules.find(c);
            if (it == m_rules.end() || it->second.empty()) {
                next.push_back(c);
                continue;
            }

            const auto& choices = it->second;
            if (choices.size() == 1) {
                next += choices[0];
            } else {
                std::uniform_int_distribution<size_t> dist(0, choices.size() - 1);
                next += choices[dist(rng)];
            }
        }

        current = std::move(next);
    }

    return current;
}
