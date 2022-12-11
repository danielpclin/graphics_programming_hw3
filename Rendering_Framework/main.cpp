#include "src/Shader.h"
#include "src/SceneRenderer.h"
#include <GLFW/glfw3.h>
#include "src/MyImGuiPanel.h"

#include "src/ViewFrustumSceneObject.h"
#include "src/InfinityPlane.h"
#include "src/MyPoissonSample.h"
#include "src/Model.h"
#include "src/ModelOld.h"
#include "src/Program.h"

#pragma comment (lib, "lib-vc2015\\glfw3.lib")
#pragma comment(lib, "assimp-vc141-mt.lib")

int FRAME_WIDTH = 1024;
int FRAME_HEIGHT = 512;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void cursorPosCallback(GLFWwindow* window, double x, double y);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

bool initializeGL();
void resizeGL(GLFWwindow *window, int w, int h);
void paintGL();
void resize(const int w, const int h);

bool m_leftButtonPressed = false;
bool m_rightButtonPressed = false;
double cursorPos[2];


MyImGuiPanel* m_imguiPanel = nullptr;

void vsyncDisabled(GLFWwindow *window);

struct DrawElementsIndirectCommand {
	unsigned int count;
	unsigned int instanceCount;
	unsigned int firstIndex;
	unsigned int baseVertex;
	unsigned int baseInstance;
};

// ==============================================
SceneRenderer *defaultRenderer = nullptr;

glm::mat4 godProjMat;
glm::mat4 godViewMat;
glm::mat4 playerProjMat;
glm::mat4 playerViewMat;

ViewFrustumSceneObject* viewFrustumSO = nullptr;
InfinityPlane *infinityPlane = nullptr;

Program* program;
ModelOld* plants[3];
Model* model;
MyPoissonSample* samples[3];
// ==============================================

void updateWhenPlayerProjectionChanged(const float nearDepth, const float farDepth);
void viewFrustumMultiClipCorner(const std::vector<float> &depths, const glm::mat4 &viewMat, const glm::mat4 &projMat, float *clipCorner);

static void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam) {
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return;
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

int main(){
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(FRAME_WIDTH, FRAME_HEIGHT, "rendering", nullptr, nullptr);
	if (window == nullptr){
		std::cout << "failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// load OpenGL function pointer
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glfwSetKeyCallback(window, keyCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetFramebufferSizeCallback(window, resizeGL);

	if (initializeGL() == false) {
		std::cout << "initialize GL failed\n";
		glfwTerminate();
		system("pause");
		return 0;
	}

	glEnable(GL_DEBUG_OUTPUT);
	if (glDebugMessageCallback)
		glDebugMessageCallback(debugMessageCallback, nullptr);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	// disable vsync
	glfwSwapInterval(0);

	// start game-loop
	vsyncDisabled(window);
		

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void vsyncDisabled(GLFWwindow *window) {
	double previousTimeForFPS = glfwGetTime();
	int frameCount = 0;

	static int IMG_IDX = 0;

	while (!glfwWindowShouldClose(window)) {
		// measure speed
		const double currentTime = glfwGetTime();
		frameCount = frameCount + 1;
		const double deltaTime = currentTime - previousTimeForFPS;
		if (deltaTime >= 1.0) {
			m_imguiPanel->setAvgFPS(frameCount * 1.0);
			m_imguiPanel->setAvgFrameTime(deltaTime * 1000.0 / frameCount);

			// reset
			frameCount = 0;
			previousTimeForFPS = currentTime;
		}			

		glfwPollEvents();
		paintGL();
		glfwSwapBuffers(window);
	}
}



bool initializeGL(){
	// initialize shader program
	// vertex shader
	Shader* vsShader = new Shader(GL_VERTEX_SHADER);
	vsShader->createShaderFromFile("src/shader/oglVertexShader.glsl");
	std::cout << vsShader->shaderInfoLog() << "\n";

	// fragment shader
	Shader* fsShader = new Shader(GL_FRAGMENT_SHADER);
	fsShader->createShaderFromFile("src/shader/oglFragmentShader.glsl");
	std::cout << fsShader->shaderInfoLog() << "\n";

	// shader program
	ShaderProgram* shaderProgram = new ShaderProgram();
	shaderProgram->init();
	shaderProgram->attachShader(vsShader);
	shaderProgram->attachShader(fsShader);
	shaderProgram->checkStatus();
	if (shaderProgram->status() != ShaderProgramStatus::READY) {
		return false;
	}
	shaderProgram->linkProgram();
	vsShader->releaseShader();
	fsShader->releaseShader();
	
	delete vsShader;
	delete fsShader;
	// =================================================================
	// init renderer
	defaultRenderer = new SceneRenderer();
	if (!defaultRenderer->initialize(FRAME_WIDTH, FRAME_HEIGHT, shaderProgram)) {
		return false;
	}

	// =================================================================
	// initialize camera
	godViewMat = glm::lookAt(glm::vec3(0.0, 50.0, 20.0), glm::vec3(0.0, 20.0, -10.0), glm::vec3(0.0, 1.0, 0.0));
	playerViewMat = glm::lookAt(glm::vec3(0.0, 8.0, 10.0), glm::vec3(0.0, 5.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

	const glm::vec4 directionalLightDir = glm::vec4(0.4, 0.5, 0.8, 0.0);
	
	defaultRenderer->setDirectionalLightDir(directionalLightDir);
	// =================================================================
	// initialize camera and view frustum
	infinityPlane = new InfinityPlane(2);
	defaultRenderer->appendObject(infinityPlane->sceneObject());

	viewFrustumSO = new ViewFrustumSceneObject(2, SceneManager::Instance()->m_fs_pixelProcessIdHandle, SceneManager::Instance()->m_fs_pureColor);
	defaultRenderer->appendObject(viewFrustumSO->sceneObject());

	resize(FRAME_WIDTH, FRAME_HEIGHT);
	// =================================================================	
	
	glUseProgram(0);
	// setup grass
	plants[0] = new ModelOld("assets/grassB.obj", "assets/grassB_albedo.png");
	plants[1] = new ModelOld("assets/bush01_lod2.obj", "assets/bush01.png");
	plants[2] = new ModelOld("assets/bush05_lod2.obj", "assets/bush05.png");

	model = new Model();
	model->loadModel("assets/grassB.obj");
	model->loadModel("assets/bush01_lod2.obj");
	model->loadModel("assets/bush05_lod2.obj");
	model->buildVAO();

	samples[0] = MyPoissonSample::fromFile("assets/poissonPoints_155304.ppd");
	samples[1] = MyPoissonSample::fromFile("assets/poissonPoints_1010.ppd");
	samples[2] = MyPoissonSample::fromFile("assets/poissonPoints_2797.ppd");

	program = new Program("src/shader/ssboVertexShader.glsl", "src/shader/ssboFragmentShader.glsl");
	std::cout << "Program id: " << program->ID << std::endl;
	
	std::vector<float> positions;
	for (int i = 0; i < 3; i++)
	{
		positions.insert(positions.end(), samples[i]->m_positions, samples[i]->m_positions + (samples[i]->m_numSample) * 3);
	}
	//assert(positions.size() == (samples[0]->m_numSample + samples[1]->m_numSample + samples[2]->m_numSample) * 3);
	//std::cout << "OFFSET " << samples[0]->m_positions[3] << " " << samples[0]->m_positions[4] << " " << samples[0]->m_positions[5] << std::endl;
	//std::cout << "OFFSET " << samples[1]->m_positions[3] << " " << samples[1]->m_positions[4] << " " << samples[1]->m_positions[5] << std::endl;
	//std::cout << "OFFSET " << samples[2]->m_positions[3] << " " << samples[2]->m_positions[4] << " " << samples[2]->m_positions[5] << std::endl;
	//std::cout << "OFFSET " << positions[3] << " " << positions[4] << " " << positions[5] << std::endl;
	//std::cout << "OFFSET " << positions[samples[0]->m_numSample * 3 + 3] << " " << positions[samples[0]->m_numSample * 3 + 4] << " " << positions[samples[0]->m_numSample * 3 + 5] << std::endl;
	//std::cout << "OFFSET " << positions[(samples[0]->m_numSample + samples[1]->m_numSample) * 3 + 3] << " " << positions[(samples[0]->m_numSample + samples[1]->m_numSample) * 3 + 4] << " " << positions[(samples[0]->m_numSample + samples[1]->m_numSample) * 3 + 5] << std::endl;

	// TODO ssbo is raw instance data ssbo
	GLuint rawInstanceDataBufferHandle;
	glGenBuffers(1, &rawInstanceDataBufferHandle);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rawInstanceDataBufferHandle);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, positions.size() * sizeof(float), positions.data(), GL_MAP_READ_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rawInstanceDataBufferHandle);

	GLuint validInstanceDataBufferHandle;
	glGenBuffers(1, &validInstanceDataBufferHandle);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, validInstanceDataBufferHandle);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, (samples[0]->m_numSample + samples[1]->m_numSample + samples[2]->m_numSample) * 3 * sizeof(float), nullptr, GL_MAP_READ_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, validInstanceDataBufferHandle);

	const int DRAW_COMMANDS_COUNT = 3;
	DrawElementsIndirectCommand drawCommands[DRAW_COMMANDS_COUNT];

	for (int i = 0; i < DRAW_COMMANDS_COUNT; i++)
	{
		drawCommands[i].count = model->meshes[i].count;
		drawCommands[i].instanceCount = samples[i]->m_numSample;
		drawCommands[i].firstIndex = model->meshes[i].indicesOffset;
		drawCommands[i].baseVertex = model->meshes[i].verticesOffset;
		drawCommands[i].baseInstance = 0;
		std::cout << "======================="<< std::endl;
		std::cout << "Draw command " << i << std::endl;
		std::cout << "count " << drawCommands[i].count << std::endl;
		std::cout << "instanceCount " << drawCommands[i].instanceCount << std::endl;
		std::cout << "firstIndex " << drawCommands[i].firstIndex << std::endl;
		std::cout << "baseVertex " << drawCommands[i].baseVertex << std::endl;
		std::cout << "baseInstance " << drawCommands[i].baseInstance << std::endl;
	}
	drawCommands[1].baseInstance = samples[0]->m_numSample;
	drawCommands[2].baseInstance = samples[0]->m_numSample + samples[1]->m_numSample;

	GLuint cmdBufferHandle;
	glGenBuffers(1, &cmdBufferHandle);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cmdBufferHandle);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(drawCommands), drawCommands, GL_MAP_READ_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cmdBufferHandle);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	glBindVertexArray(model->vao);
	glBindBuffer(GL_ARRAY_BUFFER, rawInstanceDataBufferHandle);
	glVertexAttribPointer(3, 3, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	// SSBO as draw-indirect-buffer
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBufferHandle);

	glBindVertexArray(0);

	m_imguiPanel = new MyImGuiPanel();	
	
	return true;
}
void resizeGL(GLFWwindow *window, int w, int h){
	resize(w, h);
}

void paintGL(){
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ===============================
	// update infinity plane with player camera
	const glm::vec3 PLAYER_VIEW_POSITION = glm::vec3(0.0, 8.0, 10.0);
	infinityPlane->updateState(playerViewMat, PLAYER_VIEW_POSITION);

	// update player camera view frustum
	viewFrustumSO->updateState(playerViewMat, PLAYER_VIEW_POSITION);

	// =============================================
	// start new frame
	defaultRenderer->setViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
	defaultRenderer->startNewFrame();

	// rendering with god view
	const int HALF_W = FRAME_WIDTH * 0.5;
	defaultRenderer->setViewport(0, 0, HALF_W, FRAME_HEIGHT);
	defaultRenderer->setProjection(godProjMat);
	defaultRenderer->setView(godViewMat);
	defaultRenderer->renderPass();
	// rendering with player view
	defaultRenderer->setViewport(HALF_W, 0, HALF_W, FRAME_HEIGHT);
	defaultRenderer->setProjection(playerProjMat);
	defaultRenderer->setView(playerViewMat);
	defaultRenderer->renderPass();
	// ===============================
	
	program->use();
	program->setInt("colorTexture", plants[0]->texture.textureID);
	glBindVertexArray(model->vao);

	glViewport(0, 0, HALF_W, FRAME_HEIGHT);
	program->setMat4("projMat", godProjMat);
	program->setMat4("viewMat", godViewMat);
	program->setMat4("modelMat", glm::mat4(1.0f));
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, 3, 0);

	glViewport(HALF_W, 0, HALF_W, FRAME_HEIGHT);
	program->setMat4("projMat", playerProjMat);
	program->setMat4("viewMat", playerViewMat);
	program->setMat4("modelMat", glm::mat4(1.0f));
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, 3, 0);

	glUseProgram(0);
	glBindVertexArray(0);

	ImGui::Begin("My name is window");
	m_imguiPanel->update();
	ImGui::End();

	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

////////////////////////////////////////////////
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){}
void cursorPosCallback(GLFWwindow* window, double x, double y){}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){}
void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {}

void updateWhenPlayerProjectionChanged(const float nearDepth, const float farDepth) {
	// get view frustum corner
	const int NUM_CASCADE = 2;
	const float HY = 0.0;

	float dOffset = (farDepth - nearDepth) / NUM_CASCADE;
	float *corners = new float[(NUM_CASCADE + 1) * 12];
	std::vector<float> depths(NUM_CASCADE + 1);
	for (int i = 0; i < NUM_CASCADE; i++) {
		depths[i] = nearDepth + dOffset * i;
	}
	depths[NUM_CASCADE] = farDepth;
	// get viewspace corners
	glm::mat4 tView = glm::lookAt(glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	// calculate corners of view frustum cascade
	viewFrustumMultiClipCorner(depths, tView, playerProjMat, corners);

	// update infinity plane
	for (int i = 0; i < NUM_CASCADE; i++) {
		float *cascadeBuffer = infinityPlane->cascadeDataBuffer(i);

		cascadeBuffer[0] = corners[((i + 1) * 4 + 1) * 3 + 0];
		cascadeBuffer[1] = HY;
		cascadeBuffer[2] = corners[((i + 1) * 4 + 1) * 3 + 2];

		cascadeBuffer[3] = corners[((i + 1) * 4 + 1) * 3 + 0];
		cascadeBuffer[4] = HY;
		cascadeBuffer[5] = corners[(i * 4 + 1) * 3 + 2];

		cascadeBuffer[6] = corners[((i + 1) * 4 + 2) * 3 + 0];
		cascadeBuffer[7] = HY;
		cascadeBuffer[8] = corners[(i * 4 + 2) * 3 + 2];

		cascadeBuffer[9] = corners[((i + 1) * 4 + 2) * 3 + 0];
		cascadeBuffer[10] = HY;
		cascadeBuffer[11] = corners[((i + 1) * 4 + 2) * 3 + 2];
	}
	infinityPlane->updateDataBuffer();

	// update view frustum scene object
	for (int i = 0; i < NUM_CASCADE + 1; i++) {
		float *layerBuffer = viewFrustumSO->cascadeDataBuffer(i);
		for (int j = 0; j < 12; j++) {
			layerBuffer[j] = corners[i * 12 + j];
		}
	}
	viewFrustumSO->updateDataBuffer();
}
void resize(const int w, const int h) {
	FRAME_WIDTH = w;
	FRAME_HEIGHT = h;

	// half for god view, half for player view
	const int HALF_W = w * 0.5;
	const double PLAYER_PROJ_FAR = 150.0;

	godProjMat = glm::perspective(glm::radians(75.0), HALF_W * 1.0 / h, 0.1, 500.0);
	playerProjMat = glm::perspective(glm::radians(45.0), HALF_W * 1.0 / h, 0.1, PLAYER_PROJ_FAR);

	defaultRenderer->resize(w, h);

	updateWhenPlayerProjectionChanged(0.1, PLAYER_PROJ_FAR);
}
void viewFrustumMultiClipCorner(const std::vector<float> &depths, const glm::mat4 &viewMat, const glm::mat4 &projMat, float *clipCorner) {
	const int NUM_CLIP = depths.size();

	// Calculate Inverse
	glm::mat4 viewProjInv = glm::inverse(projMat * viewMat);

	// Calculate Clip Plane Corners
	int clipOffset = 0;
	for (const float depth : depths) {
		// Get Depth in NDC, the depth in viewSpace is negative
		glm::vec4 v = glm::vec4(0, 0, -1 * depth, 1);
		glm::vec4 vInNDC = projMat * v;
		if (fabs(vInNDC.w) > 0.00001) {
			vInNDC.z = vInNDC.z / vInNDC.w;
		}
		// Get 4 corner of clip plane
		float cornerXY[] = {
			-1, 1,
			-1, -1,
			1, -1,
			1, 1
		};
		for (int j = 0; j < 4; j++) {
			glm::vec4 wcc = {
				cornerXY[j * 2 + 0], cornerXY[j * 2 + 1], vInNDC.z, 1
			};
			wcc = viewProjInv * wcc;
			wcc = wcc / wcc.w;

			clipCorner[clipOffset * 12 + j * 3 + 0] = wcc[0];
			clipCorner[clipOffset * 12 + j * 3 + 1] = wcc[1];
			clipCorner[clipOffset * 12 + j * 3 + 2] = wcc[2];
		}
		clipOffset = clipOffset + 1;
	}
}
