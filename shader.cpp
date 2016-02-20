#include "include/shader.h"
#include "include/renderer.h"

namespace yam
{
   // static members
   int Shader::program_in_use = 0;

   // other stuff
   Shader::Shader(const wcl::string& vert, const wcl::string& frag, uint32_t ttype) :
      vertex_file(vert), fragment_file(frag), status(HAS_FILES), texture_type(ttype)
   {
   }

   Shader::Shader(Shader&& other)
   {
      vertex_file = other.vertex_file;
      fragment_file = other.fragment_file;
      status = other.status;
      program = other.program;
      texture_type = other.texture_type;
      bound_textures = std::move(other.bound_textures);

      other.program = 0;
   }

   uint32_t Shader::Compile()
   {
      if (!(status & HAS_FILES))
         return WHEEL_RESOURCE_UNAVAILABLE;

      const wheel::buffer_t* raw_vs = wheel::GetBuffer(vertex_file);
      const wheel::buffer_t* raw_fs = wheel::GetBuffer(fragment_file);

      if (raw_fs == nullptr || raw_vs == nullptr)
      {
         if (raw_fs != nullptr)
         {
            yam::log(yam::ERROR, fragment_file, " access error\n");
         }
         if (raw_vs != nullptr)
         {
            yam::log(yam::ERROR, vertex_file, " access error\n");

            status &= ~HAS_FILES;
         }
         return WHEEL_RESOURCE_UNAVAILABLE;
      }

      ((wheel::buffer_t)(*raw_vs)).push_back('\0');
      ((wheel::buffer_t)(*raw_fs)).push_back('\0');

      uint32_t compile_status = Compile((const char*) &((*raw_vs)[0]), (const char*) &((*raw_fs)[0]));

      // Do not cache source files
      wheel::DeleteBuffer(vertex_file);
      wheel::DeleteBuffer(fragment_file);

      return compile_status;
   }

   uint32_t Shader::Compile(const char* vert, const char* frag)
   {
      int vertexshader, fragmentshader;
      int vertexcompiled, fragmentcompiled, linked;

      vertexshader = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(vertexshader, 1, (const GLchar**) &vert, 0);
      glCompileShader(vertexshader);
      glGetShaderiv(vertexshader, GL_COMPILE_STATUS, &vertexcompiled);

      if (!vertexcompiled)
      {
         std::cout << "Vertex shader compile error" << "\n";
         return YAM_SHADER_COMPILE_ERROR;
      }

      fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(fragmentshader, 1, (const GLchar**) &frag, 0);
      glCompileShader(fragmentshader);
      glGetShaderiv(fragmentshader, GL_COMPILE_STATUS, &fragmentcompiled);

      if (!fragmentcompiled)
      {
         std::cout << "Fragment shader compile error\n";
         return YAM_SHADER_COMPILE_ERROR;
      }

      status |= COMPILED;

      program = glCreateProgram();
      glAttachShader(program, vertexshader);
      glAttachShader(program, fragmentshader);
      glLinkProgram(program);
      glGetProgramiv(program, GL_LINK_STATUS, (int*)&linked);

      if (!linked)
      {
         yam::log(yam::ERROR, "Shader linking error\n");
         return YAM_ERROR;
      }

      status |= LINKED;

      return WHEEL_OK;
   }

   Shader::~Shader()
   {
      if (program_in_use == program)
      {
         glUseProgram(0);
      }

      glDeleteProgram(program);
   }

   void Shader::Reload()
   {
      bool onthefly = false;

      if (!(status & HAS_FILES))
      {
         yam::log(yam::ERROR, "no source files to reload shader from\n");
         return;
      }

      if (program_in_use == program)
      {
         onthefly = true;
         glUseProgram(0);
      }

      if (status & LINKED)
      {
         glDeleteProgram(program);
      }

      Compile();

      if (onthefly)
         Use(false);
   }

   void Shader::Use(bool rebindtextures)
   {
      if (program_in_use == program)
         return;

      if (!(status & (COMPILED | LINKED)))
      {
         yam::log(yam::WARNING, "Tried to use unlinked shader\n");
         return;
      }

      program_in_use = program;
      glUseProgram(program);

      if (!rebindtextures)
         return;

      // TODO: Bind textures to shader here

      for (auto& mat : bound_textures)
      {
         Bind(mat.first, mat.second);
      }
   }

   void Shader::Bind(const wcl::string& ident, const wcl::string& uniform)
   {
      int32_t tu;
/*
      if (renderer.texture.count(ident) == 0)
      {
         log(ERROR, "Cannot bind texture '",ident,"' to uniform '",uniform,"', texture doesn't exist\n");
         return;
      }
*/
      if ((tu = renderer.GetTU(ident)) < 0)
      {
         tu = renderer.texture_unit.get_free();
         log(NOTE, "binding unused TU ", tu, " to uniform '",uniform,"'\n");
         renderer.texture_unit[tu] = ident;
      }

      (*this)[uniform] = tu;

      return;
   }


   void Shader::AddBinding(const wcl::string& ident, const wcl::string& uniform)
   {
      log(NOTE, "Add binding of texture/atlas ", ident, " to uniform ", uniform, "\n");
      bound_textures[ident] = uniform;
   }
}
