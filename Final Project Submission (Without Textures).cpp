#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <GL/GLU.h>
#include <iostream>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include<SOIL2/SOIL2.h>

using namespace std;

int width = 640;
int height = 480;
const double PI = 3.14159;
const float toRadians = PI / 180.0f;

/* GLSL Error Checking Definitions */
void PrintShaderCompileError(GLuint shader)
{
	int len = 0;
	int chWritten = 0;
	char* log;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	if (len > 0)
	{
		log = (char*)malloc(len);
		glGetShaderInfoLog(shader, len, &chWritten, log);
		cout << "Shader Compile Error: " << log << endl;
		free(log);
	}
}


void PrintShaderLinkingError(int prog)
{
	int len = 0;
	int chWritten = 0;
	char* log;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 0)
	{
		log = (char*)malloc(len);
		glGetShaderInfoLog(prog, len, &chWritten, log);
		cout << "Shader Linking Error: " << log << endl;
		free(log);
	}
}


bool IsOpenGLError()
{
	bool foundError = false;
	int glErr = glGetError();
	while (glErr != GL_NO_ERROR)
	{
		cout << "glError: " << glErr << endl;
		foundError = true;
		glErr = glGetError();
	}
	return foundError;
}

/* GLSL Error Checking Definitions End Here */

// Input function prototypes

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Declare View Matrix
glm::mat4 viewMatrix;

// Initialize FOV
GLfloat fov = 45.0f;

// Declare World Center
glm::vec3 worldCenter = glm::vec3(0.0f, 0.0f, 0.0f);

// Define Camera Attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 9.0f);							// Where the camera is located
glm::vec3 target = worldCenter;													// What the camera is looking at
glm::vec3 cameraDirection = glm::normalize(cameraPosition - target);			// Where the camera is looking
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);								// Where is up in the world
glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));	// Where is the camera's right
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));	// Where us up for the camera
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));			// Where is the front of the camera
GLfloat cameraSpeed = 0.03f;													// Modifier for camera movement speed
GLfloat cameraSensitivity = 0.005f;												// Modifier for camera look sensitivity

// Declare target prototype
glm::vec3 getTarget();

// Camera transformation prototype
void TransformCamera();

// Boolean for keys and mouse buttons
bool keys[1024], mouseButtons[3];

// Boolean to check camera transformations
bool isPanning = false;
bool isOrbiting = false;

bool isOrtho = false;
bool firstLoop = true;

// Radius, Pitch, and Yaw
GLfloat radius = 3.0f, rawYaw = 0.0f, rawPitch = 0.0f, degYaw, degPitch;

GLfloat deltaTime = 0.0f, lastFrame = 0.0f;
GLfloat lastX = width / 2, lastY = height / 2, xChange, yChange;

bool firstMouseMove = true; // Detect initial mouse movement

void initCamera();
void targetFollowsCursor();

// Draw Primitive(s)
void draw()
{
	// Cube drawing starts here
	GLenum mode = GL_TRIANGLES;
	GLsizei cubeIndices = 6;
	glDrawElements(mode, cubeIndices, GL_UNSIGNED_BYTE, nullptr);
}

// Create and Compile Shaders
static GLuint CompileShader(const string& source, GLuint shaderType)
{
	// Create Shader object
	GLuint shaderID = glCreateShader(shaderType);
	const char* src = source.c_str();

	// Attach source code to Shader object
	glShaderSource(shaderID, 1, &src, nullptr);

	// Compile Shader
	glCompileShader(shaderID);

	/* Shader Linking Error Check */
	GLint linked;
	IsOpenGLError();
	glGetProgramiv(shaderID, GL_LINK_STATUS, &linked);
	if (linked != 1)
	{
		cout << "Shader Linking Failed!" << endl;
		PrintShaderLinkingError(shaderID);
	}
	/* End here */

	// Return ID of Compiled shader
	return shaderID;

}

// Create Program Object
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader)
{
	// Compile vertex shader
	GLuint vertexShaderComp = CompileShader(vertexShader, GL_VERTEX_SHADER);

	// Compile fragment shader
	GLuint fragmentShaderComp = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

	// Create program object
	GLuint shaderProgram = glCreateProgram();

	// Attach vertex and fragment shaders to program object
	glAttachShader(shaderProgram, vertexShaderComp);
	glAttachShader(shaderProgram, fragmentShaderComp);

	// Link shaders to create executable
	glLinkProgram(shaderProgram);

	/* Shader Linking Error Check */
	GLint linked;
	IsOpenGLError();
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
	if (linked != 1)
	{
		cout << "Shader Linking Failed!" << endl;
		PrintShaderLinkingError(shaderProgram);
	}
	/* End here */

	// Delete compiled vertex and fragment shaders
	glDeleteShader(vertexShaderComp);
	glDeleteShader(fragmentShaderComp);

	// Return Shader Program
	return shaderProgram;

}


int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "Tyler Pruitt Project Milestone", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// Set input callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
		cout << "Error!" << endl;

	GLfloat brickRectangleVerticesTB[] = {	// Top Bottom

		// Triangle 1
		-1.5, -1.0, 0.0, // index 0
		0.82, 0.71, 0.55, // Tan

		-1.5, 1.0, 0.0, // index 1
		0.82, 0.71, 0.55, // Tan

		1.5, -1.0, 0.0,  // index 2	
		0.82, 0.71, 0.55, // Tan

		// Triangle 2	
		1.5, 1.0, 0.0,  // index 3	
		0.82, 0.71, 0.55, // Tan
	};

	// Define element indices
	GLubyte squareIndices[] = {
		0, 1, 2,
		1, 2, 3
	};

	GLfloat brickRectangleVerticesLR[] = {   // Left right

		// Triangle 1
		-1.5, -0.5, 0.0, // index 0
		0.79, 0.68, 0.52, // Tan

		-1.5, 0.5, 0.0, // index 1
		0.79, 0.68, 0.52, // Tan

		1.5, -0.5, 0.0,  // index 2	
		0.79, 0.68, 0.52, // Tan

		// Triangle 2	
		1.5, 0.5, 0.0,  // index 3	
		0.79, 0.68, 0.52, // Tan
	};

	GLfloat brickRectangleCapVertices[] = {   // Front Back

		// Triangle 1
		-1.0, -0.5, 0.0, // index 0
		0.79, 0.68, 0.52, // Tan

		-1.0, 0.5, 0.0, // index 1
		0.79, 0.68, 0.52, // Tan

		1.0, -0.5, 0.0,  // index 2	
		0.79, 0.68, 0.52, // Tan

		// Triangle 2	
		1.0, 0.5, 0.0,  // index 3	
		0.79, 0.68, 0.52, // Tan
	};

	GLfloat shelfRectangleVerticesTB[] = { // Top Bottom

		// Triangle 1
		-5.0, -1.0, 0.0, // index 0
		0.59, 0.29, 0.0, // brown

		-5.0, 1.0, 0.0, // index 1
		0.59, 0.29, 0.0, // brown

		5.0, -1.0, 0.0,  // index 2	
		0.59, 0.29, 0.0, // brown

		// Triangle 2	
		5.0, 1.0, 0.0,  // index 3	
		0.59, 0.29, 0.0, // brown
	};

	GLfloat shelfRectangleVerticesFB[] = { // Front Back

		// Triangle 1
		-5.0, -0.25, 0.0, // index 0
		0.65, 0.35, 0.0, // brown

		-5.0, 0.25, 0.0, // index 1
		0.65, 0.35, 0.0, // brown

		5.0, -0.25, 0.0,  // index 2	
		0.65, 0.35, 0.0, // brown

		// Triangle 2	
		5.0, 0.25, 0.0,  // index 3	
		0.65, 0.35, 0.0, // brown

	};

	GLfloat shelfRectangleCapVertices[] = { // Left Right

		// Triangle 1
		-1.0, -0.25, 0.0, // index 0
		0.59, 0.29, 0.0, // brown

		-1.0, 0.25, 0.0, // index 1
		0.59, 0.29, 0.0, // brown

		1.0, -0.25, 0.0,  // index 2	
		0.59, 0.29, 0.0, // brown

		// Triangle 2	
		1.0, 0.25, 0.0,  // index 3	
		0.59, 0.29, 0.0, // brown

	};

	GLfloat floorVertices[] = {

		// Triangle 1
		-0.5, -0.5, 0.0, // index 0
		0.49, 0.19, 0.0, // brown

		-0.5, 0.5, 0.0, // index 1
		0.49, 0.19, 0.0, // brown

		0.5, -0.5, 0.0,  // index 2	
		0.49, 0.19, 0.0, // brown

		// Triangle 2	
		0.5, 0.5, 0.0,  // index 3	
		0.49, 0.19, 0.0, // brown
	};

	GLfloat wallVertices[] = {

		// Triangle 1
		-7.5, 0.0, -2.75, // index 0
		0.0, 0.7, 0.7, // Teal

		-7.5, 15.5, -2.75, // index 1
		0.0, 0.7, 0.7, // Teal

		7.5, 0.0, -2.75,  // index 2	
		0.0, 0.7, 0.7, // Teal

		// Triangle 2	
		7.5, 15.5, -2.75,  // index 3	
		0.0, 0.7, 0.7, // Teal
	};

	// Plane Transforms
	glm::vec3 brickPlanePositions[] = {
		glm::vec3(4.0f,  4.25f,  0.0f), // left 0
		glm::vec3(2.0f,  4.25f,  0.0f), // right 1
		glm::vec3(3.0f,  4.75f,  0.0f), // top 2
		glm::vec3(3.0f,  3.75f,  0.0f), // bottom 3
	};

	glm::float32 brickPlaneRotations[] = {
		90.0f, 90.0f, 90.0f, 90.0f		// left, right, top, bottom, respectively
	};

	// Plane Transforms
	glm::vec3 brickRectangleCapPlanePositions[] = {
		glm::vec3(3.0f,  4.25f,  -1.5f), // back 0
		glm::vec3(3.0f,  4.25f,  1.5f), // front 1
	};

	glm::float32 brickRectangleCapPlaneRotations[] = {
		0.0f, 0.0f		// back, front respectively
	};

	// Plane Transforms
	glm::vec3 shelfRectangleCapPlanePositions[] = {
		// Horizontal
		//Bottom shelf
		glm::vec3(-5.0f, 0.5f,  0.0f), // left 0
		glm::vec3(5.0f,  0.5f,  0.0f), // right 1

		glm::vec3(-5.0f, 3.5f,  0.0f), // left 2
		glm::vec3(5.0f,  3.5f,  0.0f), // right 3

		glm::vec3(-5.0f, 6.5f,  0.0f), // left 4
		glm::vec3(5.0f,  6.5f,  0.0f), // right 5
		//	Top shelf
		glm::vec3(-5.0f, 9.5f,  0.0f), // left 6
		glm::vec3(5.0f,  9.5f,  0.0f), // right 7

		// Vertical
		// Leftmost
		glm::vec3(-4.5f, 10.0f,  0.0f), // top	8
		glm::vec3(-4.5f,  0.0f,  0.0f), // Bottom 9

		glm::vec3(-1.5f, 10.0f,  0.0f), // top 10
		glm::vec3(-1.5f,  0.0f,  0.0f), // bottom 11

		glm::vec3(1.5f, 10.0f,  0.0f), // top 12
		glm::vec3(1.5f,  0.0f,  0.0f), // bottom 13
		// Rightmost
		glm::vec3(4.5f, 10.0f,  0.0f), // top 14
		glm::vec3(4.5f,  0.0f,  0.0f) // bottom 15
	};

	glm::float32 shelfRectangleCapPlaneRotations[] = {
		90.0f, 90.0f, // Bottom shelf
		90.0f, 90.0f,
		90.0f, 90.0f, 
		90.0f, 90.0f, // Top shelf

		//Pillars
		90.0f, 90.0f, // Leftmost pillar
		90.0f, 90.0f,
		90.0f, 90.0f,
		90.0f, 90.0f  // Rightmost pillar
	};

	// Plane Transforms
	glm::vec3 shelfRectanglePlanePositions[] = {
		// Horizontal
		// Bottom shelf
		glm::vec3(0.0f,  0.5f,  1.0f), // front 0
		glm::vec3(0.0f,  0.5f,  -1.0f), // back 1
		glm::vec3(0.0f,  0.75f,  0.0f), // top 2
		glm::vec3(0.0f,  0.25f,  0.0f), // bottom 3

		glm::vec3(0.0f,  3.5f,  1.0f), // front 4
		glm::vec3(0.0f,  3.5f,  -1.0f), // back 5
		glm::vec3(0.0f,  3.75f,  0.0f), // top 6
		glm::vec3(0.0f,  3.25f,  0.0f), // bottom 7

		glm::vec3(0.0f,  6.5f,  1.0f), // front 8
		glm::vec3(0.0f,  6.5f,  -1.0f), // back 9
		glm::vec3(0.0f,  6.75f,  0.0f), // top 10
		glm::vec3(0.0f,  6.25f,  0.0f), // bottom 11
		// Top shelf
		glm::vec3(0.0f,  9.5f,  1.0f), // front 12
		glm::vec3(0.0f,  9.5f,  -1.0f), // back 13
		glm::vec3(0.0f,  9.75f,  0.0f), // top 14
		glm::vec3(0.0f,  9.25f,  0.0f), // bottom 15

		// Vertical
		// Left Pillar
		glm::vec3(-4.5f,  5.0f,  1.0f), // front 16
		glm::vec3(-4.5f,  5.0f,  -1.0f), // back 17
		glm::vec3(-4.75f,  5.0f,  0.0f), // left 18
		glm::vec3(-4.25f,  5.0f,  0.0f), // right 19

		glm::vec3(-1.5f,  5.0f,  1.0f), // front 20
		glm::vec3(-1.5f,  5.0f,  -1.0f), // back 21
		glm::vec3(-1.75f,  5.0f,  0.0f), // left 22
		glm::vec3(-1.25f,  5.0f,  0.0f), // right 23

		glm::vec3(1.5f,  5.0f,  1.0f), // front 24
		glm::vec3(1.5f,  5.0f,  -1.0f), // back 25
		glm::vec3(1.75f,  5.0f,  0.0f), // left 26
		glm::vec3(1.25f,  5.0f,  0.0f), // right 27

		glm::vec3(4.5f,  5.0f,  1.0f), // front 28
		glm::vec3(4.5f,  5.0f,  -1.0f), // back 29
		glm::vec3(4.75f,  5.0f,  0.0f), // left 30
		glm::vec3(4.25f,  5.0f,  0.0f) // right 31
	};

	glm::float32 shelfRectanglePlaneRotations[] = {
		// Horizontal
		0.0f, 0.0f, 90.0f, 90.0f, // Bottom shelf
		0.0f, 0.0f, 90.0f, 90.0f, 
		0.0f, 0.0f, 90.0f, 90.0f, 
		0.0f, 0.0f, 90.0f, 90.0f,	// Top shelf
		
		// Vertical
		90.0f, 90.0f, 90.0f, 90.0f,	// leftmost pillar
		90.0f, 90.0f, 90.0f, 90.0f,
		90.0f, 90.0f, 90.0f, 90.0f,
		90.0f, 90.0f, 90.0f, 90.0f	// rightmost pillar

	};

	glm::float32 toiletPaperCylinderVertices[] = {

		// positon attributes (x,y,z)
		0.0f, 0.0f, 0.0f,  // vert 1
		0.0f, 0.0f, 0.0f, // black

		0.5f, 0.866f, 0.0f, // vert 2
		1.0f, 1.0f, 1.0f, // white

		1.0f, 0.0f, 0.0f, // vert 3
		1.0f, 1.0f, 1.0f, // white

		0.5f, 0.866f, 0.0f, // vert 4
		1.0f, 1.0f, 1.0f, // white

		0.5f, 0.866f, -2.0f, // vert 5
		1.0f, 1.0f, 1.0f, // white

		1.0f, 0.0f, 0.0f, // vert 6
		1.0f, 1.0f, 1.0f, // white

		1.0f, 0.0f, 0.0f, // vert 7
		1.0f, 1.0f, 1.0f, // white

		1.0f, 0.0f, -2.0f, // vert 8
		1.0f, 1.0f, 1.0f, // white

		0.5f, 0.866f, -2.0f, // vert 9
		1.0f, 1.0f, 1.0f // white
	};

	glm::float32 triangleIndices[] = {
		0,1,2
	};

	glm::float32 toiletPaperCylinderRotations[] = {
		0.0f, 60.0f, 120.0f, 180.0f, 240.0f, 300.0f		// Counterclockwise
	};

	glm::vec3 toiletPaperCylinderPositions[] = {		// left middle shelf
		glm::vec3(-3.0f,  4.65f,  1.0f),
		glm::vec3(-3.0f,  4.65f,  1.0f),
		glm::vec3(-3.0f,  4.65f,  1.0f),
		glm::vec3(-3.0f,  4.65f,  1.0f),
		glm::vec3(-3.0f,  4.65f,  1.0f),
		glm::vec3(-3.0f,  4.65f,  1.0f),
	};

	glm::float32 tennisBallSphereVertices[] = {	// counterclockwise around (0,0)

		// positon attributes (x,y,z)	
		0.0f, 0.0f, 0.3f, // vert 1
		1.0f, 0.55f, 0.0f, // orange

		1.0f, 0.0f, 0.0f, // vert 2
		1.0f, 0.55f, 0.0f, // orange

		0.5f, 0.866f, 0.0f, // vert 3
		1.0f, 0.6f, 0.0f, // orange
//          ----------------
		0.0f, 0.0f, 0.3f, // vert 4
		1.0f, 0.55f, 0.0f, // orange

		0.5f, 0.866f, 0.0f, // vert 5
		1.0f, 0.65f, 0.0f, // orange

		-0.5f, 0.866f, 0.0f, // vert 6
		1.0f, 0.65f, 0.0f, // orange
//          ----------------
		0.0f, 0.0f, 0.3f, // vert 7
		1.0f, 0.55f, 0.0f, // orange

		-0.5f, 0.866f, 0.0f, // vert 8
		1.0f, 0.65f, 0.0f, // orange

		-1.0f, -0.0f, 0.0f, // vert 9
		1.0f, 0.65f, 0.0f, // orange
//          ----------------
		0.0f, 0.0f, 0.3f,  // vert 10
		1.0f, 0.62f, 0.0f, // orange

		-1.0f, -0.0f, 0.0f, // vert 11
		1.0f, 0.65f, 0.0f, // orange

		-0.5f, -0.866f, 0.0f, // vert 12
		1.0f, 0.65f, 0.0f, // orange
//          ----------------
		0.0f, 0.0f, 0.3f,  // vert 13				
		1.0f, 0.6f, 0.0f, // orange

		-0.5f, -0.866f, 0.0f, // vert 14
		1.0f, 0.6f, 0.0f, // orange

		0.5f, -0.866f, 0.0f, // vert 15
		1.0f, 0.55f, 0.0f, // orange
//          ----------------
		0.0f, 0.0f, 0.3f,  // vert 16				
		1.0f, 0.62f, 0.0f, // orange

		0.5f, -0.866f, 0.0f, // vert 17
		1.0f, 0.55f, 0.0f, // orange

		1.0f, 0.0f, 0.0f, // vert 18
		1.0f, 0.62f, 0.0f, // orange

	};
	glm::vec3 tennisBallSpherePositions[] = {		// top middle shelf
		glm::vec3(0.0f,  7.5f,  0.5f), // front
		glm::vec3(0.5f,  7.5f,  0.0f), // right
		glm::vec3(0.0f,  7.5f,  -0.5f), // back
		glm::vec3(-0.5f,  7.5f,  0.0f), // left
		glm::vec3(0.0f,  8.0f,  0.0f),	// top
		glm::vec3(0.0f,  7.0f,  0.0f) // bottom
	};

	glm::float32 tennisBallSphereRotations[] = {

		0.0f, 90.0f, 180.0f, -90.0f, -90.0f, 90.0f	// front, right, back, left, top, bottom, respectively

	};

	// Setup some OpenGL options
	glEnable(GL_DEPTH_TEST);

	// Wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	GLuint brickRectangleTBVBO, brickRectangleTBEBO, brickRectangleTBVAO;
	glGenBuffers(1, &brickRectangleTBVBO); // Create VBO
	glGenBuffers(1, &brickRectangleTBEBO); // Create EBO
	glGenVertexArrays(1, &brickRectangleTBVAO); // Create VOA

	glBindVertexArray(brickRectangleTBVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, brickRectangleTBVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, brickRectangleTBEBO); // Select EBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(brickRectangleVerticesTB), brickRectangleVerticesTB, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW); // Load indices 
		// Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0); // Unbind VOA or close off (Must call VOA explicitly in loop)

	GLuint brickRectangleLRVBO, brickRectangleLREBO, brickRectangleLRVAO;
	glGenBuffers(1, &brickRectangleLRVBO); // Create VBO
	glGenBuffers(1, &brickRectangleTBEBO); // Create EBO
	glGenVertexArrays(1, &brickRectangleLRVAO); // Create VOA

	glBindVertexArray(brickRectangleLRVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, brickRectangleLRVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, brickRectangleTBEBO); // Select EBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(brickRectangleVerticesLR), brickRectangleVerticesLR, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW); // Load indices 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0); // Unbind VOA or close off (Must call VOA explicitly in loop)

	GLuint brickRectangleCapVBO, brickRectangleCapEBO, brickRectangleCapVAO; // Left Right
	glGenBuffers(1, &brickRectangleCapVBO); // Create VBO
	glGenBuffers(1, &brickRectangleCapEBO); // Create EBO
	glGenVertexArrays(1, &brickRectangleCapVAO); // Create VOA

	glBindVertexArray(brickRectangleCapVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, brickRectangleCapVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, brickRectangleCapEBO); // Select EBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(brickRectangleCapVertices), brickRectangleCapVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW); // Load indices 
		// Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	GLuint floorVBO, floorEBO, floorVAO;
	glGenBuffers(1, &floorVBO); // Create VBO
	glGenBuffers(1, &floorEBO); // Create EBO

	glGenVertexArrays(1, &floorVAO); // Create VOA
		glBindVertexArray(floorVAO);
		glBindBuffer(GL_ARRAY_BUFFER, floorVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO); // Select EBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW); // Load indices 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	GLuint wallVBO, wallEBO, wallVAO;
	glGenBuffers(1, &wallVBO); // Create VBO
	glGenBuffers(1, &wallEBO); // Create EBO

	glGenVertexArrays(1, &wallVAO); // Create VOA
		glBindVertexArray(wallVAO);
		glBindBuffer(GL_ARRAY_BUFFER, wallVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO); // Select EBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW); // Load indices 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	GLuint shelfRectangleTBVBO, shelfRectangleTBEBO, shelfRectangleTBVAO; // Shelf Top Bottom
	glGenBuffers(1, &shelfRectangleTBVBO); // Create VBO
	glGenBuffers(1, &shelfRectangleTBEBO); // Create EBO
	glGenVertexArrays(1, &shelfRectangleTBVAO); // Create VOA

	glBindVertexArray(shelfRectangleTBVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, shelfRectangleTBVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shelfRectangleTBEBO); // Select EBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(shelfRectangleVerticesTB), shelfRectangleVerticesTB, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW); // Load indices 
		// Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	GLuint shelfRectangleFBVBO, shelfRectangleFBEBO, shelfRectangleFBVAO; // Front Back
	glGenBuffers(1, &shelfRectangleFBVBO); // Create VBO
	glGenBuffers(1, &shelfRectangleFBEBO); // Create EBO
	glGenVertexArrays(1, &shelfRectangleFBVAO); // Create VOA

	glBindVertexArray(shelfRectangleFBVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, shelfRectangleFBVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shelfRectangleFBEBO); // Select EBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(shelfRectangleVerticesFB), shelfRectangleVerticesFB, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW); // Load indices 
		// Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	GLuint shelfRectangleCapVBO, shelfRectangleCapEBO, shelfRectangleCapVAO; // Left Right
	glGenBuffers(1, &shelfRectangleCapVBO); // Create VBO
	glGenBuffers(1, &shelfRectangleCapEBO); // Create EBO
	glGenVertexArrays(1, &shelfRectangleCapVAO); // Create VOA

	glBindVertexArray(shelfRectangleCapVAO);
		// VBO and EBO Placed in User-Defined VAO
		glBindBuffer(GL_ARRAY_BUFFER, shelfRectangleCapVBO); // Select VBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shelfRectangleCapEBO); // Select EBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(shelfRectangleCapVertices), shelfRectangleCapVertices, GL_STATIC_DRAW); // Load vertex attributes
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW); // Load indices 
		// Specify attribute location and layout to GPU
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
	glBindVertexArray(0);


	GLuint toiletPaperCylinderVBO, toiletPaperCylinderVAO; // triangle VAO variables
	glGenVertexArrays(1, &toiletPaperCylinderVAO); // Create VAO
	glGenBuffers(1, &toiletPaperCylinderVBO); // Create VBO

	glBindVertexArray(toiletPaperCylinderVAO); // Activate VAO for VBO association
		glBindBuffer(GL_ARRAY_BUFFER, toiletPaperCylinderVBO); // Enable VBO	
		glBufferData(GL_ARRAY_BUFFER, sizeof(toiletPaperCylinderVertices), toiletPaperCylinderVertices, GL_STATIC_DRAW); // Copy Vertex data to VBO
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0); // Associate VBO with VA (Vertex Attribute)
		glEnableVertexAttribArray(0); // Enable VA
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); // Associate VBO with VA
		glEnableVertexAttribArray(1); // Enable VA
	glBindVertexArray(0); // Unbind VAO (Optional but recommended)

	GLuint tennisBallSphereVBO, tennisBallSphereVAO; // triangle VAO variables
	glGenVertexArrays(1, &tennisBallSphereVAO); // Create VAO
	glGenBuffers(1, &tennisBallSphereVBO); // Create VBO

	glBindVertexArray(tennisBallSphereVAO); // Activate VAO for VBO association
		glBindBuffer(GL_ARRAY_BUFFER, tennisBallSphereVBO); // Enable VBO	
		glBufferData(GL_ARRAY_BUFFER, sizeof(tennisBallSphereVertices), tennisBallSphereVertices, GL_STATIC_DRAW); // Copy Vertex data to VBO
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0); // Associate VBO with VA (Vertex Attribute)
		glEnableVertexAttribArray(0); // Enable VA
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat))); // Associate VBO with VA
		glEnableVertexAttribArray(1); // Enable VA
	glBindVertexArray(0); // Unbind VAO (Optional but recommended)

	// Vertex shader source code
	string vertexShaderSource =
		"#version 330 core\n"
		"layout(location = 0) in vec4 vPosition;"
		"layout(location = 1) in vec4 aColor;"
		"out vec4 oColor;"
		"uniform mat4 model;"
		"uniform mat4 view;"
		"uniform mat4 projection;"
		"void main()\n"
		"{\n"
		"gl_Position = projection * view * model * vPosition;"
		"oColor = aColor;"
		"}\n";

	// Fragment shader source code
	string fragmentShaderSource =
		"#version 330 core\n"
		"in vec4 oColor;"
		"out vec4 fragColor;"
		"void main()\n"
		"{\n"
		"fragColor = oColor;"
		"}\n";

	// Creating Shader Program
	GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);


	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		// Set deltaTime
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Resize window and graphics simultaneously
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use Shader Program exe and select VAO before drawing 
		glUseProgram(shaderProgram); // Call Shader per-frame when updating attributes


		// Declare transformations (can be initialized outside loop)
		glm::mat4 projectionMatrix;

		viewMatrix = glm::lookAt(cameraPosition, getTarget(), worldUp);

		if (isOrtho == true) {
			glm::ortho(-1.0f, 600.0f, 1.0f, 600.0f, -1.0f, 100.0f);
			//		cout << "We're Ortho" << endl;
		}
		else {
			projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
			//		cout << "We're Projection" << endl;
		}

		// Get matrix's uniform location and set matrix
		GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		glBindVertexArray(brickRectangleTBVAO); // User-defined VAO must be called before draw. 
			for (GLuint i = 2; i < 4; i++)	// Top Bottom
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, brickPlanePositions[i]);
				modelMatrix = glm::rotate(modelMatrix, brickPlaneRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::rotate(modelMatrix, brickPlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}
		glBindVertexArray(0); //Incase different VAO wll be used after

			glBindVertexArray(brickRectangleLRVAO); // User-defined VAO must be called before draw. 
			for (GLuint i = 0; i < 2; i++)	// Brick left right
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, brickPlanePositions[i]);
				modelMatrix = glm::rotate(modelMatrix, brickPlaneRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}
		// Unbind Shader exe and VOA after drawing per frame
		glBindVertexArray(0); //Incase different VAO will be used after

		glBindVertexArray(brickRectangleCapVAO); // User-defined VAO must be called before draw.
			for (GLuint i = 0; i < 2; i++)		// Brick front back
			{
				glm::mat4 modelMatrix;
				modelMatrix = glm::translate(modelMatrix, brickRectangleCapPlanePositions[i]);
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				// Draw primitive(s)
				draw();
			}
		// Unbind Shader exe and VOA after drawing per frame
		glBindVertexArray(0); //Incase different VAO wii be used after

		glBindVertexArray(floorVAO);  // floor square
			glm::mat4 modelMatrix;
			modelMatrix = glm::rotate(modelMatrix, 90.f * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(15.0f, 15.0f, 15.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			draw();
		glBindVertexArray(0); //Incase different VAO will be used after

		glBindVertexArray(wallVAO);  // Wall square
			glm::mat4 wallModelMatrix;
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallModelMatrix));
			draw();
		glBindVertexArray(0); //Incase different VAO will be used after


		glBindVertexArray(shelfRectangleTBVAO); // User-defined VAO must be called before draw. 
			for (GLuint i = 0; i < 32; i++)	{
				// Create shelf top and bottoms
				if (i >= 2 && i < 4) {		// Bottom shelf
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 6 && i < 8) {
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 10 && i < 12) {
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 14 && i < 16) {		// top shelf
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}
			
				// Create Pillar Left and Rights
				if (i >= 18 && i < 20) {		// leftmost pillar
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 22 && i < 24) {
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 26 && i < 28) {
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 30 && i < 32) {		// rightmost pillar
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

			}
		// Unbind Shader exe and VOA after drawing per frame
	glBindVertexArray(0); //Incase different VAO wll be used after

		glBindVertexArray(shelfRectangleFBVAO); // User-defined VAO must be called before draw. 
			for (GLuint i = 0; i < 32; i++)	
			{
				// Create shelf front and back
				if (i >= 0 && i < 2) {	 // Bottom Shelf
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 4 && i < 6) {
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 8 && i < 10) {
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 12 && i < 14) {		// Top shelf
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				// Create Pillar Fronts and Backs
				if (i >= 16 && i < 18) {	// Leftmost pillar
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}
	
				if (i >= 20 && i < 22) {
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}				

				if (i >= 24 && i < 26) {
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
					}

				if (i >= 28 && i < 30) {	// Rightmost pillar
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectanglePlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectanglePlaneRotations[i] * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}
			}

		// Unbind Shader exe and VOA after drawing per frame
		glBindVertexArray(0); //Incase different VAO wll be used after

		glBindVertexArray(shelfRectangleCapVAO); // User-defined VAO must be called before draw. 
			for (GLuint i = 0; i < 16; i++)
			{
				if (i >= 0 && i < 8) {
					// Create shelf lefts and rights
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectangleCapPlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectangleCapPlaneRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}

				if (i >= 8 && i < 16) {
					// Create pillar tops and bottoms
					glm::mat4 modelMatrix;
					modelMatrix = glm::translate(modelMatrix, shelfRectangleCapPlanePositions[i]);
					modelMatrix = glm::rotate(modelMatrix, shelfRectangleCapPlaneRotations[i] * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
					modelMatrix = glm::rotate(modelMatrix, shelfRectangleCapPlaneRotations[i] * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
					// Draw primitive(s)
					draw();
				}
			}

		glBindVertexArray(0); //Incase different VAO wll be used after

		glBindVertexArray(toiletPaperCylinderVAO); // User-defined VAO must be called before draw.
			for (int i = 0; i < 6; i++) {
			
				modelMatrix = glm::translate(glm::mat4(1.0f), toiletPaperCylinderPositions[i]); // Position strip at 0,0,0
				modelMatrix = glm::rotate(modelMatrix, glm::radians(360.f), glm::vec3(1.0f, 0.0f, 0.0f)); 
				modelMatrix = glm::rotate(modelMatrix, glm::radians(toiletPaperCylinderRotations[i]), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate strip on z by increments in array		
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
				glDrawArrays(GL_TRIANGLES, 0, 9); // Render primitive or execute shader per draw
			}
		glBindVertexArray(0); //Incase different VAO wll be used after

		glBindVertexArray(tennisBallSphereVAO); // User-defined VAO must be called before draw.
			for (int i = 0; i < 6; i++) {
				modelMatrix = glm::translate(glm::mat4(1.0f), tennisBallSpherePositions[i]); // Position strip at 0,0,0
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));

				if (i >= 0 && i < 4) {
				modelMatrix = glm::rotate(modelMatrix, glm::radians(tennisBallSphereRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate strip on y by increments in array	
				}

				if (i >= 4 && i <= 5) {
					modelMatrix = glm::rotate(modelMatrix, glm::radians(tennisBallSphereRotations[i]), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate strip on x by increments in array
				}		
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
			glDrawArrays(GL_TRIANGLES, 0, 18); // Render primitive or execute shader per draw
			}

		glBindVertexArray(0); //Incase different VAO wll be used after

		glUseProgram(0); // Incase different shader will be used after


		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		// Poll camera transformations
		TransformCamera();
	}

	//Clear GPU resources

	glDeleteVertexArrays(1, &floorVAO);
	glDeleteBuffers(1, &floorVBO);
	glDeleteBuffers(1, &floorEBO);

	glDeleteVertexArrays(1, &wallVAO);
	glDeleteBuffers(1, &wallVBO);
	glDeleteBuffers(1, &wallEBO);

	glDeleteVertexArrays(1, &brickRectangleTBVAO);
	glDeleteBuffers(1, &brickRectangleTBVBO);
	glDeleteBuffers(1, &brickRectangleTBEBO);

	glDeleteVertexArrays(1, &brickRectangleLRVAO);
	glDeleteBuffers(1, &brickRectangleLRVBO);
	glDeleteBuffers(1, &brickRectangleTBEBO);

	glDeleteVertexArrays(1, &brickRectangleCapVAO);
	glDeleteBuffers(1, &brickRectangleCapVBO);
	glDeleteBuffers(1, &brickRectangleCapEBO);

	glDeleteVertexArrays(1, &shelfRectangleTBVAO);
	glDeleteBuffers(1, &shelfRectangleTBVBO);
	glDeleteBuffers(1, &shelfRectangleTBEBO);

	glDeleteVertexArrays(1, &shelfRectangleFBVAO);
	glDeleteBuffers(1, &shelfRectangleFBVBO);
	glDeleteBuffers(1, &shelfRectangleFBEBO);

	glDeleteVertexArrays(1, &shelfRectangleCapVAO);
	glDeleteBuffers(1, &shelfRectangleCapVBO);
	glDeleteBuffers(1, &shelfRectangleCapEBO);

	glDeleteVertexArrays(1, &toiletPaperCylinderVAO);
	glDeleteBuffers(1, &toiletPaperCylinderVBO);

	glDeleteVertexArrays(1, &tennisBallSphereVAO);
	glDeleteBuffers(1, &tennisBallSphereVBO);

	glfwTerminate();
	return 0;
}

// Define Input Callback functions
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Default cameraSpeed
	if (cameraSpeed < 0.01f)
		cameraSpeed = 0.01f;
	if (cameraSpeed > 0.3f)
		cameraSpeed = 0.3f;

	cameraSpeed += yoffset * deltaTime;
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouseMove)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}

	// Calculate cursor offset
	xChange = xpos - lastX;
	yChange = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	// Camera Follows Cursor
	targetFollowsCursor();

	// Pan camera
	if (isPanning)
	{
		if (cameraPosition.z < 0.0f)
			cameraFront.z = 1.0f;
		else
			cameraPosition.z = -1.0f;

		cameraSpeed = xChange * deltaTime;
		cameraPosition += cameraSpeed * cameraRight;

		cameraSpeed = yChange * deltaTime;
		cameraPosition += cameraSpeed * cameraUp;
	}

	// Orbit camera,
	if (isOrbiting)
	{
		rawYaw += xChange;
		rawPitch += yChange;

		// Convert Yaw and Pitch to degrees
		degYaw = glm::radians(rawYaw);
		degPitch = glm::radians(rawPitch);
		degPitch = glm::clamp(glm::radians(rawPitch), -glm::pi<float>() /2.0f + 0.1f, glm::pi<float>() / 2.0f - 0.1f);

		// Azimuth Altitude formula
		cameraPosition.x = target.x + radius * cosf(degPitch) * sinf(degYaw);
		cameraPosition.y = target.y + radius * sinf(degPitch);
		cameraPosition.z = target.z + radius * cosf(degPitch) * cosf(degYaw);
	}

}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
		mouseButtons[button] = true;
	else if (action == GLFW_RELEASE)
		mouseButtons[button] = false;
}

// Define getTarget function
glm::vec3 getTarget()
{
	target = cameraPosition + cameraFront;
	return target;
}

// Define TransformCamera function
void TransformCamera()
{
	// Pan camera
	if (keys[GLFW_KEY_LEFT_ALT] && mouseButtons[GLFW_MOUSE_BUTTON_MIDDLE])
		isPanning = true;
	else
		isPanning = false;

	// Orbit camera
	if (keys[GLFW_KEY_LEFT_ALT] && mouseButtons[GLFW_MOUSE_BUTTON_LEFT])
		isOrbiting = true;
	else
		isOrbiting = false;

	if (keys[GLFW_KEY_W]) {								// Forward
		cameraPosition += cameraSpeed * cameraFront;
	}

	if (keys[GLFW_KEY_A]) {								// Left
		cameraPosition -= cameraRight * cameraSpeed;
	}

	if (keys[GLFW_KEY_S]) {								// Back
		cameraPosition -= cameraSpeed * cameraFront;
	}

	if (keys[GLFW_KEY_D]) {								// Right
		cameraPosition += cameraRight * cameraSpeed;
	}

	if (keys[GLFW_KEY_Q]) {								// Up
		cameraPosition += cameraUp * cameraSpeed;
	}

	if (keys[GLFW_KEY_E]) {								// Down
		cameraPosition -= cameraUp * cameraSpeed;
	}

	if (keys[GLFW_KEY_P]) {
		glViewport(0.0f, 0.0f, width, height);
		if (!isOrtho) {
			isOrtho = true;
		}
		else {
			isOrtho = false;
		}
	}

	// Reset camera
	if (keys[GLFW_KEY_F])
		initCamera();
}

void initCamera()
{
	cameraPosition = glm::vec3(0.0f, 0.0f, 9.0f);
	target = worldCenter;
	cameraDirection = glm::normalize(cameraPosition - target);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
	cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
	cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
}

void targetFollowsCursor()
{
	//Yaw
	cameraFront.x += xChange * cameraSensitivity;
	//Pitch
	cameraFront.y += yChange * cameraSensitivity;
}