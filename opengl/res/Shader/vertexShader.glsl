#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

// Ajustes de vista (estática) y encuadre automático
uniform float uAspect;   // width/height
uniform float uScale;    // escala global para encajar en pantalla
uniform vec2  uCenter;   // centro del bounding box en coordenadas mundo

out vec3 ourColor;

void main()
{
	ourColor = aColor;

	// Auto-encuadre: recentrar y escalar
	vec2 p = (aPos.xy - uCenter) * uScale;

	// Corrección de aspect para que no se estire al redimensionar ventana
	p.x /= uAspect;

	gl_Position = vec4(p, aPos.z, 1.0f);
}
