#ifndef YAM_RENDERER
#define YAM_RENDERER

#include "shader.h"
#include "common.h"
#include "debug.hpp"

namespace yam
{
   class Renderer
   {
      private:
         SDL_Window* window;
         void*       context;

         bool        alive;

         GLuint      rbuffer_vbo;

         wcl::string    current_shader;

         std::unordered_map<wcl::string, Shader> shaderlist;

         std::unordered_map<wcl::string, atlas_t*> atlas;
         std::unordered_map<wcl::string, texture_t*> texture;
         std::unordered_map<wcl::string, surface_t*> surface;

      public:
         uint32_t AddShader(const wcl::string& name, Shader&& shader);
         uint32_t UseShader(const wcl::string& name);

         uint32_t CreateTexture();
         uint32_t DeleteTexture();

         uint32_t Init();
         void     Destroy();

         Renderer() : window(nullptr), context(nullptr), alive(false) {}
         ~Renderer();

         inline bool Alive() { return alive; }
         inline void Swap() { SDL_GL_SwapWindow(window); }
   };

   extern Renderer renderer;
}

#endif
