#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

// Motor de L-System (generación de cadenas). Soporta reglas estocásticas:
// para un símbolo (char) puede existir un conjunto de posibles reemplazos.
// El motor solo genera la cadena; NO sabe nada de OpenGL.
class LSystem {
public:
    using RuleSet = std::unordered_map<char, std::vector<std::string>>;

    LSystem(std::string axiom, RuleSet rules);

    // Genera una cadena a partir del axioma, aplicando "iterations".
    // Si un símbolo tiene múltiples reemplazos, elige uno al azar usando rng.
    std::string generate(int iterations, std::mt19937& rng) const;

private:
    std::string m_axiom;
    RuleSet m_rules;
};
