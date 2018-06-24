#include <iostream>
#include <string>
#include <assert.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

using namespace std;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

//sombra
//intersecção quando o objeto é transparente

const GLuint WIDTH = 800, HEIGHT = 600;
const GLchar* vertexShaderSource = "#version 410\n"
"layout (location = 0) in vec3 position;\n"
"void main()\n"
"{\n"
"gl_Position = vec4(position.x, position.y, position.z, 1.0);\n"
"}\0";
const GLchar* fragmentShaderSource = "#version 410\n"
"uniform vec4 inputColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = inputColor;\n"
"}\n\0";

//void getColorComponents(unsigned int color, unsigned int& r, unsigned int& g, unsigned int& b) {
//	r = (color & 0xFF000000) >> 24;
//	g = (color & 0x00FF0000) >> 16;
//	b = (color & 0x0000FF00) >> 8;
//}
//
//void setColorComponents(unsigned int& color, unsigned int r, unsigned int g, unsigned int b) {
//	color = 0xFF | (b << 8) | (g << 16) | (r << 24);
//	//unsigned int colour = 0xFF | (0x00 << 8) | (0xCC << 16) | (0xFF << 24);
//}

glm::vec3 LerpRGB(glm::vec3 c1, glm::vec3 c2, float t) {

	float newR, newG, newB;
	glm::vec3 newColor;

	newR = c1.x*t + c2.x*(1 - t);
	newG = c1.y*t + c2.y*(1 - t); 
	newB = c1.z*t + c2.z*(1 - t);

	newColor = glm::vec3(newR, newG, newB);

	return newColor;
}

void normalize(glm::vec3& normal) {
	GLfloat length = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z+normal.z);
	normal.x = normal.x / length;
	normal.y = normal.y / length;
	normal.z = normal.z / length;
}

bool checkIfZero(float& value) {
	if (value == 0) {
		value = 1;
		return true;
	}
	return false;
}

bool calculateT(float pointC, float originC, float directionC, float& t) {
	bool wasZero = checkIfZero(directionC);
	t = (pointC - originC) / directionC;
	return wasZero;
}

bool intersect(glm::vec3 rayOr, glm::vec3 iRay, glm::vec3 point) {

	float tx, ty, tz;
	bool wasDirZeroX, wasDirZeroY, wasDirZeroZ;
	bool intersect = false;

	wasDirZeroX = calculateT(point.x, rayOr.x, iRay.x, tx);
	wasDirZeroY = calculateT(point.y, rayOr.y, iRay.y, ty);
	wasDirZeroZ = calculateT(point.z, rayOr.z, iRay.z, tz);

	if (wasDirZeroX) {
		if (wasDirZeroY) {
			if (wasDirZeroZ) {
				// 0 0 0
				intersect = ((rayOr.x == point.x) && (rayOr.y == point.y) && (rayOr.z == point.z));
			}
			else {
				// 0 0 !0
				intersect = ((rayOr.x == point.x) && (rayOr.y == point.y) &&
							 (tz*iRay.z == point.z - rayOr.z));
			}
		}
		else if (wasDirZeroZ) {
			//0 !0 0
			intersect = ((rayOr.x == point.x) && (rayOr.z == point.z) &&
						 (ty*iRay.y == point.y - rayOr.y));
		}
		else {
			//0 !0 !0
			intersect = ((rayOr.x == point.x) && (ty == tz));
		}
	}
	else {
		if (wasDirZeroY) {
			if (wasDirZeroZ) {
				//!0, 0, 0
				intersect = ((rayOr.y == point.y) && (rayOr.z == point.z) &&
							 (tx*iRay.x == point.x - rayOr.x));
			}
			else {
				//!0, 0, !0
				intersect = ((rayOr.y == point.y) && (tx == tz));
			}
		}
		else if (wasDirZeroZ) {
			//!0, !0, 0
			intersect = ((rayOr.z == point.z) && (tx == ty));
		}
		else {
			//!0, !0, !0
			intersect = tx == ty == tz;
		}
	}

	return intersect;
}

void reflect(glm::vec3 ray, glm::vec3 rayDir, glm::vec3 intersection, glm::vec3& reflected) {
	glm::vec3 normal = glm::vec3(ray + rayDir * intersection); // point of intersection
	normalize(normal);
	reflected = ray - normal * 2.0f * glm::dot(ray, normal);
}

void refract(glm::vec3 ray, glm::vec3 rayDir, glm::vec3 intersection, glm::vec3& refracted) {
	glm::vec3 normal = glm::vec3(ray + rayDir * intersection); // point of intersection
	normalize(normal);
	float cosi = glm::dot(-normal, rayDir); // -nhit.dot(raydir);
	float k = 1.0f - 0.8f * 0.8f * (1.0f - cosi * cosi);

	refracted = rayDir * 0.8f + normal * (0.8f *cosi - sqrt(k));
}

GLFWwindow* init() {
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glewExperimental = GL_TRUE;
	glewInit();
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	return window;
}

void initShaders(GLuint& shaderProgram) {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void bindBuffers(GLuint& VAO, GLuint& reflectVAO, GLuint& refractVAO, GLuint& vVAO, GLuint& VBOv, GLuint& VBO, GLuint& VBOreflect, GLuint& refractVBO, GLuint& shaderProgram) {
	
	glm::vec3 rayOr = glm::vec3(-0.5f, -0.5f, 0.0f);
	glm::vec3 rayDir = glm::vec3(-rayOr.y, rayOr.x, rayOr.z);

	//glm::vec3 rayOr = glm::vec3(-10.0f, 0.0f, 0.0f);
	//glm::vec3 rayDir = glm::vec3(-rayOr.x, rayOr.y, rayOr.z);
	//
	//glm::vec3 iRay = glm::vec3(0.0f, -10.0f, 0.0f);
	//glm::vec3 iRayDir = glm::vec3(-iRay.x, -iRay.y, iRay.z);
	//
	//glm::vec3 intersection1 = glm::vec3(0.8f, 1.0f, 0.0f);
	//
	//if (intersect(rayOr, rayDir, intersection1)) {
	//	printf("HÁ intersecção");
	//}
	//else {
	//	printf("NÃO há intersecção");
	//}
	glm::vec3 intersection = glm::vec3(-rayOr.y, rayOr.x, rayOr.z); //intersection point, static for now

	glm::vec3 reflectedDir;
	reflect(rayOr, rayDir, intersection, reflectedDir);
	
	glm::vec3 refractDir;
	refract(rayOr, rayDir, intersection, refractDir);

	//GLfloat vertices[] = {
	//	rayOr.x, rayOr.y, rayOr.z,
	//	intersection.x, intersection.y, intersection.z
	//};

	GLfloat vertices[] = {
		rayOr.x, rayOr.y, rayOr.z,
		rayDir.x, rayDir.y, rayDir.z
	};

	//GLfloat vRay[] = {
	//	iRay.x, iRay.y, iRay.z,
	//	iRayDir.x, iRayDir.y, iRayDir.z
	//};

	GLfloat reflect[] = {
		intersection.x, intersection.y, intersection.z,
		reflectedDir.x, reflectedDir.y, reflectedDir.z
	};
	
	GLfloat refract[] = {
		intersection.x, intersection.y, intersection.z,
		refractDir.x, refractDir.y, refractDir.z
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//glGenVertexArrays(1, &vVAO);
	//glGenBuffers(1, &VBOv);
	//glBindVertexArray(vVAO);
	//glBindBuffer(GL_ARRAY_BUFFER, VBOv);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vRay), vRay, GL_STATIC_DRAW);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	//glEnableVertexAttribArray(0);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);

	glGenVertexArrays(1, &reflectVAO);
	glGenBuffers(1, &VBOreflect);
	glBindVertexArray(reflectVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBOreflect);
	glBufferData(GL_ARRAY_BUFFER, sizeof(reflect), reflect, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	
	glGenVertexArrays(1, &refractVAO);
	glGenBuffers(1, &refractVBO);
	glBindVertexArray(refractVAO);
	glBindBuffer(GL_ARRAY_BUFFER, refractVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(refract), refract, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	//interpolation test
	glm::vec3 c1 = glm::vec3(1.0f, 0.0f, 1.0f);
	glm::vec3 c2 = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 color = LerpRGB(c1, c2, 0.2f);

	GLint colorLoc = glGetUniformLocation(shaderProgram, "inputColor");
	assert(colorLoc > -1);
	glUseProgram(shaderProgram);
	glUniform4f(colorLoc, color.x, color.y, color.z, 1.0f);

}

int main() {
	GLuint VBO, VBOreflect, VBOrefract, VBOv, VAO, reflectVAO, refractVAO, vVAO, shaderProgram;
	GLFWwindow* window = init();
	initShaders(shaderProgram);
	bindBuffers(VAO, reflectVAO, refractVAO, vVAO, VBOv, VBO, VBOreflect, VBOrefract, shaderProgram);
	
	while (!glfwWindowShouldClose(window)){
		glfwPollEvents();
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(shaderProgram);

		glBindVertexArray(VAO);
		glDrawArrays(GL_LINES, 0, 3);
		glBindVertexArray(0);

		//glBindVertexArray(vVAO);
		//glDrawArrays(GL_LINES, 0, 3);
		//glBindVertexArray(0);

		glBindVertexArray(reflectVAO);
		glDrawArrays(GL_LINES, 0, 3);
		glBindVertexArray(0);
		
		glBindVertexArray(refractVAO);
		glDrawArrays(GL_LINES, 0, 3);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}
	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &reflectVAO);
	glDeleteVertexArrays(1, &refractVAO);
	glDeleteVertexArrays(1, &vVAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &VBOreflect);
	glDeleteBuffers(1, &VBOrefract);
	glDeleteBuffers(1, &VBOv);
	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

