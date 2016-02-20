#ifndef YAM_FONT
#define YAM_FONT

#include <ft2build.h>
#include FT_FREETYPE_H

#include <wheel.h>

#define YAM_FONTBUFFER_NAME "__yam_font_buffer"
#define YAM_FONTBUFFER_SIZE 1024

namespace yam
{
   struct tx_t
   {
      uint16_t s, t;
   };

   class Font
   {
      private:
         static FT_Library    library;
         static uint32_t      l_count;
         static uint32_t      colour;

         FT_Face              face;

         wheel::buffer_t      fdata;

      public:
         const FT_Face& get_face() const { return face; }

         Font(const wcl::string& file, uint32_t size);
        ~Font();
   };
}

#endif
