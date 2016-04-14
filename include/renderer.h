#ifndef YAM_RENDERER
#define YAM_RENDERER

#include "common.h"
#include "debug.hpp"
#include "shader.h"
#include "font.h"

#include <map>

namespace yam
{
   struct vertex_t
   {
      float x0, y0;
      uint16_t s0, t0;
      uint8_t r, g, b, a;

      vertex_t() {}
      vertex_t(float x, float y) : x0(x), y0(y) {}
   };

   struct rord_t
   {
      uint32_t       z_order;
      wcl::string    shader;
      GLenum         array_type;
/*
      inline size_t hash() const noexcept
      {
         return (shader.hash() + z_order) ^ array_type;
      }
*/
      bool operator<(const rord_t& value) const
      {
         return std::tie(value.z_order, shader, value.array_type)
              < std::tie(z_order, value.shader, array_type);
      }
/*
      bool operator==(const rord_t& other) const
      {
         if ((z_order == other.z_order) && (shader == other.shader)
         && (array_type == other.array_type))
            return true;

         return false;
      }
*/
      rord_t(uint32_t z, const wcl::string& mat, GLenum at = GL_TRIANGLES)
      {
         z_order = z;
         shader = mat;
         array_type = at;
      }
   };
}

namespace std
{
/*
   template<>
   struct hash<yam::rord_t>
   {
      size_t operator()(const yam::rord_t& __s) const noexcept
      {
         return __s.hash();
      }
   };
*/
}


namespace yam {

   struct rbuffer_t
   {
      GLuint            vbo;
      wheel::buffer_t   vertex_data;

      rbuffer_t()
      {
         glGenBuffers(1, &vbo);
      }

      ~rbuffer_t()
      {
         glDeleteBuffers(1, &vbo);
      }
   };

   struct atlas_t
   {
      wheel::Atlas      atlas;

      texture_t*        texture;
      uint32_t          size;

      std::unordered_map<wcl::string, wheel::rect_t> stored;

      atlas_t() : texture(nullptr) {}
   };

   struct rendertarget_t
   {
      GLuint            id;
      GLuint            depth_buf;

      uint32_t          w;
      uint32_t          h;
   };

   class Renderer; extern Renderer renderer;

   class Renderer
   {
      class TextureUnits
      {
         class tu_info_t
         {
            private:
               bool              in_use;
               wcl::string       texture;
               static uint32_t   active;

               const bool        usable;
               const uint32_t    number;
            public:

               tu_info_t(uint32_t n) : in_use(false), texture("null"), number(n), usable(true) {}
               tu_info_t(bool) : in_use(true), texture("out of bounds"), number(0), usable(false) {}

               const bool& used() { return in_use; };
               const wcl::string& current() { return texture; }

               static inline uint32_t active_unit() { return active; }

               void operator=(void* ptr)
               {
                  if (!usable)
                     return;

                  assert(ptr == nullptr);
                  glActiveTexture(GL_TEXTURE0 + number);
                  glBindTexture(GL_TEXTURE_2D, 0);

                  texture = "null";
                  in_use = false;

                  if (active != number)
                     glActiveTexture(GL_TEXTURE0 + active);
               }

               inline void operator=(const char* texture)
               {
                  operator=((wcl::string)texture);
               }
               void operator=(const wcl::string& new_texture)
               {
                  if (!usable)
                  {
                     log(ERROR, "Tried to bind texture to a texture unit that doesn't exist.\n");
                     return;
                  }

                  bool is_atlas = false;
                  if (renderer.texture.count(new_texture) == 0)
                  {
                     log(WARNING, "Tried to set texture unit ",number," to texture '",
                         new_texture,"', which doesn't exist.\n");
                         return;
                  }

                  log(FULL_DEBUG, "bound TU ", number, " to ", new_texture,"\n");

                  glActiveTexture(GL_TEXTURE0 + number);
                  glBindTexture(GL_TEXTURE_2D, renderer.texture[new_texture].id);

                  texture = new_texture;
                  active = true;
                  in_use = true;

                  if (active != number)
                     glActiveTexture(GL_TEXTURE0 + active);

                  return;
               }
         };
         static const tu_info_t null_info;

         private:
            std::vector<tu_info_t>  info;
         public:
            inline void init()
            {
               while (glGetError() != GL_NO_ERROR);

               int value;
               glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &value);

               info.reserve(value);

               for (uint32_t i = 0; i < value; ++i)
                  info.push_back(i);

               if (value < 4)
               {
                  log(FATAL, "Insufficient texture image units available (4 required, ",
                      value, "available)\n");

                  assert(0 && "device does not meet system requirements");
               }

               log(NOTE, "Texture image units available: ", info.size(), "\n");
            }

            tu_info_t& operator[](const int index)
            {
               if (index < info.size())
                  return info[index];

               return (tu_info_t&)null_info;
            }

            inline uint32_t get_active_id()
            {
               return tu_info_t::active_unit();
            }

            inline uint32_t count()
            {
               return info.size();
            }

            inline int32_t get_free()
            {
               for (int i = 0; i < info.size(); ++i)
               {
                  if (!info[i].used())
                     return i;
               }

               return -1;
            }
      };

      struct shader_proxy_t
      {
         Shader& operator[](const wcl::string& sh)
         {
            if (renderer.shaderlist.count(sh))
               return renderer.shaderlist[sh];

            assert(0 && "array index out of range");
         }
      };

      private:
         SDL_Window*    window;
         void*          context;

         bool           alive;

         wcl::string    current_shader;
         wcl::string    current_target;

         std::unordered_map<wcl::string, Shader>      shaderlist;
         std::unordered_map<wcl::string, Font>        fontlist;

         std::unordered_map<wcl::string, texture_t>   texture;
         std::unordered_map<wcl::string, atlas_t>     atlas;
         std::unordered_map<wcl::string, rendertarget_t> target;

         std::map<rord_t, rbuffer_t>                  buffers;

         inline rbuffer_t& select_buffer(uint32_t z, const wcl::string& shader,
                                         GLenum etype = GL_TRIANGLES)
         {
            return buffers[rord_t(z, shader, etype)];
         }

      public:
         TextureUnits                                 texture_unit;
         shader_proxy_t                               shader;

         uint32_t                                     scrw;
         uint32_t                                     scrh;

         uint32_t AddShader(const wcl::string& name, Shader&& shader);
         uint32_t UseShader(const wcl::string& name);

         void     SetShader(const wcl::string& name);

         inline wcl::string GetShader() { return current_shader; }

         // Render to texture, etc.
         uint32_t CreateTarget(const wcl::string& name,
                               uint32_t width, uint32_t height,
                               uint32_t channels,
                               uint32_t mrt_level = 1,
                               bool has_depth = false);

         void     SetTarget(const wcl::string& name);

         inline void SetTarget(int)
         {
            SetTarget("");
         }

         inline void RebindActiveTarget()
         {
            if (current_target == "")
               glBindFramebuffer(GL_FRAMEBUFFER, 0);
            else
               glBindFramebuffer(GL_FRAMEBUFFER, target[current_target].id);
         }

         void     AddVertex(vertex_t vert, uint32_t z_order = 0, GLenum etype = GL_TRIANGLES);

         void     AddVertices(uint32_t layer, vertex_t v)
         {
            AddVertex(v, layer);
         }
         template<typename... Rest>
         void     AddVertices(uint32_t layer, vertex_t v, Rest... rest)
         {
            AddVertex(v, layer);
            AddVertices(layer, rest...);
         }

         void     Flush();

         uint32_t CreateTexture(const wcl::string& name,
                                uint32_t w, uint32_t h,
                                uint32_t components,
                                uint32_t format = WHEEL_UNSIGNED_BYTE);

         uint32_t CreateTexture(const wcl::string& name, image_t& image);

         uint32_t UploadTextureData(const wcl::string& name,
                                    int32_t xoff, int32_t yoff,
                                    uint32_t width, uint32_t height,
                                    void* pixel_data);

         void     DeleteTexture(const wcl::string& name);

         uint32_t CreateAtlas(const wcl::string& name, uint32_t size,
                              uint32_t components, uint32_t format = WHEEL_UNSIGNED_BYTE);
         uint32_t AtlasBuffer(const wcl::string& atlas_name, const wcl::string& sprite_name,
                              uint32_t w, uint32_t h, void* data);
         uint32_t GetAtlasPos(const wcl::string& atlas_name, const wcl::string& sprite_name,
                              wheel::rect_t* result);

         int32_t  GetTU(const wcl::string& texture_or_atlas);

         inline void RebindActiveTexture()
         {
            glBindTexture(GL_TEXTURE_2D, texture[texture_unit[texture_unit[0].active_unit()].current()].id);
         }

         uint32_t Init(uint32_t w = 800, uint32_t h = 480);
         void     Destroy();

         void     Clear(float r, float g, float b)
         {
            glClearColor(r,g,b, .0);
            glClear(GL_COLOR_BUFFER_BIT);
         }

         Renderer() : window(nullptr), context(nullptr), alive(false) {}
         ~Renderer();

         inline bool Alive() { return alive; }
         inline void Swap() { SDL_GL_SwapWindow(window); }
   };
}

#endif
