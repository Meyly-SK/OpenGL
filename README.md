# OpenGL – Bosque procedural con L-System (guía para estudiantes)

Este proyecto dibuja un **bosque 2D procedural** usando OpenGL (core 3.3).  
La escena se genera combinando dos etapas separadas:

1) **Generación (modelo):** un **L-System** crea una *cadena* (string) con símbolos como `F`, `+`, `-`, `[`, `]`.  
2) **Interpretación / Render (geometría):** una “tortuga” 2D recorre esa cadena y la convierte en **vértices** (pos+color) para dibujar troncos, ramas, hojas, arbustos y pasto.

La idea principal es que el L-System **no dibuja**: solo produce texto.  
El dibujado ocurre cuando esa cadena se interpreta y se sube a la GPU.

---

## Estructura del proyecto (archivos clave)

### `opengl/opengl.cpp` (main)
Es el **orquestador** de todo el programa:

- Inicializa **GLFW** (ventana + contexto OpenGL) y **GLEW** (carga de funciones OpenGL).
- Crea el **Shader** (programa de GPU) a partir de:
  - `res/Shader/vertexShader.glsl`
  - `res/Shader/fragmentShader.glsl`
- Configura el **L-System** (reglas para `F`) y crea instancias del bosque (`TreeInstance`).
- Pre-genera la **sentence** por árbol (string) con semilla (`seed`) y un número de iteraciones.
- Crea buffers VAO/VBO para varios “streams” de geometría.
- Genera la geometría fija del **background** (cielo, montañas, nubes).
- En cada frame (loop):
  1) Calcula un viento `windDeg` (senoidal).
  2) Construye toda la geometría del frame (árboles + arbustos + pasto).
  3) Sube esa geometría a la GPU (`glBufferData`).
  4) Calcula un **auto-encuadre** (bounding box → centro + escala).
  5) Dibuja por capas: fondo → piso → troncos → ramas → hojas → arbustos.

---

### `opengl/src/LSystem.h` y `opengl/src/LSystem.cpp`
**Motor de reescritura** del L-System.

- Tiene un **axioma** (string inicial).
- Tiene reglas de reemplazo por símbolo, por ejemplo para `F`.
- Soporta reglas **estocásticas**: para un símbolo puede haber múltiples reemplazos posibles y se elige uno al azar con un `std::mt19937`.

Salida: una **string** (sentence).  
Importante: esta clase *no* sabe de OpenGL ni de geometría.

---

### `opengl/src/ProceduralForest.h` y `opengl/src/ProceduralForest.cpp`
Interpretador de la sentence con una **tortuga 2D**.

- Lee símbolos:
  - `F`: avanza y dibuja un segmento.
  - `+` y `-`: rota (con jitter aleatorio y viento en la copa).
  - `[` y `]`: guarda/restaura estado (posición/ángulo) usando un `stack` (ramificación).
- Genera geometría intercalada: por vértice guarda:
  - `pos(x,y,z) + color(r,g,b)` (6 floats)

Devuelve un `ForestGeometry` con 5 streams:
- `branches` → líneas (GL_LINES): ramitas finas
- `trunks` → triángulos (GL_TRIANGLES): tronco/ramas principales con grosor (quad = 2 tris)
- `leaves` → triángulos: hojas (triángulos verdes)
- `bushes` → triángulos: arbustos base
- `ground` → triángulos: pasto/cobertura

#### Sobre la copa (parte superior)
Para evitar árboles “pelados” arriba, en la copa se agregan:
- **ramitas laterales** (solo en `depth` alto) para crear más “tips”,
- **mini-clusters de hojas** en los extremos de esas ramitas,
- y un **“pompon”** extra (bola de hojas) ocasional para dar masa.

Todo esto se aplica **solo en la copa**, para no ensuciar el tronco.

---

### `opengl/src/Shader.h` y `opengl/src/Shader.cpp`
Wrapper simple de shaders:

- Lee archivos `.glsl`.
- Compila vertex y fragment shader.
- Linkea un programa OpenGL.
- Expone `use()` y helpers para uniforms (`setFloat`, `setVec2`, etc.).

---

## Shaders

- `res/Shader/vertexShader.glsl`
- `res/Shader/fragmentShader.glsl`

En general el pipeline es:
- Vértices con atributos: posición y color.
- Uniforms típicos usados por este proyecto:
  - `uAspect`: corregir aspect ratio.
  - `uCenter`: centro del encuadre (auto-encuadre).
  - `uScale`: escala para encajar la escena en pantalla.

---

## Cómo “funciona” el render (resumen mental rápido)

1) L-System genera una cadena: `F[+F]F[-F]...`
2) ProceduralForest interpreta esa cadena:
   - cada `F` produce geometría,
   - cada `[` / `]` crea ramificación,
   - el `depth` controla grosor, densidad y comportamiento en copa.
3) El main sube la geometría a VBOs y dibuja por capas.

---

## Controles

- `ESC`: cerrar la ventana.

---

## Notas de compilación

- Proyecto pensado para compilarse con Visual Studio (MSVC) y sus configuraciones de OpenGL/GLFW/GLEW.
- Si en VS Code aparecen errores de includes (IntelliSense), puede ser solo configuración del includePath; Visual Studio puede compilar igual si el proyecto está configurado correctamente.
