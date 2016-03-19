#include "include/image.h"

namespace yam
{
   void flip_vertical(image_t& img)
   {
      wcl::buffer_t mod;
      mod.resize(img.image.size());

      size_t it = 0;

      for (int j = 0; j < img.height; ++j)
      for (int i = 0; i < img.width; ++i)
      for (int c = 0; c < img.channels; ++c)
      {
         mod[img.width * img.channels * (img.height - j - 1) + img.channels * i + c]
         = img.image[img.width * img.channels * j + img.channels * i + c];
         it++;
      }

      std::swap(img.image, mod);
   }

   void framebuffer_to_image(size_t scrw, size_t scry, image_t& img)
   {
      img.width = scrw;
      img.height = scry;
      img.channels = 4;
      img.image.resize(scrw * scry * 4);

      glReadPixels(0, 0, scrw, scry, GL_RGBA, GL_UNSIGNED_BYTE, &img.image[0]);
   }

}
