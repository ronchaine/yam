#include "include/util.h"

#define ATLAS_SIZE 1024

namespace yam
{
   namespace draw
   {
      static double cursor_pos = 0;
      static double cursor_row = 0;

      void set_cursor(uint32_t x, uint32_t y)
      {
         cursor_pos = x;
         cursor_row = y;
      }

      std::tuple<double, double> get_cursor()
      {
         return std::tie(cursor_pos, cursor_row);
      }

      void rectangle(uint32_t layer, uint32_t x, uint32_t y,
                     uint32_t w, uint32_t h, uint32_t c,
                     const wcl::string& sprite, float scale)
      {
         const float x_unit = 2.0f / (float)renderer.GetTargetWidth() * scale;
         const float y_unit = 2.0f / (float)renderer.GetTargetHeight() * scale;

         vertex_t v0, v1, v2, v3;

         float tx_left = .0f;
         float tx_right = 1.0f;
         float tx_top = 1.0f;
         float tx_bottom = 0.0f;

         v0.s0 = 0xffff * tx_left;
         v0.t0 = 0xffff * tx_bottom;

         v1.s0 = 0xffff * tx_right;
         v1.t0 = 0xffff * tx_bottom;

         v2.s0 = 0xffff * tx_right;
         v2.t0 = 0xffff * tx_top;

         v3.s0 = 0xffff * tx_left;
         v3.t0 = 0xffff * tx_top;

         v0.x0 = -1.0f + (float)x * x_unit;
         v0.y0 = -1.0f + (float)y * y_unit;

         v1.x0 = -1.0f + (float)(x+w) * x_unit;
         v1.y0 = v0.y0;

         v2.x0 = v1.x0;
         v2.y0 = -1.0f + (float)(y+h) * y_unit;

         v3.x0 = v0.x0;
         v3.y0 = v2.y0;

         v1.r = v2.r = v3.r = v0.r = (c & 0xff000000) >> 24;
         v1.g = v2.g = v3.g = v0.g = (c & 0xff0000) >> 16;
         v1.b = v2.b = v3.b = v0.b = (c & 0xff00) >> 8;
         v1.a = v2.a = v3.a = v0.a = c & 0xff;

         renderer.AddVertices(layer, v0, v1, v3, v1, v2, v3);
      }

      void triangle(uint32_t layer,
                    uint32_t x0, uint32_t y0,
                    uint32_t x1, uint32_t y1,
                    uint32_t x2, uint32_t y2,
                    uint32_t c
                   )
      {
         const float x_unit = 2.0f / (float)renderer.GetTargetWidth();
         const float y_unit = 2.0f / (float)renderer.GetTargetHeight();

         vertex_t v0, v1, v2;

         v0.x0 = -1.0f + (float)x0 * x_unit;
         v0.y0 = -1.0f + (float)y0 * y_unit;

         v1.x0 = -1.0f + (float)x1 * x_unit;
         v1.y0 = -1.0f + (float)y1 * y_unit;

         v2.x0 = -1.0f + (float)x2 * x_unit;
         v2.y0 = -1.0f + (float)y2 * y_unit;

         v1.r = v2.r = v0.r = (c & 0xff000000) >> 24;
         v1.g = v2.g = v0.g = (c & 0xff0000) >> 16;
         v1.b = v2.b = v0.b = (c & 0xff00) >> 8;
         v1.a = v2.a = v0.a = c & 0xff;

         renderer.AddVertices(layer, v0, v1, v2);
      }

      void line(uint32_t layer,
                uint32_t x0, uint32_t y0,
                uint32_t x1, uint32_t y1,
                uint32_t c
               )
      {
         const float x_unit = 2.0f / (float)renderer.GetTargetWidth();
         const float y_unit = 2.0f / (float)renderer.GetTargetHeight();

         vertex_t v0, v1;

         v0.x0 = -1.0f + (float)x0 * x_unit;
         v0.y0 = -1.0f + (float)y0 * y_unit;

         v1.x0 = -1.0f + (float)x1 * x_unit;
         v1.y0 = -1.0f + (float)y1 * y_unit;

         v1.r = v0.r = (c & 0xff000000) >> 24;
         v1.g = v0.g = (c & 0xff0000) >> 16;
         v1.b = v0.b = (c & 0xff00) >> 8;
         v1.a = v0.a = c & 0xff;

         renderer.AddVertex(v0, layer, GL_LINES);
         renderer.AddVertex(v1, layer, GL_LINES);
      }

      void text(uint32_t layer,
                const Font& font,
                const wcl::string& text,
                uint32_t colour)
      {
         wcl::string old_shader = renderer.GetShader();

         renderer.SetShader("builtin_text");

         wheel::rect_t r;

         double cursor_newline_pos = cursor_pos;

         const float x_unit = 2.0f / (float)renderer.GetTargetWidth();
         const float y_unit = 2.0f / (float)renderer.GetTargetHeight();

         uint16_t left, right, top, bottom;
         float leftv, rightv, bottomv, topv;

         char32_t c;

         FT_Face& face = (FT_Face&)font.get_face();

         vertex_t v0, v1, v2, v3;

         for (size_t i = 0; i < text.length(); ++i)
         {
            c = *(text.getptr() + i);

            if (FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT))
               continue;

            if (c == '\n')
            {
               cursor_pos = cursor_newline_pos;
               cursor_row -= font.next_line();
               continue;
            }

            if (c == ' ')
            {
               cursor_pos += (face->glyph->advance.x >> 6);
               cursor_row += (face->glyph->advance.y >> 6);
               continue;
            }

            // If character is not in atlas, put it there.
            if (renderer.GetAtlasPos(YAM_FONTBUFFER_NAME, font.prefix + c, &r) != WHEEL_OK)
            {
               uint32_t ft_w = face->glyph->bitmap.width;
               uint32_t ft_h = face->glyph->bitmap.rows;

               uint8_t remap[ft_h][ft_w];

               for (int i = 0; i < ft_w; ++i) for (int j = 0; j < ft_h; ++j)
                  remap[ft_h - j - 1][i] = *(face->glyph->bitmap.buffer + j * ft_w + i);

               if (renderer.AtlasBuffer(YAM_FONTBUFFER_NAME, font.prefix + c, ft_w, ft_h, remap) != WHEEL_OK)
                  continue;
            }

            left     = ((float)(r.x / (float)YAM_FONTBUFFER_SIZE)) * 0xffff;
            right    = ((float)((r.x + r.w) / (float)YAM_FONTBUFFER_SIZE)) * 0xffff;
            bottom   = ((float)(r.y / (float)YAM_FONTBUFFER_SIZE)) * 0xffff;
            top      = ((float)((r.y + r.h) / (float)YAM_FONTBUFFER_SIZE)) * 0xffff;

            int ymod = -face->glyph->bitmap.rows + face->glyph->bitmap_top;

            leftv    = -1.0f + x_unit * (cursor_pos + face->glyph->bitmap_left);
            rightv   = -1.0f + x_unit * (cursor_pos + face->glyph->bitmap_left + face->glyph->bitmap.width);
            bottomv  = -1.0f + y_unit * (cursor_row + ymod);
            topv     = -1.0f + y_unit * (cursor_row + ymod + face->glyph->bitmap.rows);

            v0.r = (colour & 0xff000000) >> 24;
            v0.g = (colour & 0xff0000) >> 16;
            v0.b = (colour & 0xff00) >> 8;
            v0.a = (colour & 0xff);

            v0.s0 = left;
            v0.t0 = bottom;
            v0.x0 = leftv;
            v0.y0 = bottomv;

            v1.s0 = right;
            v1.t0 = bottom;
            v1.x0 = rightv;
            v1.y0 = v0.y0;

            v2.s0 = right;
            v2.t0 = top;
            v2.x0 = v1.x0;
            v2.y0 = topv;

            v3.s0 = left;
            v3.t0 = top;
            v3.x0 = v0.x0;
            v3.y0 = v2.y0;

            v3.r = v2.r = v1.r = v0.r;
            v3.g = v2.g = v1.g = v0.g;
            v3.b = v2.b = v1.b = v0.b;
            v3.a = v2.a = v1.a = v0.a;

            renderer.AddVertices(layer, v0, v1, v3, v1, v2, v3);

            cursor_pos += (face->glyph->advance.x >> 6);
            cursor_row += (face->glyph->advance.y >> 6);
         }

         if (old_shader != renderer.GetShader())
            renderer.SetShader(old_shader);
      }
   }
}
