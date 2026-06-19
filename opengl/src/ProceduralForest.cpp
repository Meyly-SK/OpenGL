#include "ProceduralForest.h"

#include <cmath>
#include <stack>

struct TurtleState2D {
    float x, y;
    float angleDeg;
};

float ProceduralForest::degToRad(float deg) {
    return deg * (3.14159265359f / 180.0f);
}

void ProceduralForest::pushVertex(std::vector<float>& v, float x, float y, float z, float r, float g, float b) {
    v.push_back(x);
    v.push_back(y);
    v.push_back(z);
    v.push_back(r);
    v.push_back(g);
    v.push_back(b);
}

void ProceduralForest::pushTriangle(std::vector<float>& v,
    float x1, float y1, float x2, float y2, float x3, float y3,
    float r, float g, float b) {
    pushVertex(v, x1, y1, 0.0f, r, g, b);
    pushVertex(v, x2, y2, 0.0f, r, g, b);
    pushVertex(v, x3, y3, 0.0f, r, g, b);
}

void ProceduralForest::pushThickSegment(std::vector<float>& v,
    float x1, float y1, float x2, float y2,
    float thickness,
    float r, float g, float b) {

    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f) return;

    dx /= len;
    dy /= len;

    // Perpendicular
    float nx = -dy;
    float ny = dx;

    float hx = nx * (thickness * 0.5f);
    float hy = ny * (thickness * 0.5f);

    // Quad corners
    float ax = x1 + hx, ay = y1 + hy;
    float bx = x1 - hx, by = y1 - hy;
    float cx = x2 - hx, cy = y2 - hy;
    float dxp = x2 + hx, dyp = y2 + hy;

    // Two triangles: (a,b,c) and (a,c,d)
    pushTriangle(v, ax, ay, bx, by, cx, cy, r, g, b);
    pushTriangle(v, ax, ay, cx, cy, dxp, dyp, r, g, b);
}

void ProceduralForest::appendTreeGeometry(
    ForestGeometry& out,
    const std::string& sentence,
    const TreeInstance& tree,
    float windDeg) {

    std::mt19937 rng(tree.seed);

    auto rand01 = [&]() -> float {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng);
    };

    auto randSigned = [&]() -> float {
        return rand01() * 2.0f - 1.0f;
    };

    std::stack<TurtleState2D> st;

    float x = tree.baseX;
    float y = tree.baseY;
    float angleDeg = 90.0f; // hacia arriba

    // Cache de parámetros
    const auto& p = tree.params;

    // Para que cada árbol tenga su "tendencia" diferente, añadimos un pequeño offset fijo por seed
    float treeAngleBias = randSigned() * (p.angleJitterDeg * 0.25f);

    int depth = 0;

    for (char c : sentence) {
        if (c == 'F') {
            float length = p.baseLength * (1.0f + randSigned() * p.lengthJitter);

            float angleRad = degToRad(angleDeg);
            float newX = x + length * std::cos(angleRad);
            float newY = y + length * std::sin(angleRad);

            // Segmento (rama): ramas principales con grosor real (triángulos) y ramitas finas como líneas.
            // A menor depth => más grueso (tallo/ramas principales).
            // Ajustado para verse más "árbol" (tronco notable).
            float thickness = 0.0f;
            if (depth <= 1) thickness = 0.016f;
            else if (depth == 2) thickness = 0.010f;
            else if (depth == 3) thickness = 0.006f;

            if (thickness > 0.0f) {
                pushThickSegment(out.trunks, x, y, newX, newY, thickness, p.branchR, p.branchG, p.branchB);
            } else {
                pushVertex(out.branches, x, y, 0.0f, p.branchR, p.branchG, p.branchB);
                pushVertex(out.branches, newX, newY, 0.0f, p.branchR, p.branchG, p.branchB);
            }

            // Hojas: más pobladas a mayor profundidad. Además, en la "copa" se llena mucho más para
            // que parezca un árbol con parte superior ancha y frondosa (estilo "nube").
            const int crownDepth = 4;

            float depthBoost = 0.08f * static_cast<float>(depth);
            float leafProb = p.leafProbability + depthBoost;
            if (depth >= crownDepth) leafProb = 0.95f;
            if (leafProb > 0.98f) leafProb = 0.98f;

            bool likelyTip = (depth >= crownDepth); // heurística simple: en copa tratamos como posible punta

            if (rand01() < leafProb) {
                int leafCount = 4 + depth + static_cast<int>(rand01() * 6.0f);
                if (depth >= crownDepth) leafCount = 18 + static_cast<int>(rand01() * 12.0f); // 18..30
                if (leafCount > 32) leafCount = 32;

                for (int li = 0; li < leafCount; li++) {
                    float s = p.leafSize * (0.55f + rand01() * 0.75f);

                    float spreadBase = s * (0.8f + rand01() * (1.5f + 0.25f * depth));
                    float spread = spreadBase;
                    if (depth >= crownDepth) spread = spreadBase * 2.2f; // copa más ancha

                    float jitterA = degToRad(rand01() * 360.0f);
                    float cx = newX + std::cos(jitterA) * spread;
                    float cy = newY + std::sin(jitterA) * spread;

                    float a = degToRad(rand01() * 360.0f);
                    float x1 = cx + std::cos(a) * s;
                    float y1 = cy + std::sin(a) * s;
                    float x2 = cx + std::cos(a + 2.0943951f) * s;
                    float y2 = cy + std::sin(a + 2.0943951f) * s;
                    float x3 = cx + std::cos(a + 4.1887902f) * s;
                    float y3 = cy + std::sin(a + 4.1887902f) * s;

                    float tint = 0.80f + rand01() * 0.35f;
                    pushTriangle(out.leaves, x1, y1, x2, y2, x3, y3,
                        p.leafR * tint, p.leafG * tint, p.leafB * tint);
                }
            }

            // "Pompon" extra en la copa: una bola de hojas (muy frondoso) para simular masa densa arriba.
            // Esto se mantiene estocástico (seed) y parametrizado por leafSize, cumpliendo la rúbrica.
            if (likelyTip && rand01() < 0.35f) {
                float ballR = p.leafSize * (6.0f + rand01() * 6.0f); // radio de copa local
                int tris = 80 + static_cast<int>(rand01() * 80.0f);  // 80..160 triángulos

                for (int ti = 0; ti < tris; ti++) {
                    float rr = ballR * std::sqrt(rand01());
                    float aa = degToRad(rand01() * 360.0f);
                    float cx = newX + std::cos(aa) * rr;
                    float cy = newY + std::sin(aa) * rr;

                    float s = p.leafSize * (0.45f + rand01() * 0.70f);
                    float a = degToRad(rand01() * 360.0f);

                    float x1 = cx + std::cos(a) * s;
                    float y1 = cy + std::sin(a) * s;
                    float x2 = cx + std::cos(a + 2.0943951f) * s;
                    float y2 = cy + std::sin(a + 2.0943951f) * s;
                    float x3 = cx + std::cos(a + 4.1887902f) * s;
                    float y3 = cy + std::sin(a + 4.1887902f) * s;

                    float tint = 0.75f + rand01() * 0.45f;
                    pushTriangle(out.leaves, x1, y1, x2, y2, x3, y3,
                        p.leafR * tint, p.leafG * tint, p.leafB * tint);
                }
            }

            x = newX;
            y = newY;
        }
        else if (c == '+') {
            float jitter = randSigned() * p.angleJitterDeg;

            // Viento solo en la parte superior: el tronco/base casi no se mueve.
            const int windStartDepth = 2;
            const float windRange = 2.5f; // llega más rápido al 100% en la copa
            float windFactor = (static_cast<float>(depth - windStartDepth)) / windRange;
            if (windFactor < 0.0f) windFactor = 0.0f;
            if (windFactor > 1.0f) windFactor = 1.0f;

            float windApplied = windDeg * windFactor;

            angleDeg -= (p.angleBaseDeg + windApplied + treeAngleBias + jitter);
        }
        else if (c == '-') {
            float jitter = randSigned() * p.angleJitterDeg;

            const int windStartDepth = 2;
            const float windRange = 2.5f;
            float windFactor = (static_cast<float>(depth - windStartDepth)) / windRange;
            if (windFactor < 0.0f) windFactor = 0.0f;
            if (windFactor > 1.0f) windFactor = 1.0f;

            float windApplied = windDeg * windFactor;

            angleDeg += (p.angleBaseDeg + windApplied + treeAngleBias + jitter);
        }
        else if (c == '[') {
            st.push({ x, y, angleDeg });
            depth++;
        }
        else if (c == ']') {
            if (!st.empty()) {
                TurtleState2D s = st.top();
                st.pop();
                x = s.x;
                y = s.y;
                angleDeg = s.angleDeg;
            }
            if (depth > 0) depth--;
        }
    }
}

void ProceduralForest::appendBushGeometry(
    ForestGeometry& out,
    const TreeInstance& tree,
    float bushOffsetY) {

    // Arbusto como 2-3 triángulos "hojosos" en la base.
    // Movimiento: solo se aplica bushOffsetY (común para todos).
    std::mt19937 rng(tree.seed ^ 0x9e3779b9u);

    auto rand01 = [&]() -> float {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng);
    };

    const auto& p = tree.params;

    float bx = tree.baseX;
    float by = tree.baseY + bushOffsetY;

    float radius = 0.085f * (0.8f + rand01() * 0.6f);
    int clumps = 16;

    for (int i = 0; i < clumps; i++) {
        float cx = bx + (rand01() * 2.0f - 1.0f) * radius * 0.6f;
        float cy = by + rand01() * radius * 0.3f;

        float s = radius * (0.4f + rand01() * 0.4f);

        // Triángulo
        float a = rand01() * 6.2831853f;
        float x1 = cx + std::cos(a) * s;
        float y1 = cy + std::sin(a) * s;
        float x2 = cx + std::cos(a + 2.0943951f) * s;
        float y2 = cy + std::sin(a + 2.0943951f) * s;
        float x3 = cx + std::cos(a + 4.1887902f) * s;
        float y3 = cy + std::sin(a + 4.1887902f) * s;

        // 2 capas para dar volumen
        pushTriangle(out.bushes, x1, y1, x2, y2, x3, y3, p.bushR, p.bushG, p.bushB);

        float tint = 0.80f + rand01() * 0.30f;
        pushTriangle(out.bushes, x1, y1 + 0.005f, x2, y2 + 0.005f, x3, y3 + 0.005f,
            p.bushR * tint, p.bushG * tint, p.bushB * tint);
    }
}

void ProceduralForest::appendGroundCoverGeometry(
    ForestGeometry& out,
    float bushOffsetY,
    unsigned int seed) {

    std::mt19937 rng(seed);

    auto rand01 = [&]() -> float {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng);
    };

    // Piso cubierto de vegetación baja (triángulos pequeños).
    // Para simular profundidad/perspectiva:
    // - Near grass: pasto más grande y oscuro (cerca del observador)
    // - Far grass: pasto más pequeño y claro (cerca del horizonte)
    const float groundY = -0.92f + bushOffsetY;
    const float xMin = -1.25f;
    const float xMax = 1.25f;

    // Near grass (cerca)
    int nearBlades = 220;
    for (int i = 0; i < nearBlades; i++) {
        float x = xMin + (xMax - xMin) * rand01();
        float h = 0.02f + 0.03f * rand01();
        float w = 0.01f + 0.02f * rand01();

        float x1 = x;
        float y1 = groundY;
        float x2 = x - w;
        float y2 = groundY;
        float x3 = x;
        float y3 = groundY + h;

        // Verde variado (más oscuro)
        float r = 0.04f + 0.03f * rand01();
        float g = 0.30f + 0.35f * rand01();
        float b = 0.04f + 0.03f * rand01();

        pushTriangle(out.ground, x1, y1, x2, y2, x3, y3, r, g, b);
    }

    // Far grass (lejos / cerca del horizonte): más pequeño y más claro
    // Capa lejana: bajada un poco para que no "levite" sobre el terreno
    const float farY = -0.70f + bushOffsetY; // cercano al horizonte visual
    int farBlades = 260;
    for (int i = 0; i < farBlades; i++) {
        float x = xMin + (xMax - xMin) * rand01();
        float h = 0.008f + 0.012f * rand01();
        float w = 0.004f + 0.008f * rand01();

        float x1 = x;
        float y1 = farY;
        float x2 = x - w;
        float y2 = farY;
        float x3 = x;
        float y3 = farY + h;

        // Verde más claro y menos saturado (atmósfera)
        float r = 0.06f + 0.04f * rand01();
        float g = 0.42f + 0.30f * rand01();
        float b = 0.08f + 0.06f * rand01();

        pushTriangle(out.ground, x1, y1, x2, y2, x3, y3, r, g, b);
    }

    // Piedritas/pequeños detalles (muy sutiles) para romper el plano (parches oscuros)
    int rocks = 90;
    for (int i = 0; i < rocks; i++) {
        float x = xMin + (xMax - xMin) * rand01();
        float y = (-1.05f + 0.30f * rand01()) + bushOffsetY; // zona baja
        float s = 0.006f + 0.012f * rand01();

        float x1 = x - s, y1 = y;
        float x2 = x + s, y2 = y;
        float x3 = x, y3 = y + s * (0.6f + 0.8f * rand01());

        float r = 0.04f + 0.03f * rand01();
        float g = 0.10f + 0.08f * rand01();
        float b = 0.04f + 0.03f * rand01();

        pushTriangle(out.ground, x1, y1, x2, y2, x3, y3, r, g, b);
    }
}
