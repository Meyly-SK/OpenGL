#pragma once
#include <string>
#include <vector>
#include <random>

// Generador/Intérprete 2D para árboles + hojas + arbustos.
// Devuelve geometría lista para OpenGL en formato intercalado:
// pos(3) + color(3) por vértice.

struct TreeParams {
    int iterations = 4;

    float baseLength = 0.05f;
    float angleBaseDeg = 25.0f;

    // Estocasticidad
    float angleJitterDeg = 8.0f;   // variación aleatoria +/- por giro
    float lengthJitter = 0.25f;    // variación aleatoria relativa en cada segmento

    // Hojas
    float leafSize = 0.015f;
    float leafProbability = 0.35f; // probabilidad de hoja al dibujar un segmento

    // Colores
    float branchR = 0.35f, branchG = 0.22f, branchB = 0.10f;
    float leafR = 0.10f, leafG = 0.85f, leafB = 0.10f;
    float bushR = 0.05f, bushG = 0.55f, bushB = 0.05f;
};

struct TreeInstance {
    float baseX = 0.0f;
    float baseY = -0.8f;
    unsigned int seed = 1;
    TreeParams params;
};

// Resultado de geometría (3 streams para dibujar por separado)
struct ForestGeometry {
    std::vector<float> branches; // GL_LINES (ramitas finas)
    std::vector<float> trunks;   // GL_TRIANGLES (tallos/ramas principales gruesas)
    std::vector<float> leaves;   // GL_TRIANGLES
    std::vector<float> bushes;   // GL_TRIANGLES
    std::vector<float> ground;   // GL_TRIANGLES (cobertura de piso)
};

class ProceduralForest {
public:
    ProceduralForest() = default;

    // Genera un árbol a partir de una sentence L-system.
    // windDeg: contribución común (mismo ritmo para todos los árboles).
    static void appendTreeGeometry(
        ForestGeometry& out,
        const std::string& sentence,
        const TreeInstance& tree,
        float windDeg);

    // Genera arbustos en la base con movimiento (offsetY común para todos).
    static void appendBushGeometry(
        ForestGeometry& out,
        const TreeInstance& tree,
        float bushOffsetY);

    // Cobertura global del piso para que se vea "más lleno" (mismo ritmo de movimiento).
    static void appendGroundCoverGeometry(
        ForestGeometry& out,
        float bushOffsetY,
        unsigned int seed = 424242u);

private:
    static float degToRad(float deg);

    static void pushVertex(std::vector<float>& v, float x, float y, float z, float r, float g, float b);

    static void pushTriangle(std::vector<float>& v,
        float x1, float y1, float x2, float y2, float x3, float y3,
        float r, float g, float b);

    // Segmento grueso como quad (2 triángulos)
    static void pushThickSegment(std::vector<float>& v,
        float x1, float y1, float x2, float y2,
        float thickness,
        float r, float g, float b);
};
