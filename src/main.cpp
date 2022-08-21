#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

#define TIMER_START 60.0

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);

unsigned int loadCubemap(vector<std::string> faces);

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

void setShader(Shader ourShader, DirLight dirLight, PointLight pointLight, SpotLight spotLight);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool spotLightOn = false;
bool pointLightOn = true;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 1.0f;
float lastFrame = 0.0f;

struct ProgramState {

    glm::vec3 clearColor = glm::vec3(0.8f,0.8f,1.0f);
    bool ImGuiEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    bool gameStart = false;
    double startTime;
    bool rose1Collected = false;
    bool rose2Collected = false;
    bool rose3Collected = false;

    int numOfMistakes = 0;

    DirLight dirLight;
    PointLight pointLight;
    SpotLight spotLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    //Skybox
    vector<std::string> faces;
    unsigned int cubemapTexture;

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
       // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    //light
    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(40.0f, -20.0f, 70.0f);
    dirLight.ambient = glm::vec3(0.2);
    dirLight.diffuse = glm::vec3(0.3, 0.1, 0.0);
    dirLight.specular = glm::vec3(0.4, 0.3, 0.2);

    PointLight& pointLight = programState->pointLight;
    pointLight.ambient = glm::vec3(0.5, 0.5, 0.5);
    pointLight.diffuse = glm::vec3(0.9, 0.9, 0.9);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.008f;
    pointLight.linear = 0.04f;
    pointLight.quadratic = 0.04f;

    SpotLight& spotLight = programState->spotLight;
    spotLight.position = programState->camera.Position;
    spotLight.direction = programState->camera.Front;
    spotLight.ambient = glm::vec3 (0.5f);
    spotLight.diffuse = glm::vec3 (0.9f);

    spotLight.specular = glm::vec3 (1.5f, 1.5f, 1.5f);
    spotLight.constant = 0.0003f;
    spotLight.linear = 0.0009f;
    spotLight.quadratic = 0.0009f;

    spotLight.cutOff = glm::cos(glm::radians(5.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(7.0f));

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/model_lighting.vs", "resources/shaders/model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader cubeShader("resources/shaders/cube.vs", "resources/shaders/cube.fs");

    // load models
    // -----------

    Model roseModel("resources/objects/rose/Models and Textures/rose.obj");
    roseModel.SetShaderTextureNamePrefix("material.");


    float vertices[] = {
            // positions          // colors           // texture coords
            0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f, // top right
            0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, // bottom right
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, // bottom left
            -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f  // top left
    };


    unsigned int indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };

    // cube vertices
    // -------------------------------------------------
    float cubeVertices[] = {
            // positions          // texture Coords
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    // Skybox vertices
    //____________________________________________________________________________________________
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // flags VAO
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));


    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    // flags textures
    unsigned int SRBflagTexture = loadTexture(FileSystem::getPath("resources/textures/flags/srb.png").c_str());
    unsigned int RUSflagTexture = loadTexture(FileSystem::getPath("resources/textures/flags/rus.png").c_str());
    unsigned int SPAflagTexture = loadTexture(FileSystem::getPath("resources/textures/flags/spa.png").c_str());
    unsigned int SADflagTexture = loadTexture(FileSystem::getPath("resources/textures/flags/usa.png").c_str());
    unsigned int BRAflagTexture = loadTexture(FileSystem::getPath("resources/textures/flags/bra.png").c_str());
    unsigned int ARGflagTexture = loadTexture(FileSystem::getPath("resources/textures/flags/arg.png").c_str());
    unsigned int CANflagTexture = loadTexture(FileSystem::getPath("resources/textures/flags/can.png").c_str());
    unsigned int MEXflagTexture = loadTexture(FileSystem::getPath("resources/textures/flags/mex.png").c_str());

    unsigned int textures[8];
    textures[0] = SRBflagTexture;
    textures[1] = RUSflagTexture;
    textures[2] = SPAflagTexture;
    textures[3] = SADflagTexture;
    textures[4] = BRAflagTexture;
    textures[5] = ARGflagTexture;
    textures[6] = CANflagTexture;
    textures[7] = MEXflagTexture;

    unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/box.png").c_str());

    // skybox textures
    programState->faces =
            {
                    FileSystem::getPath("resources/textures/skybox/right1.png"),
                    FileSystem::getPath("resources/textures/skybox/left1.png"),
                    FileSystem::getPath("resources/textures/skybox/top1.png"),
                    FileSystem::getPath("resources/textures/skybox/bottom1.png"),
                    FileSystem::getPath("resources/textures/skybox/back1.png"),
                    FileSystem::getPath("resources/textures/skybox/front1.png"),
            };

    programState->cubemapTexture = loadCubemap(programState->faces);


    cubeShader.use();
    cubeShader.setInt("texture1", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    vector<glm::vec3> lightPos {
            glm::vec3(45.0f,5.0f,-5.0f),
            glm::vec3(35.0f,0.0f,-10.0f),
            glm::vec3(30.0f,10.0f,5.0f),
            glm::vec3(55.0f,15.0f,-5.0f),
            glm::vec3(50.0f,-10.0f,0.0f),
            glm::vec3(60.0f,-10.0f,10.0f),
            glm::vec3(40.0f,-10.f,-5.0f),
            glm::vec3(60.0f,0.0f,0.0f)
    };

    vector<glm::vec3> flagPos {
            glm::vec3(40.3f, -4.8f, -11.0f), // SRB
            glm::vec3(64.2f, -9.7f, -2.5f), // RUS
            glm::vec3(49.95f,-0.05f,-8.0f), //ARG
            glm::vec3(59.7f,9.7f,-17.5f), // USA
            glm::vec3(35.35f,4.8f,-7.5f), // BRA
            glm::vec3(64.5f, -0.1f, -12.5f), // SPA// ARG
            glm::vec3(54.75f,-7.75f,-12.5f), // CAN
            glm::vec3(45.1f,-9.8f,-18.0f) // MEX
    };


    vector<glm::vec3> cubePos {
            glm::vec3(50.0f,0.0f,-10.0f), // ARG
            glm::vec3(40.0f,-5.0f,-15.0f), // SRB
            glm::vec3(35.0f,5.0f,-10.0f), // BRA
            glm::vec3(60.0f,10.0f,-20.0f), // USA
            glm::vec3(55.0f,-8.0f,-15.f), // CAN
            glm::vec3(65.0f,-10.0f,-5.0f), // RUS
            glm::vec3(45.0f,-10.f,-20.0f), // MEX
            glm::vec3(65.0f,0.0f,-15.0f) // SPA
    };

    vector<glm::vec3> rosePos {
            glm::vec3(40.0f,-5.0f,-16.0f),
            glm::vec3(65.0f,-10.0f,-5.2f),
            glm::vec3(50.0f,0.0f,-10.0f)
    };

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        setShader(ourShader, dirLight, pointLight, spotLight);

        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render - FLAGS
        glm::mat4 model = glm::mat4(1.0f);

        for(int i=0; i<flagPos.size(); i++) {
            glBindTexture(GL_TEXTURE_2D, textures[i]);

            ourShader.use();
            model = glm::mat4(1.0f);
            model = glm::translate(model, flagPos[i]);
            model = glm::scale(model, glm::vec3(1.0f));

            ourShader.setMat4("model", model);
            ourShader.setMat4("view", view);
            ourShader.setMat4("projection", projection);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }


        // render - CUBES
        for(int i=0; i<cubePos.size(); i++) {
            cubeShader.use();
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos[i]);
            model = glm::scale(model, glm::vec3(3.0f));

            cubeShader.setMat4("model", model);
            cubeShader.setMat4("view", view);
            cubeShader.setMat4("projection", projection);
            //cubes
            glBindVertexArray(cubeVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cubeTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }


        // render - ROSES
        // 1
        ourShader.use();
        glm::mat4 modelRose1 = glm::mat4(1.0f);
        modelRose1 = glm::translate(modelRose1,rosePos[0]);
        if(programState->rose1Collected) {
            modelRose1 = glm::scale(modelRose1, glm::vec3(0.05f));
            modelRose1 = glm::rotate(modelRose1, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else {
            modelRose1 = glm::scale(modelRose1, glm::vec3(0.015f));
        }
        ourShader.setMat4("model", modelRose1);
        roseModel.Draw(ourShader);

        // 2
        glm::mat4 modelRose2 = glm::mat4(1.0f);
        modelRose2 = glm::translate(modelRose2,rosePos[1]);
        if(programState->rose2Collected) {
            modelRose2 = glm::scale(modelRose2, glm::vec3(0.05f));
            modelRose2 = glm::rotate(modelRose2, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else {
            modelRose2 = glm::scale(modelRose2, glm::vec3(0.015f));
        }
        ourShader.setMat4("model", modelRose2);
        roseModel.Draw(ourShader);

        // 3
        glm::mat4 modelRose3 = glm::mat4(1.0f);
        modelRose3 = glm::translate(modelRose3,rosePos[2]);
        if(programState->rose3Collected) {
            modelRose3 = glm::scale(modelRose3, glm::vec3(0.05f));
            modelRose3 = glm::rotate(modelRose3, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else {
            modelRose3 = glm::scale(modelRose3, glm::vec3(0.015f));
        }
        ourShader.setMat4("model", modelRose3);
        roseModel.Draw(ourShader);

        // draw skybox
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);


        double xpos = programState->camera.Front.x;
        double ypos = programState->camera.Front.y;
        double zpos = programState->camera.Front.z;
        double zoom = programState->camera.Zoom;

        if ((xpos + 0.128)*(xpos + 0.128) + (ypos + 0.064)*(ypos + 0.064) + (zpos + 0.989)*(zpos + 0.989) < 0.002
            && zoom <= 7 && zoom >= 3 && programState->gameStart) {
            spotLightOn = true;
        }
        else if ((xpos - 0.208)*(xpos - 0.208) + (ypos - 0.099)*(ypos - 0.099) + (zpos + 0.972)*(zpos + 0.972) < 0.002
                 && zoom <= 8 && zoom >= 3 && programState->gameStart) {
            spotLightOn = true;
        }
        else if ((xpos - 0.66)*(xpos - 0.66) + (ypos + 0.226)*(ypos + 0.226) + (zpos + 0.715)*(zpos + 0.715) < 0.002
                 && zoom <= 7 && zoom >= 3 && programState->gameStart) {
            spotLightOn = true;
        }
        else {
            spotLightOn = false;
        }

        if (programState->ImGuiEnabled) {
            DrawImGui(programState);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    /*
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

     */

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {


    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        double time = TIMER_START;
        if(programState->gameStart && !(programState->rose1Collected && programState->rose2Collected && programState->rose3Collected)) {
            time = max(TIMER_START - glfwGetTime() + programState->startTime, 0.0);
           // std::cout << time << endl;

        }
        ImGui::Begin("European Roses");
        ImGui::Text("Timer: %f sec", time);

        if(!programState->gameStart) {
            ImGui::Text("U kutijama sa zastavama drzava iz Evrope kriju se ruze. \n"
                        "Pronadji ih, priblizi im se, i sakupi cvece. \n"
                        "Pritisni ENTER za pocetak. Imas 60 sekundi.");
        }
        else if(time == 0) {
            programState->CameraMouseMovementUpdateEnabled = false;
            ImGui::Text("Nazalost, nisi uspeo/uspela..\n"
                        "Pritsni ESC za izlazak, pa pokusaj ponovo.\n");
        }
        else {
            ImGui::Text("Pozicioniraj se na odgovarajucu kutiju tako\n"
                        "da ona otprilike zauizima ceo ekran.\n"
                        "Pritisni SPACE da pokupis ruzu.\n");

            if(programState->rose1Collected && !(programState->rose2Collected && programState->rose3Collected)) {
                ImGui::Text("BRAVO! Nasao si ruzu!\n");
            }
            if(programState->rose2Collected && !(programState->rose1Collected && programState->rose3Collected)) {
                ImGui::Text("BRAVO! Nasao si ruzu!\n");
            }
            if(programState->rose3Collected && !(programState->rose1Collected && programState->rose2Collected)) {
                ImGui::Text("BRAVO! Nasao si ruzu!\n");
            }
            if(programState->rose1Collected && programState->rose2Collected && programState->rose3Collected){
                ImGui::Text("BRAVO! Nasao si sve ruze!"
                            "Pritisni ESC za izlazak.\n");
            }
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        if(!programState->gameStart) {
            programState->startTime = glfwGetTime();
        }
        programState->gameStart = true;
        pointLightOn = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    }

    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS && programState->gameStart) {

        double xpos = programState->camera.Front.x;
        double ypos = programState->camera.Front.y;
        double zpos = programState->camera.Front.z;
        double zoom = programState->camera.Zoom;

        if ((xpos + 0.128)*(xpos + 0.128) + (ypos + 0.064)*(ypos + 0.064) + (zpos + 0.989)*(zpos + 0.989) < 0.002
            && zoom <= 7 && zoom >= 3) {
            programState->rose1Collected = true;
        }
        else if ((xpos - 0.208)*(xpos - 0.208) + (ypos - 0.099)*(ypos - 0.099) + (zpos + 0.972)*(zpos + 0.972) < 0.002
                 && zoom <= 8 && zoom >= 3) {
            programState->rose3Collected = true;
        }
        else if ((xpos - 0.66)*(xpos - 0.66) + (ypos + 0.226)*(ypos + 0.226) + (zpos + 0.715)*(zpos + 0.715) < 0.002
                 && zoom <= 7 && zoom >= 3) {
            programState->rose2Collected = true;
        }
    }

    if(programState->rose1Collected && programState->rose2Collected && programState->rose3Collected){
        pointLightOn = true;
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void setShader(Shader ourShader, DirLight dirLight, PointLight pointLight, SpotLight spotLight) {

    ourShader.use();

    ourShader.setVec3("dirLight.direction", dirLight.direction);
    ourShader.setVec3("dirLight.ambient", dirLight.ambient);
    ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
    ourShader.setVec3("dirLight.specular", dirLight.specular);

    ourShader.setInt("pointLightOn", pointLightOn);

    pointLight.position = glm::vec3(40.0f, 3.5, -13.0f);

    ourShader.setVec3("pointLight1.position", pointLight.position);
    ourShader.setVec3("pointLight1.ambient", pointLight.ambient);
    ourShader.setVec3("pointLight1.diffuse", pointLight.diffuse);
    ourShader.setVec3("pointLight1.specular", pointLight.specular);
    ourShader.setFloat("pointLight1.constant", pointLight.constant);
    ourShader.setFloat("pointLight1.linear", pointLight.linear);
    ourShader.setFloat("pointLight1.quadratic", pointLight.quadratic);
    ourShader.setVec3("viewPosition", programState->camera.Position);
    ourShader.setFloat("material.shininess", 32.0f);

    pointLight.position = glm::vec3(57.0f, 0.5f, -10.0f);

    ourShader.setVec3("pointLight2.position", pointLight.position);
    ourShader.setVec3("pointLight2.ambient", pointLight.ambient);
    ourShader.setVec3("pointLight2.diffuse", pointLight.diffuse);
    ourShader.setVec3("pointLight2.specular", pointLight.specular);
    ourShader.setFloat("pointLight2.constant", pointLight.constant);
    ourShader.setFloat("pointLight2.linear", pointLight.linear);
    ourShader.setFloat("pointLight2.quadratic", pointLight.quadratic);
    ourShader.setVec3("viewPosition", programState->camera.Position);
    ourShader.setFloat("material.shininess", 32.0f);

    pointLight.position = glm::vec3(57.0f, -3.5f, -12.0f);

    ourShader.setVec3("pointLight3.position", pointLight.position);
    ourShader.setVec3("pointLight3.ambient", pointLight.ambient);
    ourShader.setVec3("pointLight3.diffuse", pointLight.diffuse);
    ourShader.setVec3("pointLight3.specular", pointLight.specular);
    ourShader.setFloat("pointLight3.constant", pointLight.constant);
    ourShader.setFloat("pointLight3.linear", pointLight.linear);
    ourShader.setFloat("pointLight3.quadratic", pointLight.quadratic);
    ourShader.setVec3("viewPosition", programState->camera.Position);
    ourShader.setFloat("material.shininess", 32.0f);

    ourShader.setInt("spotLightOn", spotLightOn);
    ourShader.setVec3("spotLight.position", programState->camera.Position);
    ourShader.setVec3("spotLight.direction", programState->camera.Front);
    ourShader.setVec3("spotLight.ambient", spotLight.ambient);
    ourShader.setVec3("spotLight.diffuse", spotLight.diffuse);
    ourShader.setVec3("spotLight.specular", spotLight.specular);
    ourShader.setFloat("spotLight.constant", spotLight.constant);
    ourShader.setFloat("spotLight.linear", spotLight.linear);
    ourShader.setFloat("spotLight.quadratic", spotLight.quadratic);
    ourShader.setFloat("spotLight.cutOff", spotLight.cutOff);
    ourShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);

}

