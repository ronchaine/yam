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
   typedef uint32_t img_format_t;
   typedef std::vector<uint32_t> palette_t;

   struct point3d_t
   {
      size_t x, y, z;
      point3d_t(size_t x, size_t y, size_t z) : x(x), y(y), z(z) {}
   };

   struct point2d_t
   {
      size_t x, y;
      point2d_t(size_t x, size_t y) : x(x), y(y) {}
   };

   struct texture_t
   {
      uint32_t    id;
      uint32_t    w,h;
      uint32_t    channels;
      uint32_t    format;
   };

   struct image_t
   {
         struct pixel_access_t
         {
            private:
               image_t& img;
               size_t x, y;

            public:
               pixel_access_t(image_t& img, size_t x, size_t y) : img(img), x(x), y(y) {}

               void operator=(uint32_t col)
               {
                  assert((img.channels == 4) && "operator= only implemented for 4-channel images");
                  img.image[img.width * img.channels * y + img.channels * x + 0] = (col & (0xff000000)) >> 24;
                  img.image[img.width * img.channels * y + img.channels * x + 1] = (col & (0xff0000)) >> 16;
                  img.image[img.width * img.channels * y + img.channels * x + 2] = (col & (0xff00)) >> 8;
                  img.image[img.width * img.channels * y + img.channels * x + 3] = (col & (0xff));
               }

               uint32_t value()
               {
                  assert((img.channels == 4) && "value() only implemented for 4-channel images");
                  return (img.image[img.width * img.channels * y + img.channels * x + 0] << 24)
                       + (img.image[img.width * img.channels * y + img.channels * x + 1] << 16)
                       + (img.image[img.width * img.channels * y + img.channels * x + 2] << 8)
                       + (img.image[img.width * img.channels * y + img.channels * x + 3]);
               }
         };

         uint32_t       width;
         uint32_t       height;
         uint32_t       channels;

         wcl::buffer_t  image;
         palette_t      palette;

         uint8_t& operator[](point3d_t l)
         {
            return image[width * channels * l.y + channels * l.x + l.z];
         }

         pixel_access_t operator[](point2d_t l)
         {
            return pixel_access_t(*this, l.x, l.y);
         }
   };
}

#endif
