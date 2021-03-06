//
// Created by Guy on 06/08/2017.
//

#ifndef USE_OVR
#define USE_OVR true
#endif

#include <render/OpenGLContext.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <render/IRenderProvider.hpp>
#include <utils/TextUtils.hpp>
#include <engine/OmicronEngine.hpp>
#include <engine/OmicronEngineWrapper.hpp>
#include <io/EngineLoader.hpp>
#include <render/newRender/FlatRenderer.hpp>

namespace Omicron {

    OpenGLContext::OpenGLContext(GLFWwindow* window, IRenderProvider* renderProvider) : BaseContext(window, renderProvider) {

    }

    void OpenGLContext::Init() {
        DEBUG_PRINT(SetActiveContext());

        for(int i = 0; i < MAX_KEYS; i++)
            keys[i] = false;

        DEBUG_PRINT(auto res = gladLoadGLLoader((GLADloadproc) glfwGetProcAddress));
        if(!res)
            throw std::runtime_error("Failed to initialize OpenGL context");

        printf("OpenGL context initialized\n");

        int width = 0;
        int height = 0;
        DEBUG_PRINT(glfwGetWindowSize(window, &width, &height));


//        #if USE_OVR
//        DEBUG_PRINT(renderer = new ovr::OVRRenderer(this));
//        #else
//        renderer = new OpenGLRenderer(this);
//        #endif

        SpawnRenderer();
        EngineLoader::SetRenderer(renderer);
        renderer->Init();
        SpawnCamera();
        camera->Position = {3.f, 3.f, 3.f};
        Resize(width, height);

        glfwSetWindowUserPointer(window, this);

        glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
            OpenGLContext* context = static_cast<OpenGLContext*>(glfwGetWindowUserPointer(win));

            if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
                glfwSetWindowShouldClose(win, GL_TRUE);

            if(action == GLFW_PRESS) {

                if(key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
                    int id = key - GLFW_KEY_0;
                    context->renderer->SetBufferType(id);
                }

//                if(key == GLFW_KEY_1)
//                    context->renderer->SetBufferType(1);
//                if(key == GLFW_KEY_2)
//                    context->renderer->SetBufferType(2);
//                if(key == GLFW_KEY_3)
//                    context->renderer->SetBufferType(3);
//                if(key == GLFW_KEY_4)
//                    context->renderer->SetBufferType(4);
//                if(key == GLFW_KEY_5)
//                    context->renderer->SetBufferType(5);
//                if(key == GLFW_KEY_6)
//                    context->renderer->SetBufferType(6);
//                if(key == GLFW_KEY_7)
//                    context->renderer->SetBufferType(7);
//                if(key == GLFW_KEY_8)
//                    context->renderer->SetBufferType(8);
//                if(key == GLFW_KEY_9)
//                    context->renderer->SetBufferType(9);

                if(key == GLFW_KEY_TAB) {
                    if(context->renderer->GetPolygonMode() == GL_POINT)
                        context->renderer->SetPolygonMode(GL_LINE);
                    else if(context->renderer->GetPolygonMode() == GL_LINE)
                        context->renderer->SetPolygonMode(GL_FILL);
                    else if(context->renderer->GetPolygonMode() == GL_FILL)
                        context->renderer->SetPolygonMode(GL_POINT);
                }
            }

            if(key == GLFW_KEY_LEFT_SHIFT) {
                if(action == GLFW_PRESS)
                    context->camera->MovementSpeed = 30.f;
                else if(action == GLFW_RELEASE)
                    context->camera->MovementSpeed = 3.f;
            }

            if(action == GLFW_PRESS)
                context->keys[key] = true;
            else if(action == GLFW_RELEASE)
                context->keys[key] = false;


            if(action == GLFW_PRESS && key == GLFW_KEY_R) {
                OmicronEngineWrapper* coreEngineWrapper = OmicronEngineWrapper::GetEngineWrapper("CoreEngine");
                if(coreEngineWrapper) {
                    OmicronEngine* engine = coreEngineWrapper->GetChild();
                    engine->Clear();
                    EngineLoader::LoadIntoEngine("engines/testEngine.xml", engine, [](){});
                }
            }
        });

        glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xPos, double yPos) {
            OpenGLContext* context = static_cast<OpenGLContext*>(glfwGetWindowUserPointer(win));
            if(context->firstMouse) {
                context->lastX = xPos;
                context->lastY = yPos;
                context->firstMouse = false;
            }

            float xOffset = xPos - context->lastX;
            float yOffset = context->lastY - yPos;

            context->lastX = xPos;
            context->lastY = yPos;

            context->camera->ProcessMouseMovement(xOffset, yOffset);
        });

        glfwSetScrollCallback(window, [](GLFWwindow* win, double xOffset, double yOffset) {
            OpenGLContext* context = static_cast<OpenGLContext*>(glfwGetWindowUserPointer(win));
            context->camera->ProcessMouseScroll(yOffset);
        });

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height) {
            OpenGLContext* context = static_cast<OpenGLContext*>(glfwGetWindowUserPointer(win));
            context->Resize(width, height);
        });

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void OpenGLContext::SetActiveContext() {
        glfwMakeContextCurrent(window);
    }

    void OpenGLContext::Resize(int width, int height) {
        this->windowWidth  = width;
        this->windowHeight = height;
        if(renderer)
            renderer->Resize(width, height);
        glViewport(0, 0, width, height);
    }

    void OpenGLContext::Loop(OmicronEngine* engine) {
        float delta = 0.f;
        float last = 0.f;
        float current = 0.f;

        camera->Position = {10.f, 3.f, 10.f};

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            current = glfwGetTime();
            delta = current - last;
            last = current;

            if(engine)
                engine->Step(delta);

            ProcessKeys(delta);
            Render(delta);

            std::stringstream ss;
            ss << "Approx' FPS: "
               << (1.0f / delta)
               << ", "
               << "Draw calls: "
               << GetDrawCalls();
            glfwSetWindowTitle(window, ss.str().c_str());

            glfwSwapBuffers(window);
        }

        renderer->Shutdown();
        CLEAR_PTR(renderer);

        Terminate();
    }

    void OpenGLContext::Render(float delta) {
        std::vector<RenderCommand> cmds = renderProvider->Renderables();
        renderer->Update(delta);
        renderer->Render(cmds);
    }

    void OpenGLContext::Terminate() {
        if(terminated) return;
        terminated = true;
        SetWindowShouldClose(true);
        glfwTerminate();
    }

    void OpenGLContext::ProcessKeys(float delta) {
        if(keys[GLFW_KEY_W]) camera->ProcessKeyboard(FORWARD, delta);
        if(keys[GLFW_KEY_S]) camera->ProcessKeyboard(BACKWARD, delta);
        if(keys[GLFW_KEY_A]) camera->ProcessKeyboard(LEFT, delta);
        if(keys[GLFW_KEY_D]) camera->ProcessKeyboard(RIGHT, delta);
    }

    Camera* OpenGLContext::GetCamera() {
        return camera;
    }


    size_t OpenGLContext::GetWidth() {
        return windowWidth;
    }

    size_t OpenGLContext::GetHeight() {
        return windowHeight;
    }

    void OpenGLContext::SpawnRenderer() {
        renderer = new FlatRenderer(this);
    }

    void OpenGLContext::SpawnCamera() {
        camera = new Camera();
    }

    void OpenGLContext::SetWindowShouldClose(bool shouldClose) {
        glfwSetWindowShouldClose(window, shouldClose);
    }

};