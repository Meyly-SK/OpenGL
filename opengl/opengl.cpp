#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <stack>
#include <random>

#include "src/Shader.h"
#include "src/LSystem.h"
#include "src/ProceduralForest.h"

 // Vista estática (sin WASD) + auto-encuadre
static float g_aspect = 1200.0f / 800.0f;

void responsive(GLFWwindow* window, int width, int height);
void userInput(GLFWwindow* window);


static LSystem makeTreeLSystem() {
    // Reglas estocรกsticas: 'F' puede expandirse de distintas maneras.
    // Esto ayuda a que cada รกrbol sea distinto incluso con mismas iteraciones.
    LSystem::RuleSet rules;
    rules['F'] = {
        "F[+F]F[-F][F]",
        "F[+F]F",
        "F[-F]F",
        "FF"
    };

    return LSystem("F", std::move(rules));
}

static std::vector<TreeInstance> makeForestInstances() {
    std::vector<TreeInstance> trees;
    trees.reserve(8);

    // Diseño: 2 filas (fondo y frente) para dar sensación de bosque.
    // Fondo: más pequeño, más oscuro, un poco más arriba.
    // Frente: más grande, colores más vivos.
    const float xs[8] = { -0.90f, -0.62f, -0.35f, -0.10f, 0.15f, 0.38f, 0.62f, 0.88f };

    for (int i = 0; i < 8; i++) {
        TreeInstance t;
        bool backRow = (i % 2 == 0);

        t.baseX = xs[i] + (backRow ? -0.03f : 0.03f); // leve jitter para que no sea "perfecto"
        t.baseY = backRow ? -0.75f : -0.85f;
        t.seed = 1337u + static_cast<unsigned int>(i) * 101u;

        // Variar parámetros por árbol (parametrizado)
        t.params.iterations = backRow ? (3 + (i % 2)) : (4 + (i % 2)); // fondo 3..4, frente 4..5

        // Aumentamos un poco el tamaño de la fila de fondo para "alargar" los mini árboles
        float sizeScale = backRow ? 0.95f : 1.15f;

        t.params.baseLength = sizeScale * (0.035f + 0.006f * (i % 4));
        t.params.angleBaseDeg = 18.0f + 3.0f * (i % 6);
        t.params.angleJitterDeg = 6.0f + 1.5f * (i % 5);

        // Más frondoso arriba (aumentamos probabilidad de hoja global)
        t.params.leafProbability = (backRow ? 0.35f : 0.45f) + 0.03f * (i % 3);
        t.params.leafSize = sizeScale * (0.010f + 0.0025f * (i % 5));

        // Colores por fila
        if (backRow) {
            // Fondo (lejos): menos saturación + más azulado (atmósfera)
            t.params.branchR = 0.26f; t.params.branchG = 0.20f; t.params.branchB = 0.14f;
            t.params.leafR = 0.14f;  t.params.leafG = 0.60f;  t.params.leafB = 0.20f;
            t.params.bushR = 0.05f;  t.params.bushG = 0.34f;  t.params.bushB = 0.10f;
        } else {
            // Frente (cerca): más vivo y contrastado
            t.params.branchR = 0.42f; t.params.branchG = 0.28f; t.params.branchB = 0.13f;
            t.params.leafR = 0.10f;  t.params.leafG = 0.85f;  t.params.leafB = 0.10f;
            t.params.bushR = 0.05f;  t.params.bushG = 0.55f;  t.params.bushB = 0.05f;
        }

        trees.push_back(t);
    }

    return trees;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 800, "L-System Fractal", NULL, NULL);
    if (!window) { std::cout << "Error\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, responsive);
    if (glewInit() != GLEW_OK) { std::cout << "Error Glew\n"; glfwTerminate(); return -1; }

    Shader myShader("res/Shader/vertexShader.glsl", "res/Shader/fragmentShader.glsl");

    LSystem treeSystem = makeTreeLSystem();
    std::vector<TreeInstance> forest = makeForestInstances();

    // Pre-generamos las sentences por รกrbol (motor separado del render)
    std::vector<std::string> sentences;
    sentences.reserve(forest.size());
    for (const auto& t : forest) {
        std::mt19937 rng(t.seed);
        sentences.push_back(treeSystem.generate(t.params.iterations, rng));
    }

    // Background (cielo + terreno)
    unsigned int bgVBO, bgVAO;

    // 5 streams: ramas finas (lines), tallos gruesos (triangles), hojas (triangles), arbustos (triangles), piso (triangles)
    unsigned int branchVBO, branchVAO;
    unsigned int trunkVBO, trunkVAO;
    unsigned int leafVBO, leafVAO;
    unsigned int bushVBO, bushVAO;
    unsigned int groundVBO, groundVAO;

    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);

    glGenVertexArrays(1, &branchVAO);
    glGenBuffers(1, &branchVBO);

    glGenVertexArrays(1, &trunkVAO);
    glGenBuffers(1, &trunkVBO);

    glGenVertexArrays(1, &leafVAO);
    glGenBuffers(1, &leafVBO);

    glGenVertexArrays(1, &bushVAO);
    glGenBuffers(1, &bushVBO);

    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);

    auto setupVAO = [](unsigned int vao, unsigned int vbo) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    };

    setupVAO(bgVAO, bgVBO);
    setupVAO(branchVAO, branchVBO);
    setupVAO(trunkVAO, trunkVBO);
    setupVAO(leafVAO, leafVBO);
    setupVAO(bushVAO, bushVBO);
    setupVAO(groundVAO, groundVBO);

    // Cargar geometría del background (cielo degradado + terreno verdoso).
    // Se dibuja en coordenadas NDC; luego usamos uniforms neutrales (uCenter=0, uScale=1).
    std::vector<float> bgVerts;
    bgVerts.reserve(12 * 6);

    auto pushBg = [&](float x, float y, float r, float g, float b) {
        bgVerts.push_back(x); bgVerts.push_back(y); bgVerts.push_back(0.0f);
        bgVerts.push_back(r); bgVerts.push_back(g); bgVerts.push_back(b);
    };

    auto pushBgTri = [&](
        float x1, float y1,
        float x2, float y2,
        float x3, float y3,
        float r, float g, float b) {
        pushBg(x1, y1, r, g, b);
        pushBg(x2, y2, r, g, b);
        pushBg(x3, y3, r, g, b);
    };

    auto pushCircleFan = [&](
        float cx, float cy, float radius,
        int segments,
        float r, float g, float b) {

        if (segments < 6) segments = 6;

        for (int i = 0; i < segments; i++) {
            float a0 = (static_cast<float>(i) / static_cast<float>(segments)) * 6.2831853f;
            float a1 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * 6.2831853f;

            float x0 = cx + std::cos(a0) * radius;
            float y0 = cy + std::sin(a0) * radius;
            float x1 = cx + std::cos(a1) * radius;
            float y1 = cy + std::sin(a1) * radius;

            pushBgTri(cx, cy, x0, y0, x1, y1, r, g, b);
        }
    };

    // SKY quad: y from -0.05..1.2 (horizonte -> arriba)
    // Degradado: arriba más claro, horizonte más oscuro.
    float skyTopY = 1.2f;
    float horizonY = -0.55f;
    float skyBotY = horizonY;
    float skyTopR = 0.45f, skyTopG = 0.70f, skyTopB = 0.95f;
    float skyBotR = 0.18f, skyBotG = 0.35f, skyBotB = 0.70f;

    // Tri 1
    pushBg(-1.2f, skyTopY, skyTopR, skyTopG, skyTopB);
    pushBg(-1.2f, skyBotY, skyBotR, skyBotG, skyBotB);
    pushBg(1.2f, skyBotY, skyBotR, skyBotG, skyBotB);
    // Tri 2
    pushBg(-1.2f, skyTopY, skyTopR, skyTopG, skyTopB);
    pushBg(1.2f, skyBotY, skyBotR, skyBotG, skyBotB);
    pushBg(1.2f, skyTopY, skyTopR, skyTopG, skyTopB);

    // Montañas: 2 capas de siluetas para dar profundidad.
    // Usamos RNG fijo para que no parpadeen (determinista).
    std::mt19937 bgRng(2026u);
    auto bgRand01 = [&]() -> float {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(bgRng);
    };

    // Capa lejana (más clara, azulada, cerca del horizonte)
    {
        float baseY = horizonY - 0.02f;
        float minH = 0.10f;
        float maxH = 0.22f;

        float mr = 0.22f, mg = 0.32f, mb = 0.48f;

        int peaks = 6;
        float x0 = -1.25f;
        float x1 = 1.25f;
        float step = (x1 - x0) / static_cast<float>(peaks);

        for (int i = 0; i < peaks; i++) {
            float left = x0 + step * i;
            float right = left + step * (0.95f + 0.15f * bgRand01());
            float mid = (left + right) * 0.5f + (bgRand01() * 2.0f - 1.0f) * step * 0.15f;

            float h = minH + (maxH - minH) * bgRand01();
            float topY = baseY + h;

            pushBgTri(left, baseY, right, baseY, mid, topY, mr, mg, mb);
        }
    }

    // Capa media (más oscura, más baja)
    {
        float baseY = horizonY - 0.12f;
        float minH = 0.14f;
        float maxH = 0.30f;

        float mr = 0.10f, mg = 0.18f, mb = 0.20f;

        int peaks = 7;
        float x0 = -1.25f;
        float x1 = 1.25f;
        float step = (x1 - x0) / static_cast<float>(peaks);

        for (int i = 0; i < peaks; i++) {
            float left = x0 + step * i;
            float right = left + step * (0.90f + 0.20f * bgRand01());
            float mid = (left + right) * 0.5f + (bgRand01() * 2.0f - 1.0f) * step * 0.18f;

            float h = minH + (maxH - minH) * bgRand01();
            float topY = baseY + h;

            pushBgTri(left, baseY, right, baseY, mid, topY, mr, mg, mb);
        }
    }

    // GROUND quad: y from -1.2..-0.80
    // Degradado: más claro arriba (cerca del horizonte) y más oscuro abajo.
    float groundTopY = horizonY;
    float groundBotY = -1.2f;
    float groundTopR = 0.14f, groundTopG = 0.35f, groundTopB = 0.16f;
    float groundBotR = 0.06f, groundBotG = 0.20f, groundBotB = 0.08f;

    // Tri 1
    pushBg(-1.2f, groundTopY, groundTopR, groundTopG, groundTopB);
    pushBg(-1.2f, groundBotY, groundBotR, groundBotG, groundBotB);
    pushBg(1.2f, groundBotY, groundBotR, groundBotG, groundBotB);
    // Tri 2
    pushBg(-1.2f, groundTopY, groundTopR, groundTopG, groundTopB);
    pushBg(1.2f, groundBotY, groundBotR, groundBotG, groundBotB);
    pushBg(1.2f, groundTopY, groundTopR, groundTopG, groundTopB);

    // Nubes: círculos aproximados con triángulos (fan).
    // Colores claros con leve azul para integrarse con el cielo.
    {
        float cr = 0.90f, cg = 0.94f, cb = 0.98f;
        float cr2 = 0.82f, cg2 = 0.88f, cb2 = 0.96f;

        // Nube 1
        pushCircleFan(-0.65f, 0.75f, 0.10f, 14, cr, cg, cb);
        pushCircleFan(-0.55f, 0.78f, 0.12f, 14, cr, cg, cb);
        pushCircleFan(-0.45f, 0.74f, 0.09f, 14, cr2, cg2, cb2);

        // Nube 2
        pushCircleFan(0.35f, 0.88f, 0.08f, 14, cr, cg, cb);
        pushCircleFan(0.45f, 0.90f, 0.11f, 14, cr, cg, cb);
        pushCircleFan(0.55f, 0.86f, 0.09f, 14, cr2, cg2, cb2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, bgVerts.size() * sizeof(float), bgVerts.data(), GL_STATIC_DRAW);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();

        userInput(window);

        // Movimiento comรบn (mismo ritmo) para todos
        // Viento más visible (grados). La base se mantiene fija por el filtro por profundidad.
        float windDeg = std::sin(currentFrame * 1.5f) * 10.0f;

        // Base fija (sin "bouncing"): piso/pasto/arbustos no deben moverse arriba/abajo.
        float bushOffsetY = 0.0f;

        ForestGeometry geom;

        for (size_t i = 0; i < forest.size(); i++) {
            ProceduralForest::appendTreeGeometry(geom, sentences[i], forest[i], windDeg);
            ProceduralForest::appendBushGeometry(geom, forest[i], bushOffsetY);
        }
        ProceduralForest::appendGroundCoverGeometry(geom, bushOffsetY);

        // Subir a GPU (dinรกmico por viento + arbustos)
        glBindBuffer(GL_ARRAY_BUFFER, branchVBO);
        glBufferData(GL_ARRAY_BUFFER, geom.branches.size() * sizeof(float), geom.branches.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, trunkVBO);
        glBufferData(GL_ARRAY_BUFFER, geom.trunks.size() * sizeof(float), geom.trunks.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, leafVBO);
        glBufferData(GL_ARRAY_BUFFER, geom.leaves.size() * sizeof(float), geom.leaves.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, bushVBO);
        glBufferData(GL_ARRAY_BUFFER, geom.bushes.size() * sizeof(float), geom.bushes.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
        glBufferData(GL_ARRAY_BUFFER, geom.ground.size() * sizeof(float), geom.ground.data(), GL_DYNAMIC_DRAW);

        // Auto-encuadre: calcular bounding box en coordenadas mundo
        auto updateBounds = [](const std::vector<float>& v, float& minX, float& maxX, float& minY, float& maxY) {
            for (size_t i = 0; i + 5 < v.size(); i += 6) {
                float x = v[i + 0];
                float y = v[i + 1];
                minX = (x < minX) ? x : minX;
                maxX = (x > maxX) ? x : maxX;
                minY = (y < minY) ? y : minY;
                maxY = (y > maxY) ? y : maxY;
            }
        };

        float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
        updateBounds(geom.branches, minX, maxX, minY, maxY);
        updateBounds(geom.trunks, minX, maxX, minY, maxY);
        updateBounds(geom.leaves, minX, maxX, minY, maxY);
        updateBounds(geom.bushes, minX, maxX, minY, maxY);
        updateBounds(geom.ground, minX, maxX, minY, maxY);

        // Evitar división por cero
        float width = (maxX - minX);
        float height = (maxY - minY);
        if (width < 1e-5f) width = 1e-5f;
        if (height < 1e-5f) height = 1e-5f;

        // Centro del bounding box
        float centerX = (minX + maxX) * 0.5f;
        float centerY = (minY + maxY) * 0.5f;

        // Escala para encajar con margen (0.9)
        float scaleX = 0.9f * (2.0f / width);
        float scaleY = 0.9f * (2.0f / height);
        float scale = (scaleX < scaleY) ? scaleX : scaleY;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        myShader.use();

        // Background: uniforms neutrales (NDC)
        myShader.setFloat("uAspect", g_aspect);
        myShader.setFloat("uScale", 1.0f);
        myShader.setVec2("uCenter", 0.0f, 0.0f);

        glBindVertexArray(bgVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(bgVerts.size() / 6));

        // Bosque: uniforms auto-encuadre
        myShader.setFloat("uAspect", g_aspect);
        myShader.setFloat("uScale", scale);
        myShader.setVec2("uCenter", centerX, centerY);

        // Piso animado (pasto)
        glBindVertexArray(groundVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(geom.ground.size() / 6));

        // Tallos gruesos
        glBindVertexArray(trunkVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(geom.trunks.size() / 6));

        // Ramas finas
        glBindVertexArray(branchVAO);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(geom.branches.size() / 6));

        // Hojas
        glBindVertexArray(leafVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(geom.leaves.size() / 6));

        // Arbustos
        glBindVertexArray(bushVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(geom.bushes.size() / 6));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}

void responsive(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (height == 0) height = 1;
    g_aspect = static_cast<float>(width) / static_cast<float>(height);
}

void userInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}
