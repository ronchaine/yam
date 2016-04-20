#ifndef YAM_FONT
#define YAM_FONT

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include <wheel.h>
#include "image.h"

#define YAM_FONTBUFFER_NAME "__yam_font_buffer"
#define YAM_FONTBUFFER_SIZE 1024

namespace yam
{
   struct tx_t
   {
      uint16_t s, t;
   };

   enum font_type_t {
      YAM_FONT_TRUETYPE,
      YAM_FONT_BITMAP
   };

   struct font_metrics {
      uint32_t w;
      uint32_t h;
   };

   class Font
   {
      protected:
         font_type_t          type;

      public:
         const wheel::string  prefix;

         virtual float next_line() const { return 0; }

         Font(const wcl::string& prefix) : prefix(prefix) {}
         virtual ~Font() {}
   };

   class BitmapFont : public Font
   {
      private:
         std::unordered_map<char32_t, font_metrics> glyph;
         uint32_t line_spacing;

      public:
         inline float next_line() const { return line_spacing; }

         BitmapFont(const wcl::string& file, uint32_t size);
        ~BitmapFont();
   };

   class TTFFont : public Font
   {
      private:
         static FT_Library    library;
         static uint32_t      l_count;
         static uint32_t      colour;

         float  line_spacing;

         FT_Face              face;

         wheel::buffer_t      fdata;

      public:
         const FT_Face& get_face() const { return face; }

         inline float next_line() const { return line_spacing * face->height / 64.0f; }

         TTFFont(const wcl::string& file, uint32_t size, float line_spacing = 1.0f);
        ~TTFFont();
   };
}

#endif
