#include "include/font.h"
#include "include/debug.hpp"

#include "include/renderer.h"
#include "include/image/png.hpp"

namespace yam
{
   FT_Library  TTFFont::library  = nullptr;
   uint32_t    TTFFont::l_count  = 0;
   uint32_t    TTFFont::colour   = 0xffffffff;

   bool        Font::font_init   = false;

   Font::Font(const wcl::string& prefix) : prefix(prefix)
   {
      if (font_init == false)
      {
         // Create alpha-only atlas
         renderer.CreateAtlas(YAM_FONTBUFFER_NAME, YAM_FONTBUFFER_SIZE, 1);
      }
   }

   std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, bool, uint32_t>
   BitmapFont::get_next_glyph(image_t& img, uint32_t start, uint32_t row, uint32_t size)
   {
      // We shouldn't be here if this wasn't true.
      assert(img.channels == 1);

      uint32_t left = ~0, top = ~0, w = 0, h = 0;
      bool result = false;
      bool row_empty = true;

      while (row * size < img.height)
      {
         for (size_t i = start; i < img.width; ++i)
         {
            row_empty = true;
            for (size_t j = row * size; j < row * size + size; ++j)
            {
               if (img.image[img.width * j + i] != 0)
               {
                  row_empty = false;

                  if (left == ~0)
                     left = i;

                  if (top > j)
                     top = j;

                  if (h < j)
                     h = j;
               }
            }

            // Check if we're through
            if ((row_empty == true) && (left != ~0))
            {
               w = i - left;
               h = h - top + 1;

               result = true;

               return std::tie(left, top, w, h, result, row);
            }
         }
         row++;
         start = 0;
      }

      return std::tie(left, top, w, h, result, row);
   }

   BitmapFont::BitmapFont(const wcl::string& file, const wcl::string& glyphs, uint32_t size, uint32_t spacelen)
   : Font(file + "/" + size + "/")
   {
      type = YAM_FONT_BITMAP;

      line_spacing = size;

      font_metrics sb;
      sb.advance = spacelen;

      metrics[' '] = sb;

      yam::image_t font;
      yam::load_to_buffer<yam::format::PNG>(font, file);

      if (font.channels != 1)
      {
         log(ERROR, "BitmapFont image channels != 1\n");
         return;
      }

      yam::log(FULL_DEBUG, "Creating bitmap font from '", file, "'\n");
//      yam::flip_vertical(font);

      uint32_t l0, r0 = 0, row = 0;

      uint32_t glyphs_read = 0;

      uint32_t left, top, w, h;
      bool success = false;

      // rip the requested glyphs from the image
      for (char32_t c : glyphs)
      {
         wcl::string char_str(c);
         std::tie(left, top, w, h, success, row) = get_next_glyph(font, r0, row, size);
         if (!success)
         {
            log(ERROR, "Can't read requested glyphs from '",file,"'\n");
            return;
         }

         // Set next glyph start
         r0 = left + w;

         // Add c-glyph, to atlas
         uint8_t remap[h][w];

         for (int i = 0; i < w; ++i) for (int j = 0; j < h; ++j)
            remap[h - j - 1][i] = font.image[(top + j) * font.width + left + i];

         renderer.AtlasBuffer(YAM_FONTBUFFER_NAME, prefix + c, w, h, remap);

         font_metrics m;
         m.w = w;
         m.h = h;
         m.advance = w + 1;
         m.top = 0;
         m.left = 0;

         metrics[c] = m;
      }
   }

   BitmapFont::~BitmapFont()
   {
   }













   TTFFont::TTFFont(const wcl::string& file, uint32_t size, float ls)
   : Font(file + "/" + size + "/"), line_spacing(ls)
   {
      type = YAM_FONT_TRUETYPE;

      if (l_count == 0)
      {
         if (FT_Init_FreeType(&library))
         {
            yam::log(yam::ERROR, "FreeType initialisation failed\n");
            return;
         }
      }
      l_count++;

      const wheel::buffer_t* buf = wheel::GetBuffer(file);
      if (buf == nullptr)
      {
         log(ERROR, "failed to load font '",file,"'\n");
         return;
      }

      fdata = *buf;
      wheel::DeleteBuffer(file);

      if (FT_New_Memory_Face(library, &fdata[0], fdata.size(), 0, &face))
      {
         log(ERROR, "Freetype error: couldn't create new face from memory buffer\n");
         return;
      }

      FT_Select_Charmap(face, ft_encoding_unicode);
      FT_Set_Pixel_Sizes(face, 0, size);

      const wcl::string precache = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz";

      uint32_t ft_w, ft_h;
      for (char32_t c : precache)
      {
         if (FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT))
            continue;

         ft_w = face->glyph->bitmap.width;
         ft_h = face->glyph->bitmap.rows;

         uint8_t remap[ft_h][ft_w];

         for (int i = 0; i < ft_w; ++i) for (int j = 0; j < ft_h; ++j)
            remap[ft_h - j - 1][i] = *(face->glyph->bitmap.buffer + j * ft_w + i);

         renderer.AtlasBuffer(YAM_FONTBUFFER_NAME, prefix + c, ft_w, ft_h, remap);
      }
   }

   TTFFont::~TTFFont()
   {
      FT_Done_Face(face);

      l_count--;
      if (l_count == 0)
         FT_Done_FreeType(library);
   }

   bool TTFFont::atlas_glyph(char32_t c) const
   {
      uint32_t ft_w = face->glyph->bitmap.width;
      uint32_t ft_h = face->glyph->bitmap.rows;

      uint8_t remap[ft_h][ft_w];

      for (int i = 0; i < ft_w; ++i) for (int j = 0; j < ft_h; ++j)
         remap[ft_h - j - 1][i] = *(face->glyph->bitmap.buffer + j * ft_w + i);

      if (renderer.AtlasBuffer(YAM_FONTBUFFER_NAME, prefix + c, ft_w, ft_h, remap) != WHEEL_OK)
         return false;

      return true;
   }

}
