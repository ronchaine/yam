#ifndef YAM_FONT
#define YAM_FONT

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

#include <wheel.h>
#include "image.h"

#define YAM_FONTBUFFER_NAME "__yam_font_buffer"
#define YAM_FONTBUFFER_SIZE 256

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
      uint32_t advance;
      uint32_t top;
      uint32_t left;
   };

   class Font
   {
      protected:
         static bool          font_init;
         font_type_t          type;

         Font(const wcl::string& prefix);

      public:
         const wheel::string  prefix;

         virtual int32_t get_advance(char32_t glyph) { return 0; }
         virtual int32_t get_advance_vertical(char32_t glyph) { return 0; }

         virtual bool prepare_glyph(char32_t glyph) const { return true; }

         virtual float next_line() const { return 0; }
         virtual ~Font() {}

         virtual bool atlas_glyph(char32_t glyph) const { return false; }

         virtual int32_t glyph_left(char32_t glyph) { return 0; }
         virtual int32_t glyph_width(char32_t glyph) { return 0; }
         virtual int32_t glyph_top(char32_t glyph) { return 0; }
         virtual int32_t glyph_height(char32_t glyph) { return 0; }
   };

   class BitmapFont : public Font
   {
      private:
         std::unordered_map<char32_t, font_metrics> glyph;
         uint32_t line_spacing;

         std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, bool, uint32_t>
         get_next_glyph(image_t& font, uint32_t start, uint32_t row, uint32_t size);

         std::unordered_map<char32_t, font_metrics> metrics;

      public:
         inline float next_line() const { return line_spacing; }

         inline int32_t get_advance(char32_t c) { return metrics[c].advance; }

         inline int32_t glyph_left(char32_t c) { return metrics[c].left; }
         inline int32_t glyph_width(char32_t c) { return metrics[c].w; }
         inline int32_t glyph_top(char32_t c) { return metrics[c].top; }
         inline int32_t glyph_height(char32_t c) { return metrics[c].h; }

         BitmapFont(const wcl::string& file, const wcl::string& glyphs, uint32_t size, uint32_t hbreak = 3);
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

         inline bool prepare_glyph(char32_t c) const
         {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT))
               return false;

            return true;
         }

         inline int32_t get_advance(char32_t c) { return (face->glyph->advance.x >> 6); }

         inline int32_t glyph_left(char32_t glyph) { return face->glyph->bitmap_left; }
         inline int32_t glyph_width(char32_t glyph) { return face->glyph->bitmap.width; }
         inline int32_t glyph_top(char32_t glyph) { return -face->glyph->bitmap.rows + face->glyph->bitmap_top; }
         inline int32_t glyph_height(char32_t glyph) { return face->glyph->bitmap.rows; }

         bool atlas_glyph(char32_t c) const;

         TTFFont(const wcl::string& file, uint32_t size, float line_spacing = 1.0f);
        ~TTFFont();
   };
}

#endif
