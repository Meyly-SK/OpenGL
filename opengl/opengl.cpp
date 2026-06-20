#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include "src/Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image/stb_image.h"

float vertices[] =
{
	//Posición			//Color				//Textura
	-0.5f,-0.5f,0.0f,	1.0f,0.0f,0.0f,		0.0f,0.0f,
	 0.5f,-0.5f,0.0f,	0.0f,1.0f,0.0f,		1.0f,0.0f,
	 0.5f, 0.5f,0.0f,	0.0f,0.0f,1.0f,		1.0f,1.0f,

	 0.5f, 0.5f,0.0f,	0.0f,0.0f,1.0f,		1.0f,1.0f,
	-0.5f, 0.5f,0.0f,	1.0f,0.0f,1.0f,		0.0f,1.0f,
	-0.5f,-0.5f,0.0f,	1.0f,0.0f,0.0f,		0.0f,0.0f,

};

void responsive(GLFWwindow* window, int width, int height);
void userInput(GLFWwindow* window);
void mouse_cursor_position(GLFWwindow* window, double xpos, double ypos);
void mouse_scroll_position(GLFWwindow* window, double xoffset, double yoffset);

unsigned int load_texture(const char* texture_path);

glm::mat4 model;
glm::vec3 myPos = glm::vec3(0.0f);

int main()
{
	//Inicializar GLFW
	glfwInit();

	//Inicializar la version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	//Crear ventana
	GLFWwindow* window = glfwCreateWindow(1200, 800, "Mi primera ventana", NULL, NULL);

	//Validar la ventana
	if (!window)
	{
		std::cout << "Error en al inicialziación la ventana\n";
		glfwTerminate();
	}


	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, responsive);
	glfwSetCursorPosCallback(window, mouse_cursor_position);
	glfwSetScrollCallback(window, mouse_scroll_position);

	//Validar GLEW
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize Glew\n";
		glfwTerminate();
	}


	//Buffers
	unsigned int VBO, VAO;

	glGenVertexArrays(1, &VAO);

	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

	//Vertices
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Colores
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//Textura
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//Textura
	stbi_set_flip_vertically_on_load(true);
	GLuint contianer_texture = load_texture("res/Texture/madera.jpg");
	GLuint face_texture = load_texture("res/Texture/awesomeface.png");

	//Shader
	Shader myShader("res/Shader/vertexShader.glsl", "res/Shader/fragmentShader.glsl");
	myShader.use();
	myShader.setInt("container_texture", 0);
	myShader.setInt("face_texture", 1);

	//iniiciamos
	while (!glfwWindowShouldClose(window))
	{

		//Cambios del FPS
		userInput(window);
		float time = glfwGetTime();
		float xValue = std::cos(time) / 2.0f + 0.5f; // 0.0f - 1.0f
		float yValue = std::sin(time) / 2.0f + 0.5f; // 0.0f - 1.0f
		float zValue = std::cos(time) / 2.0f + 0.5f; // 0.0f - 1.0f

		//vector
		glm::vec3 myVector;
		myVector.x = xValue;
		myVector.y = yValue;
		myVector.z = 0.31f;

		myShader.setVec3("colors", myVector);


		//Matrix
		model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f)); //Escala
		model = glm::translate(model, glm::vec3(myPos)); //Posicion
		model = glm::rotate(model, glm::radians(-55.0f) * time, glm::vec3(xValue, yValue, 0.5f)); //Rotacion
		myShader.setMat4("model", model);

		//Render Color
		glClearColor(0.1f, 0.2f, 0.3f, 1.0f); // 0.0 - 1.0
		glClear(GL_COLOR_BUFFER_BIT);

		myShader.use();
		glActiveTexture(GL_TEXTURE0); //Predeterminado
		glBindTexture(GL_TEXTURE_2D, contianer_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, face_texture);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Traspaso de la pantalla 
		glfwSwapBuffers(window);
		//cambio de eventos (congela)
		glfwPollEvents();
	}

	//Limpiar
	glfwDestroyWindow(window);
	glfwTerminate();

}

void responsive(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void userInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_TRUE)
		myPos.y += 0.005f;
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_TRUE)
		myPos.y -= 0.005f;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_TRUE)
		myPos.x += 0.005f;
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_TRUE)
		myPos.x -= 0.005f;
}
void mouse_cursor_position(GLFWwindow* window, double xpos, double ypos)
{

}

void mouse_scroll_position(GLFWwindow* window, double xoffset, double yoffset)
{
	//std::cout << "Scroll offset : " << xoffset << " " << yoffset << std::endl;
}

unsigned int load_texture(const char* texture_path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	//Filtros
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(texture_path, &width, &height, &nrChannels, 0);

	if (data)
	{
		//Reconocer el tipo de archivo
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		if (nrChannels == 3) // jpg file
			format = GL_RGB;
		if (nrChannels == 4) // png file
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Error al cargar la textura\n";
	}

	stbi_image_free(data);

	return textureID;
}