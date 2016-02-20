#include "include/defaultshaders.h"
#include "include/baseengine.h"
#include "include/console.h"

#include "include/argparser.hpp"

#include "include/util.h"

namespace yam
{
   OutputTarget log;

   Font* monospace;

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

         Render();
      }

      return true;
   }
}

void yam::BaseEngine::Render()
{
   renderer.Clear(0.0, 0.0, 0.0);

   renderer.SetShader("testi");

   draw::rectangle(0, 20, 20, 40, 40, 0xff0f30ef);
   draw::rectangle(0, 40, 40, 40, 40, 0xff0f30ef);

   draw::triangle(0, 100, 20, 120, 20, 60, 50, 0xffffffff);

   draw::line(0, 50, 50, 600, 80, 0xffffffff);

   draw::set_cursor(10, 200);
   draw::text(0, *monospace, "This is text", 0xffffffff);

   yam::renderer.Flush();
   renderer.Swap();
}

void yam::BaseEngine::Update()
{
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

   yam::renderer.AddShader("testi", yam::Shader("shaders/test.vs", "shaders/test.fs"));
   yam::renderer.AddShader("builtin_text", yam::Shader("shaders/gui.vs", "shaders/gui.fs"));
   yam::renderer.shader["builtin_text"].AddBinding(YAM_FONTBUFFER_NAME, "guiatlas");

   yam::monospace = new yam::Font("Cousine-Regular.ttf", 15);

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

   yam::renderer.CreateTexture("test texture", 32, 32, 4);
   yam::renderer.texture_unit[0] = "test texture";

   std::cout << "texture unit 0 bound to " << yam::renderer.texture_unit[0].current() << "\n";

   game->Run();

   delete yam::monospace;

   delete console;
   delete game;

   return 0;
}
