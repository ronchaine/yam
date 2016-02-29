#ifndef YAM_FONT
#define YAM_FONT

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

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

         float  line_spacing;

         FT_Face              face;

         wheel::buffer_t      fdata;

      public:
         const wheel::string  prefix;

         const FT_Face& get_face() const { return face; }

         inline float next_line() const { return line_spacing * face->height / 64.0f; }

         Font(const wcl::string& file, uint32_t size, float line_spacing = 1.0f);
        ~Font();
   };
}

#endif
