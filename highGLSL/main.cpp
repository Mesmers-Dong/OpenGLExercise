#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "../stb_image.h"
#include "../shader.h"
#include "../camera.h"

#define WIDTH 800
#define HEIGHT 600

void processInput(GLFWwindow *);

void frameCallback(GLFWwindow *, int, int);

void mouseMoveCallback(GLFWwindow *, double, double);

void mouseScrollCallback(GLFWwindow *, double, double);

unsigned int loadImage(const char *);

unsigned int loadCubemap(const char *skyFaces[]);

void bindSpotLight(Shader &, int, mat4);

Camera camera;

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPEN_FORWARD_COMPAT,GL_TRUE);
#endif
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "High OpenGL", NULL, NULL);
    if (window == nullptr) {
        printf("%s", "fail to create GLFW Window");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, frameCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        printf("%s", "load loader fail");
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    //开启面剔除
//    glEnable(GL_CULL_FACE);
//    glCullFace(GL_FRONT);
//    glFrontFace(GL_CCW);

    Shader shaderr("../vertex.glsl", "../fragr.glsl");
    Shader shaderg("../vertex.glsl", "../fragg.glsl");
    Shader shaderb("../vertex.glsl", "../fragb.glsl");
    Shader shadery("../vertex.glsl", "../fragy.glsl");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float cubeVertices[] = {
            // positions
            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, 0.5f, -0.5f,
            0.5f, 0.5f, -0.5f,
            -0.5f, 0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f, 0.5f,
            0.5f, -0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
            -0.5f, 0.5f, 0.5f,
            -0.5f, -0.5f, 0.5f,

            -0.5f, 0.5f, 0.5f,
            -0.5f, 0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, 0.5f,
            -0.5f, 0.5f, 0.5f,

            0.5f, 0.5f, 0.5f,
            0.5f, 0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,

            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, 0.5f,
            0.5f, -0.5f, 0.5f,
            -0.5f, -0.5f, 0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, 0.5f, -0.5f,
            0.5f, 0.5f, -0.5f,
            0.5f, 0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
            -0.5f, 0.5f, 0.5f,
            -0.5f, 0.5f, -0.5f,
    };

    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);

    unsigned int uboMatrices;
    glGenBuffers(1, &uboMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(mat4));

    shaderr.setBlockBindingIndex("Matrices", 0);
    shaderg.setBlockBindingIndex("Matrices", 0);
    shaderb.setBlockBindingIndex("Matrices", 0);
    shadery.setBlockBindingIndex("Matrices", 0);

//    glDepthFunc(GL_LEQUAL);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.3f, 0.2f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model;
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom),
                                                (float) WIDTH / (float) HEIGHT, 0.1f, 100.0f);

        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), glm::value_ptr(view));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), glm::value_ptr(projection));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // cubes
        shaderr.use();
        glBindVertexArray(cubeVAO);
        model = glm::mat4();
        model = glm::translate(model, vec3(-0.75f, 0.75f, 0));
        shaderr.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        shaderg.use();
        glBindVertexArray(cubeVAO);
        model = glm::mat4();
        model = glm::translate(model, vec3(0.75f, 0.75f, 0));
        shaderg.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        shaderb.use();
        glBindVertexArray(cubeVAO);
        model = glm::mat4();
        model = glm::translate(model, vec3(-0.75f, -0.75f, 0));
        shaderb.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        shadery.use();
        glBindVertexArray(cubeVAO);
        model = glm::mat4();
        model = glm::translate(model, vec3(0.75f, -0.75f, 0));
        shadery.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

bool first = true;
double lastXPos, lastYPos;

void bindSpotLight(Shader &shader, int vao, mat4 projection) {
    shader.use();
    shader.setMat4("view", camera.getViewMatrix());
    shader.setMat4("projection", projection);
    mat4 model;
    model = translate(model, glm::vec3(1.2f, 1.0f, 2.0f));
    model = scale(model, vec3(0.2));
    shader.setMat4("model", model);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void mouseMoveCallback(GLFWwindow *window, double xPos, double yPos) {
    if (first) {
        first = false;
        lastXPos = xPos;
        lastYPos = yPos;
    }
    double xOffset = xPos - lastXPos;
    double yOffset = lastYPos - yPos;
    lastXPos = xPos;
    lastYPos = yPos;
    camera.moveMouse(xOffset, yOffset);
}

void mouseScrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
    camera.scrollMouse(yOffset);
}

double lastTime;

void processInput(GLFWwindow *window) {
    double currentTime = glfwGetTime();
    double deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W)) {
        camera.moveKey(CameraMovement::FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S)) {
        camera.moveKey(CameraMovement::DOWN, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A)) {
        camera.moveKey(CameraMovement::LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D)) {
        camera.moveKey(CameraMovement::RIGHT, deltaTime);
    }
}

void frameCallback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

unsigned int loadImage(const char *path) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum gLenum;
        if (nrChannels == 1) {
            gLenum = GL_RED;
        } else if (nrChannels == 3) {
            gLenum = GL_RGB;
        } else if (nrChannels == 4) {
            gLenum = GL_RGBA;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, gLenum, width, height, 0, gLenum, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(data);
    return texture;
}

unsigned int loadCubemap(const char *skyFaces[]) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    for (unsigned int i = 0; i < 6; ++i) {
        printf("%s\n", skyFaces[i]);
        unsigned char *data = stbi_load(skyFaces[i], &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
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