#include "include/defaultshaders.h"
#include "include/baseengine.h"
#include "include/console.h"

#include "include/argparser.hpp"

namespace yam
{
   OutputTarget log;

   bool BaseEngine::ok()
   {
      return !(int_flags && YAM_ERROR);
   }

   BaseEngine::BaseEngine()
   {
      int_flags = YAM_CLEAR_FLAGS;

      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC))
      {
         yam::log(yam::ERROR, "Couldn't init SDL: '%s'\n", SDL_GetError());
         int_flags |= YAM_ERROR;
      }

      shader_active = false;

      if (renderer.Init())
         int_flags |= YAM_ERROR;

      if (!SDL_NumJoysticks())
         log(NOTE, "No controllers found (", SDL_NumJoysticks(), " reported by SDL)\n");

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

         log(NOTE, "controller: ", SDL_JoystickName(joy), " instance ", instance, "\n");
      }
   }

   BaseEngine::~BaseEngine()
   {
      SDL_Quit();
   }

   bool BaseEngine::WindowIsOpen()
   {
      return renderer.Alive();
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

      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                            sizeof(wheel::vertex_t),
                            (void*)0
                           );
      glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_TRUE,
                            sizeof(wheel::vertex_t),
                            (void*)(2*sizeof(float))
                           );
      glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                            sizeof(wheel::vertex_t),
                            (void*)(4*sizeof(float))
                           );

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
      renderer.Swap();
   }

   inline uint32_t BaseEngine::GetEvents(wheel::EventList* events)
   {
      SDL_Event sdlevent;
      while(SDL_PollEvent(&sdlevent))
      {
//         wheel::Event* n_ptr = new wheel::Event;
//         wheel::Event& newevent = *n_ptr;
         wheel::Event newevent;

         if (sdlevent.type == SDL_WINDOWEVENT)
         {
            switch (sdlevent.window.event)
            {
               case SDL_WINDOWEVENT_CLOSE:
                  renderer.Destroy();
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

   bool BaseEngine::Run()
   {
      if (!t_active)
      {
         t_active = true;
         t_previous = std::chrono::steady_clock::now();

         t_delay = 0;
      }

      while(WindowIsOpen())
      {
         t_current = std::chrono::steady_clock::now();
         t_elapsed = std::chrono::duration<double, std::milli>(t_current - t_previous).count();
         t_previous = t_current;
         t_delay += t_elapsed;

         while (t_delay >= update_interval)
         {
            frame++;

            GetEvents(&events);
            // rollback?

            Update();
            t_delay -= update_interval;
         }

         SwapBuffers();
      }

      return true;
   }
}

int main(int argc, char* argv[])
{
   uint32_t cerr = wcl::initialise(argc, argv);

   if (cerr)
   {
      std::cout << "wheel initialisation error -- fatal, exiting...\n";
      return 255;
   }

   yam::BaseEngine* game = new yam::BaseEngine();
   yam::Console* console = new yam::Console(game);

   yam::renderer.AddShader("testi", yam::Shader("shaders/gui.vs", "shaders/gui.fs"));

   if (!game->ok())
   {
      yam::log(yam::ERROR, "error in initialisation\n");
      delete game;

      return 1;
   } else {
      yam::log(yam::SUCCESS, "Engine init successful\n");
   }

   if (console->RunScript("autoexec.scr"))
   {
      yam::log(yam::WARNING, "error in running init script\n");
   }

   yam::log.set_frameptr(game->get_frameptr());

   game->Run();

   delete console;
   delete game;

   return 0;
}
