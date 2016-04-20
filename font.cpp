#include "include/font.h"
#include "include/debug.hpp"

#include "include/renderer.h"
#include "include/image/png.hpp"

namespace yam
{
   FT_Library  TTFFont::library  = nullptr;
   uint32_t    TTFFont::l_count  = 0;
   uint32_t    TTFFont::colour   = 0xffffffff;

   BitmapFont::BitmapFont(const wcl::string& file, uint32_t size) : Font(file + "/" + size + "/")
   {
      type = YAM_FONT_BITMAP;

/*
      yam::image_t bmfont;
      yam::load_to_buffer<yam::format::PNG>(bmfont, "bitmapfont.png");
      yam::flip_vertical(bmfont);
*/
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

         // Create alpha-only atlas
         renderer.CreateAtlas(YAM_FONTBUFFER_NAME, YAM_FONTBUFFER_SIZE, 1);
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
}
