#ifndef YAM_UTIL
#define YAM_UTIL

#include "renderer.h"
#include "font.h"

namespace yam
{
   namespace draw
   {
      void rectangle(uint32_t layer, uint32_t x, uint32_t y,
                     uint32_t w, uint32_t h,
                     uint32_t c = 0xffffffff,
                     const wcl::string& sprite = "",
                     float scale = 1.0
                    );

      void triangle(uint32_t layer,
                    uint32_t x0, uint32_t y0,
                    uint32_t x1, uint32_t y1,
                    uint32_t x2, uint32_t y2,
                    uint32_t c = 0xffffffff
                   );

      void line(uint32_t layer,
                uint32_t x0, uint32_t y0,
                uint32_t x1, uint32_t y1,
                uint32_t c = 0xffffffff
               );

      void text(uint32_t layer, uint32_t x, uint32_t y,
                const wcl::string& text, const Font& font,
                uint32_t c = 0xffffffff);
   }
}

#endif
