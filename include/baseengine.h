#ifndef YAM_BASEENGINE_H
#define YAM_BASEENGINE_H

#include "common.h"

namespace yam
{
   class BaseEngine
   {
      private:
         SDL_Window*    window;
         GLuint         cvbo;

         uint32_t       int_flags;
         bool           window_alive;
         uint32_t       width, height;

         void*          context;

      protected:
         // Renderer stuff
         bool           shader_active;
         uint32_t       scrw, scrh;

         uint32_t       fbo_main, fbo_texture;

         renderbuffer_t rbuffer;

         wcl::string    current_shader;

         std::unordered_map<wcl::string, int32_t> shaderlist;
         std::unordered_map<wcl::string, atlas_t> atlas;
         std::unordered_map<wcl::string, texture_t> texture;
         std::unordered_map<wcl::string, surface_t> surface;

         // Controller list
         std::list<void*> controllers;
         std::unordered_map<uint16_t, bool> keys_down;

         // Internal atlasing
         std::vector<atlas_t> atlases;

         // Map from sprite name to atlas entry
   //FIXME:       std::unordered_map<wcl::string, atlas_entry_t> sprites;

         // Map from texture name to texture struct
         std::unordered_map<wcl::string, texture_t> textures;

         uint32_t       get_txc(const wcl::string& bname, wheel::rect_t& result);
         uint32_t       atlas_buffer(const wcl::string& bname, uint32_t w, uint32_t h, void* data);

      public:

         // Rendering stuff
         void           AddVertex(wheel::vertex_t vert, wheel::buffer_t* buf = nullptr);
         void           Flush(int32_t array_type = GL_TRIANGLES);

         // Shader stuff
         uint32_t       AddShader(const wcl::string& name,
                                  const wcl::string& vert,
                                  const wcl::string& frag);

         uint32_t       AddShader(const wcl::string& name,
                                  const char* vert,
                                  const char* frag);

         uint32_t       UseShader(const wcl::string& name);

         inline int32_t get_program() { return shaderlist[current_shader]; }

         // Controller stuff
         wheel::string  GetControllerTypeString(void* controller);

         // Window stuff
         uint32_t       OpenWindow(const wcl::string& title, uint32_t width, uint32_t height);
         void           SwapBuffers();

         uint32_t       GetEvents(wheel::EventList* events);

         bool           WindowIsOpen();

         uint32_t       DrawSprite(const wcl::string& sprite,
                                   uint32_t x, uint32_t y, uint32_t w = ~0, uint32_t h = ~0,
                                   int32_t pivot_x = 0, int32_t pivot_y = 0, float angle = .0f);

         // Debug & Error handling
         void           Error(errlevel_t level, const wcl::string& msg);


         bool           ok();

         BaseEngine();
        ~BaseEngine();
   };
}

#endif
