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

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

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
    glm::vec3 shipPosition = glm::vec3(-4.1f, 2.141f, 6.6f);
//    glm::vec3 mastiffPosition = glm::vec3(3.0f, 0.0f, 9.0f);
    glm::vec3 corgiPosition = glm::vec3(3.0f, 0.0f, 9.0f);
    glm::vec3 treePosition = glm::vec3(-7.0f, 0.0f, 1.0f);
    glm::vec3 cartPosition = glm::vec3(-5.0f, 0.0f, 8.0f);

    float shipScale = 0.05f;
//    float mastiffScale = 0.05f;
    float corgiScale = 0.1f;
    float treeScale = 0.25f;
    float cartScale = 0.035f;

    glm::vec3 corgiRotation = glm::vec3(-2.6f, -7.8f, -7.7f);
    float corgiAngle = 155.450f;
    glm::vec3 shipRotation = glm::vec3(5.7f, -173.5f, -4.8f);
    float shipAngle = 90.5f;
//    glm::vec3 mastiffRotation = glm::vec3(1.0f, 1.0f, 1.0f);
//    float mastiffAngle = 1.0f;

    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

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

unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
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
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader transparentShader("resources/shaders/3.1.blending.vs", "resources/shaders/3.1.blending.fs");

    unsigned int grassTexture = loadTexture(FileSystem::getPath("resources/textures/1601.m10.i311.n029.S.c10.164511620 Seamless green grass vector pattern.jpg").c_str(), true);
    ourShader.use();
    ourShader.setInt("texture1", 0);

    // load models
    // -----------
    Model shipModel("resources/objects/ship/StMaria.obj");
    shipModel.SetShaderTextureNamePrefix("material.");

//    Model mastiffModel("resources/objects/mastiff/13458_Bullmastiff_v1_L3.obj");
//    mastiffModel.SetShaderTextureNamePrefix("material.");

    Model corgiModel("resources/objects/corgi/corgi.obj");
    corgiModel.SetShaderTextureNamePrefix("material.");

    Model treeModel("resources/objects/tree/Tree.obj");
    treeModel.SetShaderTextureNamePrefix("material.");

    Model cartModel("resources/objects/cart/Cart.obj");
    cartModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 15.0, 2.0);
    pointLight.ambient = glm::vec3(1.1, 1.1, 1.1);
    pointLight.diffuse = glm::vec3(0.2, 0.2, 0.2);
    pointLight.specular = glm::vec3(0.01, 0.01, 0.01);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    float planeVertices[] = {
            // positions            // normals         // texcoords
            1000.0f, -0.5f,  1000.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
            -1000.0f, -0.5f,  1000.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -1000.0f, -0.5f, -1000.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

            1000.0f, -0.5f,  1000.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
            -1000.0f, -0.5f, -1000.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
            1000.0f, -0.5f, -1000.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
    };

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  1.00f,  0.0f,  0.0f,  0.0f,
            0.0f, 0.00f,  0.0f,  0.0f,  1.0f,
            1.0f, 0.00f,  0.0f,  1.0f,  1.0f,

            0.0f,  1.0,  0.0f,  0.0f,  0.0f,
            1.0f, 0.0f,  0.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  0.0f,  1.0f,  0.0f
    };

    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

// transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/clipart974955.png").c_str(), false);

    vector<glm::vec3> vegetation
    {
        glm::vec3(-7.5f, -0.1f, -5.0f),
        glm::vec3( 3.0f, -0.1f, 0.0f),
        glm::vec3( 0.0f, -0.1f, 8.5f),
        glm::vec3(-0.3f, -0.1f, -10.0f),
    };

    transparentShader.use();
    transparentShader.setInt("texture1", 0);

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

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        pointLight.position = glm::vec3(4.0, 4.0f, 4.0);
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render ship
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->shipPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->shipScale));    // it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, glm::radians(programState->shipAngle), programState->shipRotation);
        ourShader.setMat4("model", model);
        shipModel.Draw(ourShader);

        // render mastiff
//        model = glm::mat4(1.0f);
//        model = glm::translate(model,
//                               programState->mastiffPosition); // translate it down so it's at the center of the scene
//        model = glm::scale(model, glm::vec3(programState->mastiffScale));    // it's a bit too big for our scene, so scale it down
//        model = glm::rotate(model, glm::radians(programState->mastiffAngle), programState->mastiffRotation);
//        ourShader.setMat4("model", model);
//        mastiffModel.Draw(ourShader);

        // render corgi
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->corgiPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->corgiScale));
        model = glm::rotate(model, glm::radians(programState->corgiAngle), programState->corgiRotation);
        ourShader.setMat4("model", model);
        corgiModel.Draw(ourShader);

        // render cart
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->cartPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->cartScale));
//        model = glm::rotate(model, glm::radians(programState->corgiAngle), programState->corgiRotation);
        ourShader.setMat4("model", model);
        cartModel.Draw(ourShader);

        glBindVertexArray(planeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // render bush
        transparentShader.use();
        transparentShader.setMat4("projection", projection);
        transparentShader.setMat4("view", view);
        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            transparentShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // render tree
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->treePosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->treeScale));
//        model = glm::rotate(model, glm::radians(programState->corgiAngle), programState->corgiRotation);
        ourShader.setMat4("model", model);
        treeModel.Draw(transparentShader);


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
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(CAMERA_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(CAMERA_DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(CAMERA_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(CAMERA_RIGHT, deltaTime);
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
        ImGui::DragFloat3("Ship position", (float*)&programState->shipPosition);
        ImGui::DragFloat("Ship scale", &programState->shipScale, 0.05, 0.1, 4.0);\
        ImGui::DragFloat3("Ship rotation", (float *) &programState->shipRotation, 0.1);
        ImGui::DragFloat("Ship angle", &programState->shipAngle, 0.05, -360.0, 360.0);

//        ImGui::DragFloat3("Mastiff position", (float*)&programState->mastiffPosition);
//        ImGui::DragFloat("Mastiff scale", &programState->mastiffScale, 0.05, 0.1, 4.0);
//        ImGui::DragFloat3("Mastiff rotation", (float *) &programState->mastiffRotation, 0.1);
//        ImGui::DragFloat("Mastiff angle", &programState->mastiffAngle, 0.05, -180.0, 180.0);

        ImGui::DragFloat3("Corgi position", (float*)&programState->corgiPosition);
        ImGui::DragFloat("Corgi scale", &programState->corgiScale, 0.05, 0.1, 4.0);
        ImGui::DragFloat3("Corgi rotation", (float *) &programState->corgiRotation, 0.1);
        ImGui::DragFloat("Corgi angle", &programState->corgiAngle, 0.05, -180.0, 180.0);

        ImGui::DragFloat3("Tree position", (float*)&programState->treePosition);
        ImGui::DragFloat("Tree scale", &programState->treeScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat3("Cart position", (float*)&programState->cartPosition);
        ImGui::DragFloat("Cart scale", &programState->cartScale, 0.05, -0.5, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
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
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        programState->CameraMouseMovementUpdateEnabled = !programState->CameraMouseMovementUpdateEnabled;
    }
}
