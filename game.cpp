#include "include/defaultshaders.h"
#include "include/game.h"
#include "include/image.h"
#include "include/image/png.hpp"

#include "include/argparser.hpp"

#include "include/util.h"

namespace yam
{
   OutputTarget log;

   TTFFont* monospace;
   TTFFont* symbola;
   BitmapFont* fnt1;

   bool Game::ok()
   {
      return !(int_flags && YAM_ERROR);
   }

   Game::Game(uint32_t scrw, uint32_t scrh)
   {
      t_active = false;
      int_flags = YAM_CLEAR_FLAGS;

      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC))
      {
         yam::log(yam::ERROR, "Couldn't init SDL: '%s'\n", SDL_GetError());
         int_flags |= YAM_ERROR;
      }

      if (renderer.Init(scrw, scrh))
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

   Game::~Game()
   {
      SDL_Quit();
   }

   bool Game::WindowIsOpen()
   {
      return renderer.Alive();
   }

   void Game::SwapBuffers()
   {
      renderer.Swap();
   }

   inline uint32_t Game::GetEvents(wheel::EventList* events)
   {
      events->clear();

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
               case SDL_WINDOWEVENT_RESIZED:
               case SDL_WINDOWEVENT_SIZE_CHANGED:
                  yam::log(WARNING, "Window size changed\n");
                  continue;
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
            newevent.data.push_back(WHEEL_AXIS_POSITION);
            newevent.data.push_back((uint8_t)what.axis);
            newevent.data.write<uint16_t>(what.value);
         }

         if (events != nullptr)
            events->push_back(newevent);
      }

      return WHEEL_OK;
   }

   bool Game::Run()
   {
      if (!t_active)
      {
         t_active = true;
         t_previous = std::chrono::steady_clock::now();

         t_delay = 0;

         /*
         Debug stuff
         */
         state.eventmap.map_event(wheel::describe_event(WHEEL_EVENT_CONTROLLER), "ctrl",
         [&](wheel::Event& e)
            {
            }
         );
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

void yam::Game::Update()
{
   state.eventmap.process(events);
}

void yam::Game::Render()
{
   {
      yam::renderer.SetTarget("test_target");
      renderer.Clear(0x000000ff);

      renderer.SetShader("builtin_primitive");

      draw::rectangle(4, 20, 20, 40, 40, 0xff0f30ef);
      draw::rectangle(3, 40, 40, 40, 40, 0xff0f30ef);
      /*

      draw::triangle(2, 100, 20, 120, 20, 60, 50, 0xff00ffff);

      draw::line(1, 50, 50, 600, 80, 0xffffffff);

      renderer.SetShader("testi");
      draw::rectangle(3, 40, 40, 240, 240, 0xffffffff);
*/
      renderer.SetShader("testi");
      draw::rectangle(3, 40, 40, 20, 30, 0xffffffff);

      draw::set_cursor(10, 100);
      draw::text(0, *monospace, "This is text\n om nom nom", 0xffffffff);

      draw::set_cursor(10, 10);
      draw::text(0, *fnt1, "bitmap text works as well\nright?", 0xffffffff);

      yam::renderer.Flush();
      renderer.Swap();

      yam::renderer.SetTarget(0);
      renderer.Clear(0.0, 0.0, 0.0);
      yam::renderer.SetShader("final");
      draw::rectangle(0, 0, 0, renderer.scrw, renderer.scrh, 0xffffffff);

      yam::renderer.Flush();
      renderer.Swap();

   }
}

void fill_template(yam::image_t& sprite_template)
{
   //#799d3d
   for (size_t i = 0; i < sprite_template.width; ++i)
   for (size_t j = 0; j < sprite_template.height; ++j)
   {
      if (sprite_template[{i,j}].value() == 0x799d3dff)
      {
         sprite_template[{i, j}] = 0x303030ff;
      }
   }
}

int main(int argc, char* argv[])
{
   wheel::string a("builtin_primitive");
   wheel::string b("testi");

   uint32_t cerr = wcl::initialise(argc, argv);

   if (cerr)
   {
      std::cout << "wheel initialisation error -- fatal, exiting...\n";
      return 255;
   }

   yam::Game* game = new yam::Game(1920, 1080);

   yam::renderer.AddShader("builtin_primitive", yam::Shader("shaders/primitive.vs", "shaders/primitive.fs"));
   yam::renderer.AddShader("builtin_text", yam::Shader("shaders/gui.vs", "shaders/gui.fs"));
   yam::renderer.shader["builtin_text"].AddBinding(YAM_FONTBUFFER_NAME, "guiatlas");

   yam::renderer.AddShader("testi", yam::Shader("shaders/test.vs", "shaders/test.fs"));
   yam::renderer.shader["testi"].AddBinding("test texture", "texture");

   yam::symbola = new yam::TTFFont("Symbola.ttf", 35, 1.1f);
   yam::monospace = new yam::TTFFont("Cousine-Regular.ttf", 11, 0.3f);

   yam::fnt1 = new yam::BitmapFont("bitmapfont.png", "abcdefghijklmnopqrstuvwxyzäöŋɲ", 8);
//   BitmapFont::BitmapFont(const wcl::string& file, const wcl::string& glyphs, uint32_t size)

   yam::renderer.CreateTarget("test_target", 480, 270, 4);

   yam::renderer.AddShader("final", yam::Shader("shaders/test.vs", "shaders/post.fs"));
   yam::renderer.shader["final"].AddBinding("test_target_color0", "texture");

   if (!game->ok())
   {
      yam::log(yam::ERROR, "error in initialisation\n");
      delete game;

      return 1;
   } else {
      yam::log(yam::SUCCESS, "Engine init successful\n");
   }

   yam::log.set_frameptr(game->get_frameptr());
/*
   wcl::buffer_t* fnt1 = wcl::GetBuffer("bitmapfont.png");
   yam::read_png(*fnt1);
*/
   yam::image_t img;

//   yam::load_to_buffer<yam::format::PNG>(img, "content/test_paletted.png");
   yam::load_to_buffer<yam::format::PNG>(img, "template1.png");
   yam::flip_vertical(img);

   fill_template(img);

   yam::renderer.CreateTexture("test texture", img);

//   yam::load_to_texture<yam::format::PNG>("test texture", "content/test_diffuse.png");
//   yam::save_png("testout.png", img);

   game->Run();

   delete yam::fnt1;
   delete yam::symbola;
   delete yam::monospace;

   delete game;

   return 0;
}
