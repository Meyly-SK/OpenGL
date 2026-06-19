#include "LSystem.h"

/*

Un L-System (Lindenmayer System) es un sistema de reescritura:

- Tenemos una cadena inicial llamada "axioma". Ejemplo: "F"
- Tenemos reglas que reemplazan símbolos, por ejemplo:
    F -> "F[+F]F[-F]"
- Aplicamos estas reglas varias iteraciones. Cada iteración crea una cadena nueva.

En este proyecto:
- LSystem SOLO genera texto (std::string).
- La interpretación (convertir texto en geometría) ocurre en ProceduralForest.
- Se soportan reglas estocásticas: si un símbolo tiene varias opciones,
  se elige una al azar usando un RNG (std::mt19937).

*/

LSystem::LSystem(std::string axiom, RuleSet rules)
    : m_axiom(std::move(axiom)), m_rules(std::move(rules)) {}

std::string LSystem::generate(int iterations, std::mt19937& rng) const {
    // Generación iterativa:
    // Partimos del axioma y aplicamos las reglas N veces.
    // En cada iteración construimos una nueva cadena ("next") a partir de la anterior.
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

            // Si el símbolo tiene reemplazos posibles, elegimos uno.
            // Si hay más de uno, usamos RNG para elegir aleatoriamente.
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
