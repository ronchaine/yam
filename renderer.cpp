#include "include/renderer.h"

namespace yam
{
   Renderer renderer;
   const Renderer::TextureUnits::tu_info_t Renderer::TextureUnits::null_info(true);
   uint32_t Renderer::TextureUnits::tu_info_t::active = 0;

   uint32_t Renderer::Init(uint32_t w, uint32_t h)
   {
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

      scrw = w; scrh = h;

      window = SDL_CreateWindow("Test",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                w, h,
                                SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);

      if (window == nullptr)
      {
         log(FATAL, "Could not open window\n");

         return 1;
      }

      context = SDL_GL_CreateContext(window);

      if (context == nullptr)
      {
         log(FATAL, "Could not create OpenGL context\n");

         return 1;
      }

      glewExperimental = GL_TRUE;
      GLenum err = glewInit();

      if (GLEW_OK != err)
      {
         log(FATAL, "GLEW error: ", glewGetErrorString(err), "\n");

         return 1;
      }

      log(NOTE, "Using GLEW " , glewGetString(GLEW_VERSION), "\n");

      glViewport(0.0, 0.0, w, h);

      GLuint VertexArrayID;
      glGenVertexArrays(1, &VertexArrayID);
      glBindVertexArray(VertexArrayID);

      //glGenBuffers(1, &rbuffer_vbo);

      glDisable(GL_MULTISAMPLE);
      glDisable(GL_DEPTH_TEST);

//      glPolygonMode(GL_BACK, GL_LINE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      alive = true;

      int32_t GLMaj, GLMin;
      glGetIntegerv(GL_MAJOR_VERSION, &GLMaj);
      glGetIntegerv(GL_MINOR_VERSION, &GLMin);

      int32_t prof;
      SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &prof);

      log(NOTE, "OpenGL version: ",GLMaj,".",GLMin," ");
      if (prof == 1)
         std::cout << "core\n";
      else if (prof == 2)
         std::cout << "compatibility\n";
      else if (prof == 3)
         std::cout << "ES\n";
      else
         std::cout << "\n";

      texture_unit.init();

      return 0;
   }

   void Renderer::Destroy()
   {
      alive = false;

      if (context != nullptr)
         SDL_GL_DeleteContext(context);
   }

   Renderer::~Renderer()
   {
      if (alive) Destroy();
/*
      for (auto t : texture)
         glDeleteTextures(1, &t.second->id);

      for (auto s : surface)
         glDeleteTextures(1, &s.second->id);
*/
      if (window != nullptr)
      {
         //glDeleteBuffers(1, &rbuffer_vbo);
         //glDeleteFramebuffers(1, &fbo_main);
      }

   }

   uint32_t Renderer::AddShader(const wcl::string& name, Shader&& shader)
   {
      if (!(shader.Status() & Shader::COMPILED))
      {
         yam::log(yam::NOTE, "Shader '",name,"' not precompiled, compiling now...\n");

         if (shader.Compile() != WHEEL_OK)
         {
            std::cout << " - Could not compile shader, not added.\n";
            return YAM_SHADER_COMPILE_ERROR;
         }
      }
      shaderlist.insert(std::make_pair(name, std::move(shader)));

      yam::log(yam::SUCCESS, "Added new shader: '", name, "'\n");

      return WHEEL_OK;
   }

   uint32_t Renderer::UseShader(const wcl::string& name)
   {
      if (shaderlist.count(name) != 1)
      {
         yam::log(yam::ERROR, "Cannot use shader '", name, "', it doesn't exist.\n");
         return WHEEL_RESOURCE_UNAVAILABLE;
      }

      shaderlist[name].Use();

//      yam::log(yam::NOTE, "Switching to shader ", name, "\n");

      return WHEEL_OK;
   }

   void Renderer::SetShader(const wcl::string& name)
   {
      if (shaderlist.count(name) != 1)
         yam::log(yam::WARNING, "Shader '", name, "' requested before it is loaded.\n");

      current_shader = name;
   }


   void Renderer::AddVertex(vertex_t vertex, uint32_t z_order, GLenum etype)
   {
      wheel::buffer_t& cbuf = select_buffer(z_order, current_shader, etype).vertex_data;

      cbuf.write<float>(vertex.x0);
      cbuf.write<float>(vertex.y0);

      cbuf.write<uint16_t>(vertex.s0);
      cbuf.write<uint16_t>(vertex.t0);

      cbuf.write<uint8_t>(vertex.r);
      cbuf.write<uint8_t>(vertex.g);
      cbuf.write<uint8_t>(vertex.b);
      cbuf.write<uint8_t>(vertex.a);
   }

   void Renderer::Flush()
   {
      static wcl::string cs = "______do___not____use____";
      for (auto& buf : buffers)
      {
         if (cs != buf.first.shader)
         {
            if (UseShader(buf.first.shader))
               continue;

            cs = buf.first.shader;
         }

         size_t rsize = buf.second.vertex_data.size();

         glBindBuffer(GL_ARRAY_BUFFER, buf.second.vbo);
         glBufferData(GL_ARRAY_BUFFER, rsize, &buf.second.vertex_data[0], GL_DYNAMIC_DRAW);

         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glEnableVertexAttribArray(2);

         glBindBuffer(GL_ARRAY_BUFFER, buf.second.vbo);

         glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                               sizeof(vertex_t),
                               (void*)0
                              );
         glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_TRUE,
                               sizeof(vertex_t),
                               (void*)(2*sizeof(float))
                              );
         glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                               sizeof(vertex_t),
                               (void*)(3*sizeof(float))
                              );

         glDrawArrays(buf.first.array_type, 0, rsize / sizeof(vertex_t));

         glDisableVertexAttribArray(2);
         glDisableVertexAttribArray(1);
         glDisableVertexAttribArray(0);

         buf.second.vertex_data.clear();
         buf.second.vertex_data.seek(0);
      }
   }

   uint32_t Renderer::CreateTarget(const wcl::string& name,
                                   uint32_t width, uint32_t height,
                                   uint32_t channels,
                                   uint32_t mrt_level,
                                   bool has_depth)
   {
      rendertarget_t rtarget;

      glGenFramebuffers(1, &rtarget.id);
      glBindFramebuffer(GL_FRAMEBUFFER, rtarget.id);

      for (int i = 0; i < mrt_level; ++i)
      {
         CreateTexture(name + "_color" + i, width, height, channels, WHEEL_UNSIGNED_BYTE);
      }

      if (has_depth)
      {
         glGenRenderbuffers(1, &rtarget.depth_buf);
         glBindRenderbuffer(GL_RENDERBUFFER, rtarget.depth_buf);
         glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
         glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rtarget.depth_buf);
      } else {
         rtarget.depth_buf = 0;
      }

      GLenum drawbuffers[mrt_level];

      for (int i = 0; i < mrt_level; ++i)
      {
         glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, texture[name + "_color" + i].id, 0);
         drawbuffers[i] = GL_COLOR_ATTACHMENT0 + i;
      }

      glDrawBuffers(1, drawbuffers);

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      {
         log(ERROR, "Could not create render target '", name, "'\n");
         return WHEEL_ERROR;
      }

      rtarget.w = width;
      rtarget.h = height;

      log(SUCCESS, "Created render target '", name, "'\n");

      target[name] = rtarget;
      RebindActiveTarget();

      return WHEEL_OK;
   }

   void Renderer::SetTarget(const wcl::string& name)
   {
      if ((target.count(name) == 0) && (name != ""))
      {
         log(ERROR, "Asking for non-existant render target '",name,"'");
         return;
      }

      Flush();

      current_target = name;
      if (name != "")
      {
         glViewport(0.0, 0.0, target[current_target].w, target[current_target].h);
         glBindFramebuffer(GL_FRAMEBUFFER, target[current_target].id);
      }
      else
      {
         glViewport(0.0, 0.0, scrw, scrh);
         glBindFramebuffer(GL_FRAMEBUFFER, 0);
      }
   }

   uint32_t Renderer::CreateTexture(const wcl::string& name,
                                    uint32_t w, uint32_t h,
                                    uint32_t channels,
                                    uint32_t format)
   {
      texture_t ntex;

      glGenTextures(1, &ntex.id);
      ntex.w = w; ntex.h = h;
      ntex.channels = channels;
      ntex.format = format;

      glBindTexture(GL_TEXTURE_2D, ntex.id);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      if (channels == 1)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, format, (void*)0);
      else if (channels == 2)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, w, h, 0, GL_RG, format, (void*)0);
      else if (channels == 3)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, format, (void*)0);
      else if (channels == 4)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, format, (void*)0);

      texture[name] = ntex;
      RebindActiveTexture();

      return WHEEL_OK;
   }

   uint32_t Renderer::CreateTexture(const wcl::string& name, image_t& image)
   {
      texture_t ntex;

      glGenTextures(1, &ntex.id);
      ntex.w = image.width;
      ntex.h = image.height;
      ntex.channels = image.channels;
      ntex.format = WHEEL_UNSIGNED_BYTE;

      glBindTexture(GL_TEXTURE_2D, ntex.id);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      if (image.channels == 1)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ntex.w, ntex.h, 0, GL_RED, ntex.format, (void*)(&image.image[0]));
      else if (image.channels == 2)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, ntex.w, ntex.h, 0, GL_RG, ntex.format, (void*)(&image.image[0]));
      else if (image.channels == 3)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ntex.w, ntex.h, 0, GL_RGB, ntex.format, (void*)(&image.image[0]));
      else if (image.channels == 4)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ntex.w, ntex.h, 0, GL_RGBA, ntex.format, (void*)(&image.image[0]));

      texture[name] = ntex;

      RebindActiveTexture();

      return WHEEL_OK;
   }


   uint32_t Renderer::UploadTextureData(const wcl::string& name,
                                        int32_t xoff, int32_t yoff,
                                        uint32_t w, uint32_t h,
                                        void* pixel_data)
   {
      if (!texture.count(name))
      {
         log(ERROR, "Can't upload texture data to texture ", name, ", it doesn't exist.\n");
         return WHEEL_RESOURCE_UNAVAILABLE;
      }

      glBindTexture(GL_TEXTURE_2D, texture[name].id);

      if (texture[name].channels == 1)
         glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, w, h, GL_RED, texture[name].format, pixel_data);
      else if (texture[name].channels == 2)
         glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, w, h, GL_RG, texture[name].format, pixel_data);
      else if (texture[name].channels == 3)
         glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, w, h, GL_RGB, texture[name].format, pixel_data);
      else if (texture[name].channels == 4)
         glTexSubImage2D(GL_TEXTURE_2D, 0, xoff, yoff, w, h, GL_RGBA, texture[name].format, pixel_data);
      else
      {
         if (name != texture_unit[texture_unit.get_active_id()].current())
            RebindActiveTexture();
         return 666;
      }

      if (name != texture_unit[texture_unit.get_active_id()].current())
         RebindActiveTexture();

      return WHEEL_OK;
   }

   void Renderer::DeleteTexture(const wcl::string& name)
   {
      if (!texture.count(name))
         return;

      glDeleteTextures(1, &texture[name].id);

      texture.erase(name);
   }

   uint32_t Renderer::CreateAtlas(const wcl::string& name, uint32_t size,
                                  uint32_t channels, uint32_t format)
   {
      atlas_t atlas_new;

      CreateTexture(name, size, size, channels, format);

      atlas_new.texture = &texture[name];

      atlas_new.size = size;
      atlas_new.atlas.width = size;
      atlas_new.atlas.height = size;

      atlas_new.atlas.Reset();

      atlas[name] = atlas_new;

      return WHEEL_OK;
   }

   uint32_t Renderer::AtlasBuffer(const wcl::string& atlas_name, const wcl::string& sprite_name,
                                  uint32_t w, uint32_t h, void* data)
   {
      if (atlas.count(atlas_name) == 0)
      {
         log(ERROR, "Can't add sprite '",sprite_name,"' to atlas '",atlas_name,"', atlas doesn't exist\n");
         return WHEEL_RESOURCE_UNAVAILABLE;
      }

      if (atlas[atlas_name].stored.count(sprite_name) != 0)
      {
         log(ERROR, "Can't add sprite '",sprite_name,"' to atlas '",atlas_name,"', name is not unique\n");
         return WHEEL_INVALID_VALUE;
      }

      wheel::rect_t r = atlas[atlas_name].atlas.Fit(w, h);
      if (r.w == 0)
      {
         log(ERROR, "Can't add sprite '",sprite_name,"' to atlas '",atlas_name,"', atlas is full\n");
         return WHEEL_ATLAS_FULL;
      }

      atlas[atlas_name].atlas.Prune(r);
      atlas[atlas_name].stored[sprite_name] = r;

      UploadTextureData(atlas_name, r.x, r.y, r.w, r.h, data);

      return WHEEL_OK;
   }

   uint32_t Renderer::GetAtlasPos(const wcl::string& atlas_name, const wcl::string& sprite_name,
                                  wheel::rect_t* result)
   {
      if (atlas.count(atlas_name) == 0)
      {
         log(ERROR, "Trying to access texture atlas '", atlas_name,"' that doesn't exist.\n");
         return WHEEL_RESOURCE_UNAVAILABLE;
      }

      if (atlas[atlas_name].stored.count(sprite_name) == 0)
      {
//       this log message causes horrible spam with fonts, since they autoload.
//         log(WARNING, "Atlas '",atlas_name,"' doesn't have sprite '",sprite_name,"'\n");
         return WHEEL_RESOURCE_UNAVAILABLE;
      }

      *result = atlas[atlas_name].stored[sprite_name];

      return WHEEL_OK;
   }

   int32_t Renderer::GetTU(const wcl::string& ident)
   {
      int i;
      for (i = 0; i < texture_unit.count(); ++i)
      {
         if (texture_unit[i].current() == ident)
            return i;
      }

      return -1;
   }

}
