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
}
