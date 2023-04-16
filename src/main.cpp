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

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);

unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 700;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool blinn = false;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    float anglex;
    float angley;
    float anglez;

    PointLight pointLight;
    PointLight dirLight;

    ProgramState()
            : camera(glm::vec3(-11.1f, 0.5f, 38.38f)) {}

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar system", NULL, NULL);
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

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader modelShader("resources/shaders/model_lighting.vs", "resources/shaders/model_lighting.fs");
    Shader lightShader("resources/shaders/model_lighting.vs", "resources/shaders/light_source.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

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

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/2/right.jpeg"),
                    FileSystem::getPath("resources/textures/skybox/2/left.jpeg"),
                    FileSystem::getPath("resources/textures/skybox/2/top.jpeg"),
                    FileSystem::getPath("resources/textures/skybox/2/bottom.jpeg"),
                    FileSystem::getPath("resources/textures/skybox/2/front.jpeg"),
                    FileSystem::getPath("resources/textures/skybox/2/back.jpeg")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);


    // load models
    // -----------
    Model sunModel("resources/objects/sun/sun.obj");
    sunModel.SetShaderTextureNamePrefix("material.");

    Model mercuryModel("resources/objects/mercury/mercury.obj");
    mercuryModel.SetShaderTextureNamePrefix("material.");

    Model venusModel("resources/objects/venus/venus.obj");
    venusModel.SetShaderTextureNamePrefix("material.");

    Model earthModel("resources/objects/earth/earth.obj");
    earthModel.SetShaderTextureNamePrefix("material.");

    Model atmosphereModel("resources/objects/earth/earth.obj");
    atmosphereModel.SetShaderTextureNamePrefix("material.");

    Model moonModel("resources/objects/moon/moon.obj");
    moonModel.SetShaderTextureNamePrefix("material.");

    Model marsModel("resources/objects/mars/mars.obj");
    marsModel.SetShaderTextureNamePrefix("material.");

    Model jupiterModel("resources/objects/jupiter/jupiter.obj");
    jupiterModel.SetShaderTextureNamePrefix("material.");

    Model saturnModel("resources/objects/saturn/13906_Saturn_v1_l3.obj");
    saturnModel.SetShaderTextureNamePrefix("material.");

    Model uranusModel("resources/objects/uranus/uranus.obj");
    uranusModel.SetShaderTextureNamePrefix("material.");

    Model neptuneModel("resources/objects/neptune/neptune.obj");
    neptuneModel.SetShaderTextureNamePrefix("material.");

//    programState->pointLight.constant = 1.0f;
//    programState->pointLight.linear = 0.09f;
//    programState->pointLight.quadratic = 0.002f;



    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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


        // sun

        lightShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 sunPos = glm::vec3(0.0f);
        float sunSize = 10.5f;
        model = glm::translate(model, sunPos);
        model = glm::scale(model, glm::vec3(sunSize));
        model = glm::rotate(model, currentFrame/4, glm::vec3(0.0f, 1.0f, 0.0f));
        lightShader.setMat4("model", model);
        sunModel.Draw(lightShader);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        modelShader.use();
        modelShader.setVec3("pointLight.ambient", glm::vec3(1.0));
        modelShader.setVec3("pointLight.diffuse", glm::vec3(0.8));
        modelShader.setVec3("pointLight.specular", glm::vec3(0.05));
        modelShader.setVec3("pointLight.position", glm::vec3(0.0f));
        modelShader.setFloat("pointLight.constant", 1.0f);
        modelShader.setFloat("pointLight.linear", 0.09f);
        modelShader.setFloat("pointLight.quadratic", 0.001f);
        modelShader.setVec3("viewPosition", programState->camera.Position);
        modelShader.setFloat("material.shininess", 128.0f);
        modelShader.setVec3("dirLight.direction", glm::vec3(0.0f));
        modelShader.setVec3("dirLight.ambient", glm::vec3(0.01));
        modelShader.setVec3("dirLight.diffuse", glm::vec3(0.02));
        modelShader.setVec3("dirLight.specular", glm::vec3(0.0));
        modelShader.setBool("blinn", blinn);
        // view/projection transformations
        modelShader.setVec3("color", glm::vec3(1.0f));
        modelShader.setFloat("alpha", 1.0f);
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        // mercury
        model = glm::mat4(1.0f);
        glm::vec3 mercuryPos = glm::vec3(sin(currentFrame/4)*13, 4.0f, cos(currentFrame/4)*13);
        float mercurySize = 1.7f;
        model = glm::translate(model, mercuryPos);
        model = glm::scale(model, glm::vec3(mercurySize));
        model = glm::rotate(model, currentFrame/3, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        mercuryModel.Draw(modelShader);

        // venus
        model = glm::mat4(1.0f);
        glm::vec3 venusPos = glm::vec3(sin(currentFrame/5)*18, 4.0f, cos(currentFrame/5)*18);
        float venusSize = 2.4f;
        model = glm::translate(model, venusPos);
        model = glm::scale(model, glm::vec3(venusSize));
        model = glm::rotate(model, -currentFrame/2, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        venusModel.Draw(modelShader);

        // earth
        model = glm::mat4(1.0f);
        glm::vec3 earthPos = glm::vec3(sin(currentFrame/6)*24.3, 4.0f, cos(currentFrame/6)*24.3);
        float earthSize = 2.3f;
        model = glm::translate(model, earthPos);
        model = glm::scale(model, glm::vec3(earthSize));
        model = glm::rotate(model, currentFrame/2, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        earthModel.Draw(modelShader);

        // moon
        model = glm::mat4(1.0f);
        glm::vec3 moonPos = glm::vec3(earthPos.x + sin(currentFrame*2)*2.85, 4.5f, earthPos.z + cos(currentFrame*2)*2.85);
        float moonSize = 0.3f;
        model = glm::translate(model, moonPos);
        model = glm::scale(model, glm::vec3(moonSize));
        model = glm::rotate(model, currentFrame/4, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        moonModel.Draw(modelShader);

        // mars
        model = glm::mat4(1.0f);
        glm::vec3 marsPos = glm::vec3(sin(currentFrame/7)*30, 4.0f, cos(currentFrame/7)*30);
        float marsSize = 2.2f;
        model = glm::translate(model, marsPos);
        model = glm::scale(model, glm::vec3(marsSize));
        model = glm::rotate(model, currentFrame/2, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        marsModel.Draw(modelShader);

        // jupiter
        model = glm::mat4(1.0f);
        glm::vec3 jupiterPos = glm::vec3(sin(currentFrame/8)*37, 4.0f, cos(currentFrame/8)*37);
        float jupiterSize = 3.7f;
        model = glm::translate(model, jupiterPos);
        model = glm::scale(model, glm::vec3(jupiterSize));
        model = glm::rotate(model, currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        jupiterModel.Draw(modelShader);

        // saturn
        model = glm::mat4(1.0f);
        glm::vec3 saturnPos = glm::vec3(sin(currentFrame/9)*44, 4.0f, cos(currentFrame/9)*44);
        float saturnSize = 0.01f;
        model = glm::translate(model, saturnPos);
        model = glm::scale(model, glm::vec3(saturnSize));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//        model = glm::rotate(model, glm::radians(programState->angley), glm::vec3(0.0f, 1.0f, 0.0f));
//        model = glm::rotate(model, glm::radians(programState->anglez), glm::vec3(0.0f, 0.0f, 1.0f));
        modelShader.setMat4("model", model);
        saturnModel.Draw(modelShader);

        // uranus
        model = glm::mat4(1.0f);
        glm::vec3 uranusPos = glm::vec3(sin(currentFrame/10)*49.6, 4.0f, cos(currentFrame/10)*49.6);
        float uranusSize = 2.5f;
        model = glm::translate(model, uranusPos);
        model = glm::scale(model, glm::vec3(uranusSize));
        model = glm::rotate(model, currentFrame/2, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        uranusModel.Draw(modelShader);

        // neptune
        model = glm::mat4(1.0f);
        glm::vec3 neptunePos = glm::vec3(sin(currentFrame/11)*56, 4.0f, cos(currentFrame/11)*56);
        float neptuneSize = 2.6f;
        model = glm::translate(model, neptunePos);
        model = glm::scale(model, glm::vec3(neptuneSize));
        model = glm::rotate(model, currentFrame/2, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.setMat4("model", model);
        neptuneModel.Draw(modelShader);

        //atmosphere
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        modelShader.setVec3("color", glm::vec3(0.53f, 0.65f, 0.81f));
        modelShader.setFloat("alpha", 0.1f);

        model = glm::mat4(1.0f);
        glm::vec3 atmospherePos = glm::vec3(sin(currentFrame/6)*24.3, 4.0f, cos(currentFrame/6)*24.3);
        float atmosphereSize = 2.4f;
        model = glm::translate(model, atmospherePos);
        model = glm::scale(model, glm::vec3(atmosphereSize));
        modelShader.setMat4("model", model);
        atmosphereModel.Draw(modelShader);

        model = glm::mat4(1.0f);
        atmospherePos = glm::vec3(sin(currentFrame/5)*18, 4.0f, cos(currentFrame/5)*18);
        atmosphereSize = 2.5f;
        model = glm::translate(model, atmospherePos);
        model = glm::scale(model, glm::vec3(atmosphereSize));
        modelShader.setVec3("color", glm::vec3(0.78f, 0.5f, 0.06f));
        modelShader.setMat4("model", model);
        atmosphereModel.Draw(modelShader);

        glDisable(GL_CULL_FACE);


        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime*7);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime*7);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime*7);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime*7);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime*7);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime*7);
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
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
//
        ImGui::SliderFloat("anglex", &programState->anglex, -360, 360);
        ImGui::SliderFloat("angley", &programState->angley, -360, 360);
        ImGui::SliderFloat("anglez", &programState->anglez, -360, 360);

//        ImGui::DragFloat3("Sun position", (float*)&programState->sunPosition);
//        ImGui::DragFloat("Sun scale", &programState->sunScale, 0.05, 0.1, 4.0);
//
//        ImGui::DragFloat3("Moon position", (float*)&programState->moonPosition);
//        ImGui::DragFloat("Moon scale", &programState->moonScale, 0.05, 0.1, 4.0);
//
//        ImGui::DragFloat3("pointLight.constant", (float*)&programState->pointLight.constant);
//        ImGui::DragFloat3("pointLight.linear", (float*)&programState->pointLight.linear);
        ImGui::DragFloat3("pointLight.quadratic", (float*)&programState->pointLight.quadratic);
//
//        ImGui::DragFloat3("dirLight.ambient", (float*)&programState->dirLight.ambient);
//        ImGui::DragFloat3("dirLight.diffuse", (float*)&programState->dirLight.diffuse);
//        ImGui::DragFloat3("dirLight.specular", (float*)&programState->dirLight.specular);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
        programState->CameraMouseMovementUpdateEnabled = !programState->CameraMouseMovementUpdateEnabled;
        std::cout << "Camera lock - " << (programState->CameraMouseMovementUpdateEnabled ? "Disabled" : "Enabled") << '\n';
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS){
        blinn = !blinn;
    }
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

// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
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
