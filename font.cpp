#include "include/font.h"
#include "include/debug.hpp"

namespace yam
{
   FT_Library  Font::library  = nullptr;
   uint32_t    Font::l_count  = 0;
   uint32_t    Font::colour   = 0xffffffff;

   Font::Font(const wcl::string& file, uint32_t size)
   {
      if (l_count == 0)
      {
         if (FT_Init_FreeType(&library))
         {
            yam::log(yam::ERROR, "FreeType initialisation failed\n");
            return;
         }

         //CreateTexture();
         //CreateAtlas();
      }
   }
}
