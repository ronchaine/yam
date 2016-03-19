#ifndef YAM_IMAGE_H
#define YAM_IMAGE_H

#include "common.h"
#include "debug.hpp"

namespace yam
{
   template<img_format_t T>
   uint32_t load_to_buffer(image_t& target, const wcl::string& file);

   template<img_format_t T>
   uint32_t load_to_texture(const wcl::string& texture, const wcl::string& file);

   void flip_vertical(image_t& img);
   void framebuffer_to_image(image_t& img);
/*
   class Image
   {
      private:
         uint32_t    width;
         uint32_t    height;
         uint32_t    channels;

         wcl::buffer_t image;
      public:
         // read data
         uint32_t CaptureScreen();
         uint32_t LoadPNG(const wcl::buffer_t& data);
         uint32_t LoadPNG(const wcl::string& file);

         // write data
         uint32_t WritePNG(const wcl::string& file);
   };
*/
}

#endif
