#ifndef YAM_BASEENGINE_H
#define YAM_BASEENGINE_H

#include "common.h"
#include "shader.h"
#include "renderer.h"

namespace yam
{
   class BaseEngine
   {
      private:
         uint32_t          int_flags;
         uint32_t          width, height;

         void*             context;

         uint64_t          frame;

         // Timing, synch

         timepoint_t t_current;
         timepoint_t t_previous;

         double t_elapsed;
         double t_delay;

         static constexpr double update_interval = 1000.0 / 60.0;

         bool   t_active;

      protected:
         // Renderer stuff
         bool           shader_active;
         uint32_t       scrw, scrh;

         uint32_t       fbo_main, fbo_texture;

         renderbuffer_t rbuffer;

         // Event list
         wheel::EventList events;

         // Controller list
         std::list<void*> controllers;
         std::unordered_map<uint16_t, bool> keys_down;

         // Internal atlasing
         std::vector<atlas_t> atlases;

         // Map from sprite name to atlas entry
   //FIXME:       std::unordered_map<wcl::string, atlas_entry_t> sprites;

         // Map from texture name to texture struct
         std::unordered_map<wcl::string, texture_t> textures;

         uint32_t          get_txc(const wcl::string& bname, wheel::rect_t& result);
         uint32_t          atlas_buffer(const wcl::string& bname,
                                        uint32_t w, uint32_t h,
                                        void* data);

      public:

         // Strange stuff
         const uint64_t*   get_frameptr() { return &frame; }

         // Rendering stuff FIXME: move to renderer
         void              AddVertex(wheel::vertex_t vert, wheel::buffer_t* buf = nullptr);
         void              Flush(int32_t array_type = GL_TRIANGLES);
         void              Render();

         // Shader stuff FIXME: move to renderer
         uint32_t          AddShader(const wcl::string& name, Shader&& shader);
         uint32_t          UseShader(const wcl::string& name);

         // Controller stuff
         wheel::string     GetControllerTypeString(void* controller);

         // Window stuff
         uint32_t          OpenWindow(const wcl::string& title, uint32_t width, uint32_t height);
         void              SwapBuffers();

         uint32_t          GetEvents(wheel::EventList* events);

         bool              WindowIsOpen();

         uint32_t          DrawSprite(const wcl::string& sprite,
                                      uint32_t x, uint32_t y, uint32_t w = ~0, uint32_t h = ~0,
                                      int32_t pivot_x = 0, int32_t pivot_y = 0, float angle = .0f);

         bool              ok();

         // Main game loop
         bool              Run();

         // Update everything
         void              Update() {}

         BaseEngine();
        ~BaseEngine();
   };
}

#endif
