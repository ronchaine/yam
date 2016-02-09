#ifndef YAM_COMMON_H
#define YAM_COMMON_H

#include <wheel.h>
#include <wheel_atlas.h>
#include <wheel_extras_font.h>

#include "debug.hpp"

#include <chrono>

//#include "lua.hpp"

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

#define YAM_SHADER_COMPILE_ERROR WHEEL_MODULE_SHADER_COMPILE_ERROR

#define WHEEL_STRFMT_BUFFER_SIZE 256
namespace wheel
{
   inline const wheel::string strfmt(const char* format, ...)
   {
      char buffer[WHEEL_STRFMT_BUFFER_SIZE];
      va_list args;
      va_start (args, format);
      vsnprintf(buffer, WHEEL_STRFMT_BUFFER_SIZE, format, args);
      va_end(args);

      return wheel::string(buffer);
   }
}

namespace yam
{
   typedef std::chrono::steady_clock::time_point timepoint_t;

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

   struct renderbuffer_t
   {
      wheel::buffer_t   current;
      GLuint            cvbo;

      renderbuffer_t() : cvbo(0) {}
   };

}

#endif