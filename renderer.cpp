#include "include/renderer.h"

namespace yam
{
   Renderer renderer;

   uint32_t Renderer::Init()
   {
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

      window = SDL_CreateWindow("Test",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                800, 600,
                                SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);

      if (window == nullptr)
      {
         log(FATAL, "Could not open window\n");

         return 1;
      }

      context = SDL_GL_CreateContext(window);

      if (context == nullptr)
      {
         log(FATAL, "Could not create OpenGL context\n");

         return 1;
      }

      glewExperimental = GL_TRUE;
      GLenum err = glewInit();

      if (GLEW_OK != err)
      {
         log(FATAL, "GLEW error: ", glewGetErrorString(err), "\n");

         return 1;
      }

      log(NOTE, "Using GLEW " , glewGetString(GLEW_VERSION), "\n");

      glViewport(0.0, 0.0, 800.0, 600.0);

      GLuint VertexArrayID;
      glGenVertexArrays(1, &VertexArrayID);
      glBindVertexArray(VertexArrayID);

      glGenBuffers(1, &rbuffer_vbo);

      glDisable(GL_MULTISAMPLE);
      glEnable(GL_DEPTH_TEST);

      glPolygonMode(GL_BACK, GL_LINE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      alive = true;

      int32_t GLMaj, GLMin;
      glGetIntegerv(GL_MAJOR_VERSION, &GLMaj);
      glGetIntegerv(GL_MINOR_VERSION, &GLMin);

      int32_t prof;
      SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &prof);

      log(NOTE, "OpenGL version: ",GLMaj,".",GLMin," ");
      if (prof == 1)
         std::cout << "core\n";
      else if (prof == 2)
         std::cout << "compatibility\n";
      else if (prof == 3)
         std::cout << "ES\n";
      else
         std::cout << "\n";

      return 0;
   }

   void Renderer::Destroy()
   {
      alive = false;
   }

   Renderer::~Renderer()
   {
      if (alive) Destroy();

      for (auto t : texture)
         glDeleteTextures(1, &t.second->id);

      for (auto s : surface)
         glDeleteTextures(1, &s.second->id);

      if (window != nullptr)
      {
         glDeleteBuffers(1, &rbuffer_vbo);
         //glDeleteFramebuffers(1, &fbo_main);
      }

   }

   uint32_t Renderer::AddShader(const wcl::string& name, Shader&& shader)
   {
      if (!(shader.Status() & Shader::COMPILED))
      {
         yam::log(yam::NOTE, "Shader '",name,"' not precompiled, compiling now...\n");

         if (shader.Compile() != WHEEL_OK)
         {
            std::cout << " - Could not compile shader, not added.\n";
            return YAM_SHADER_COMPILE_ERROR;
         }
      }
      shaderlist.insert(std::make_pair(name, std::move(shader)));

      yam::log(yam::SUCCESS, "Added new shader: '", name, "'\n");

      return WHEEL_OK;
   }

   uint32_t Renderer::UseShader(const wcl::string& name)
   {
      if (shaderlist.count(name) != 1)
      {
         std::cout << "[!] Cannot use shader ''" << name << "', it doesn't exist.\n";
      }

      shaderlist[name].Use();

      return WHEEL_OK;
   }

}
