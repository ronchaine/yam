// clang++ `sdl2-config --cflags` -I./deps/wheel/include --std=c++11 game.cpp

#include <wheel.h>
#include <wheel_atlas.h>

#include <wheel_extras_font.h>

#include "include/defaultshaders.h"

// Begin OpenAL stuff
// Adds dependency to OpenAL
#include <AL/al.h>
// End OpenAL stuff

// Begin OpenGL stuff
// Adds dependencies to OpenGL, GLEW, GLM

// Adds dependency to SDL
#include <SDL.h>

#ifdef __APPLE__
   #include <OpenGL/glew.h>
   #include <OpenGL/gl.h>
#else
   #include <GL/glew.h>
   #include <GL/gl.h>
#endif

#include <glm/glm.hpp>

#define YAM_CLEAR_FLAGS 0x00000000

#define YAM_ERROR       0x01

#define YAM_VBUF_SIZE   1024

namespace yam
{
   struct atlas_t
   {
      wheel::Atlas      atlas;
      wheel::string     texture;

      std::unordered_map<wcl::string, wheel::rect_t> stored;
   };

   struct texture_t
   {
      uint32_t          id;

      uint32_t          w;
      uint32_t          h;
      uint32_t          channels;
      uint32_t          format;
   };

   struct surface_t
   {
      uint32_t id;
      std::vector<texture_t> attachment;
   };

   struct vertex_cache_t
   {
      std::set<wcl::string>      stored;
      std::vector<wcl::buffer_t> buffer;
   };

   struct renderbuffer_t
   {
      vertex_cache_t    cache;
      wheel::buffer_t   current;
      GLuint            cvbo;

      renderbuffer_t() : cvbo(0) {}
   };

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

         bool           ok();

         BaseEngine();
        ~BaseEngine();
   };

   // End OpenGL stuff
   bool BaseEngine::ok()
   {
      return !(int_flags && YAM_ERROR);
   }

   BaseEngine::BaseEngine()
   {
      int_flags = YAM_CLEAR_FLAGS;

      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC))
      {
         std::cout << "Couldn't init SDL: " << SDL_GetError() << "\n";
         int_flags |= YAM_ERROR;
      }

      shader_active = false;

      window = nullptr;

      window_alive = false;

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
         std::cout << "failed to open window\n";
         int_flags |= YAM_ERROR;
      }

      context = SDL_GL_CreateContext(window);

      if (context == nullptr)
      {
         std::cout << "failed to create opengl context\n";
         int_flags |= YAM_ERROR;
      }

      glewExperimental = GL_TRUE;
      GLenum err = glewInit();

      if (GLEW_OK != err)
      {
         fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      }
      fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

      glViewport(0.0, 0.0, 800.0, 600.0);
      width = 800; height = 600;

      GLuint VertexArrayID;
      glGenVertexArrays(1, &VertexArrayID);
      glBindVertexArray(VertexArrayID);

      glGenBuffers(1, &rbuffer.cvbo);

      glDisable(GL_MULTISAMPLE);
      glDisable(GL_DEPTH_TEST);
      glPolygonMode(GL_BACK, GL_LINE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      rbuffer.current.reserve(YAM_VBUF_SIZE);

      window_alive = true;

      int32_t GLMaj, GLMin;
      glGetIntegerv(GL_MAJOR_VERSION, &GLMaj);
      glGetIntegerv(GL_MINOR_VERSION, &GLMin);

      int32_t prof;
      SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &prof);

      std::cout << "OpenGL version: " << GLMaj << "." << GLMin << " ";
      if (prof == 1)
         std::cout << "core\n";
      else if (prof == 2)
         std::cout << "compatibility\n";
      else if (prof == 3)
         std::cout << "ES\n";
      else
         std::cout << "\n";

      if (!SDL_NumJoysticks())
         std::cout << "No controllers found (" << SDL_NumJoysticks() << ")\n";

      for (int i = 0; i < SDL_NumJoysticks(); ++i)
      {
         SDL_Joystick* joy = SDL_JoystickOpen(i);
         bool add = true;

         for (auto c : controllers)
         {
            if (SDL_JoystickInstanceID((SDL_Joystick*)c) == SDL_JoystickInstanceID(joy))
            {
               add = false;
               break;
            }
         }

         if (add) controllers.push_back(joy);
         int instance = SDL_JoystickInstanceID(joy);

         std::cout << "controller: " << SDL_JoystickName(joy) << " instance " << instance << "\n";
      }
   }

   BaseEngine::~BaseEngine()
   {
      for (auto t : texture)
         glDeleteTextures(1, &t.second.id);

      for (auto s : surface)
         glDeleteTextures(1, &s.second.id);

      if (window != nullptr)
      {
         glDeleteBuffers(1, &rbuffer.cvbo);
         glDeleteFramebuffers(1, &fbo_main);
      }

      SDL_Quit();
   }

   bool BaseEngine::WindowIsOpen()
   {
      return window_alive;
   }

   void BaseEngine::AddVertex(wheel::vertex_t vert, wheel::buffer_t* buf)
   {
      if (buf == nullptr)
         buf = &(rbuffer.current);

      if (buf->pos() > buf->capacity() - 24)
         Flush();

      buf->write<float>(vert.x0);
      buf->write<float>(vert.y0);

      buf->write<uint16_t>(vert.s0);
      buf->write<uint16_t>(vert.t0);

      buf->write<uint8_t>(vert.r);
      buf->write<uint8_t>(vert.g);
      buf->write<uint8_t>(vert.b);
      buf->write<uint8_t>(vert.a);

      return;
   }

   void BaseEngine::Flush(int32_t array_type)
   {
      size_t rsize = rbuffer.current.size();

      glBindBuffer(GL_ARRAY_BUFFER, rbuffer.cvbo);
      glBufferData(GL_ARRAY_BUFFER, rsize,
                   &rbuffer.current[0], GL_DYNAMIC_DRAW);

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glEnableVertexAttribArray(2);

      glBindBuffer(GL_ARRAY_BUFFER, rbuffer.cvbo);

      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(wheel::vertex_t), (void*)0);
      glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(wheel::vertex_t), (void*)(2*sizeof(float)));
      glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(wheel::vertex_t), (void*)(4*sizeof(float)));

      glDrawArrays(array_type, 0, rsize / sizeof(wheel::vertex_t));

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
      glDisableVertexAttribArray(2);

      rbuffer.current.clear();
      rbuffer.current.seek(0);

      return;
   }

   void BaseEngine::SwapBuffers()
   {
      SDL_GL_SwapWindow(window);
   }

   inline uint32_t BaseEngine::GetEvents(wheel::EventList* events)
   {
      SDL_Event sdlevent;
      while(SDL_PollEvent(&sdlevent))
      {
         wheel::Event newevent;

         if (sdlevent.type == SDL_WINDOWEVENT)
         {
            switch (sdlevent.window.event)
            {
               case SDL_WINDOWEVENT_CLOSE:
                  window_alive = false;
                  continue;
            }
         } else if (sdlevent.type == SDL_MOUSEMOTION) {
            newevent.data.push_back(WHEEL_EVENT_MOUSE);
            newevent.data.push_back(WHEEL_MOUSE_POSITION);

            newevent.data.write<uint32_t>(sdlevent.motion.x);
            newevent.data.write<uint32_t>(sdlevent.motion.y);
            newevent.data.write<uint32_t>(sdlevent.motion.xrel);
            newevent.data.write<uint32_t>(sdlevent.motion.yrel);
            newevent.data.write<uint32_t>(sdlevent.motion.state);
         } else if (sdlevent.type == SDL_MOUSEBUTTONDOWN) {
            newevent.data.push_back(WHEEL_EVENT_MOUSE);
            newevent.data.push_back(WHEEL_PRESS);
            newevent.data.push_back(sdlevent.button.button);
            newevent.data.push_back(sdlevent.button.clicks);
         } else if (sdlevent.type == SDL_MOUSEBUTTONUP) {
            newevent.data.push_back(WHEEL_EVENT_MOUSE);
            newevent.data.push_back(WHEEL_RELEASE);
            newevent.data.push_back(sdlevent.button.button);
            newevent.data.push_back(sdlevent.button.clicks);
         } else if (sdlevent.type == SDL_KEYUP) {
            keys_down[sdlevent.key.keysym.scancode] = false;
            newevent.data.push_back(WHEEL_EVENT_KEYBOARD);
            newevent.data.push_back(WHEEL_RELEASE);
            newevent.data.write<uint16_t>(sdlevent.key.keysym.scancode);
         } else if (sdlevent.type == SDL_KEYDOWN) {
            if (keys_down[sdlevent.key.keysym.scancode] == true)
               continue;
            keys_down[sdlevent.key.keysym.scancode] = true;

            newevent.data.push_back(WHEEL_EVENT_KEYBOARD);
            newevent.data.push_back(WHEEL_PRESS);
            newevent.data.write<uint16_t>(sdlevent.key.keysym.scancode);
         } else if (sdlevent.type == SDL_JOYDEVICEADDED) {
         } else if (sdlevent.type == SDL_JOYDEVICEREMOVED) {
         } else if (sdlevent.type == SDL_JOYHATMOTION) {
            // type, controller id, evtype, hat #, direction
            newevent.data.push_back(WHEEL_EVENT_CONTROLLER);
            SDL_JoyHatEvent what = sdlevent.jhat;
            newevent.data.push_back((uint8_t)what.which);
            newevent.data.push_back(WHEEL_HAT_POSITION);
            newevent.data.push_back((uint8_t)what.hat);

            for (auto c : controllers)
            {
               SDL_Joystick* joy = (SDL_Joystick*) c;
               if (SDL_JoystickInstanceID(joy) == what.which)
                  newevent.data.push_back(SDL_JoystickGetHat(joy, what.hat));
            }
         } else if (sdlevent.type == SDL_JOYBUTTONDOWN) {
            // type, controller id, evtype, button
            newevent.data.push_back(WHEEL_EVENT_CONTROLLER);
            SDL_ControllerButtonEvent what = sdlevent.cbutton;
            newevent.data.push_back((uint8_t)what.which);
            newevent.data.push_back(WHEEL_PRESS);
            newevent.data.push_back((uint8_t)what.button);
         } else if (sdlevent.type == SDL_JOYBUTTONUP) {
            // type, controller id, evtype, button
            newevent.data.push_back(WHEEL_EVENT_CONTROLLER);
            SDL_ControllerButtonEvent what = sdlevent.cbutton;
            newevent.data.push_back((uint8_t)what.which);
            newevent.data.push_back(WHEEL_RELEASE);
            newevent.data.push_back((uint8_t)what.button);
         } else if (sdlevent.type == SDL_JOYAXISMOTION) {
            newevent.data.push_back(WHEEL_EVENT_CONTROLLER);
            SDL_ControllerAxisEvent what = sdlevent.caxis;
            newevent.data.push_back((uint8_t)what.which);
            newevent.data.push_back(WHEEL_RELEASE);
            newevent.data.push_back((uint8_t)what.axis);
            newevent.data.write<uint16_t>(what.value);
         }
      }

      return WHEEL_OK;
   }
}

int main(int argc, char* argv[])
{
   yam::BaseEngine* game = new yam::BaseEngine();

   wheel::EventList ev;

   if (!game->ok())
   {
      std::cout << "error in initialisation\n";
      delete game;

      return 1;
   } else {
      std::cout << "Engine init successful\n";
   }

   while(game->WindowIsOpen())
   {
      game->GetEvents(&ev);
      game->SwapBuffers();
   }

   delete game;
   return 0;
}
