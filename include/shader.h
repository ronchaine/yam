#ifndef YAM_SHADER
#define YAM_SHADER

#include "common.h"

namespace yam
{
   constexpr uint32_t NO_TEXTURE   = 0x00;
   constexpr uint32_t RGBA_TEXTURE = 0x01;
   constexpr uint32_t PAL_TEXTURE  = 0x02;

   class Shader
   {
      struct UniformAssignment
      {
         union
         {
            struct { GLint sx,sy,sz,sw; };
            struct { GLuint ux,uy,uz,uw; };
            struct { GLfloat x,y,z,w; };
         };

         int order;
         int type;

         // 4D
         UniformAssignment(GLint x, GLint y, GLint z, GLint w) :
            sx(x), sy(y), sz(z), sw(w), order(4), type(0) {}
         UniformAssignment(GLuint x, GLuint y, GLuint z, GLuint w) :
            ux(x), uy(y), uz(z), uw(w), order(4), type(1) {}
         UniformAssignment(GLfloat x, GLfloat y, GLfloat z, GLfloat w) :
            x(x), y(y), z(z), w(w), order(4), type(2) {}

         // 3D
         UniformAssignment(GLint x, GLint y, GLint z) : sx(x), sy(y), sz(z), order(3), type(0) {}
         UniformAssignment(GLuint x, GLuint y, GLuint z) : ux(x), uy(y), uz(z), order(3), type(1){}
         UniformAssignment(GLfloat x, GLfloat y, GLfloat z) : x(x), y(y), z(z), order(3), type(2) {}

         // 2D
         UniformAssignment(GLint x, GLint y) : sx(x), sy(y), order(2), type(0) {}
         UniformAssignment(GLuint x, GLuint y) : ux(x), uy(y), order(2), type(1) {}
         UniformAssignment(GLfloat x, GLfloat y) : x(x), y(y), order(2), type(2) {}

         // 1D
         UniformAssignment(GLint x) : sx(x), order(1), type(0) {}
         UniformAssignment(GLuint x) : ux(x), order(1), type(1) {}
         UniformAssignment(GLfloat x) : x(x), order(1), type(2) {}
      };

      class UniformProxy
      {
         private:
            GLint uniform_loc;
            GLint prog;
            wheel::string name;

         public:
            UniformProxy(const wheel::string& uniformname, int prog) : prog(prog), name(uniformname)
            {
               while (glGetError() != GL_NO_ERROR);

               uniform_loc = glGetUniformLocation(prog, uniformname.std_str().c_str());

               if (glGetError() != GL_NO_ERROR)
                  log(ERROR, "Cannot find uniform '", uniformname, "'.");
            }

            void operator=(const UniformAssignment& val)
            {
               GLint oldprog = program_in_use;
               if (prog != program_in_use)
                  glUseProgram(prog);

               while (glGetError() != GL_NO_ERROR);
               if (val.type == 0) // GLint
               {
                  if (val.order == 1) glUniform1i(uniform_loc, val.sx);
                  if (val.order == 2) glUniform2i(uniform_loc, val.sx, val.sy);
                  if (val.order == 3) glUniform3i(uniform_loc, val.sx, val.sy, val.sz);
                  if (val.order == 4) glUniform4i(uniform_loc, val.sx, val.sy, val.sz, val.sw);
               } else if (val.type == 1) { // GLuint
                  if (val.order == 1) glUniform1ui(uniform_loc, val.ux);
                  if (val.order == 2) glUniform2ui(uniform_loc, val.ux, val.uy);
                  if (val.order == 3) glUniform3ui(uniform_loc, val.ux, val.uy, val.uz);
                  if (val.order == 4) glUniform4ui(uniform_loc, val.ux, val.uy, val.uz, val.uw);
               } else if (val.type == 2) { // GLfloat
                  if (val.order == 1) glUniform1f(uniform_loc, val.x);
                  if (val.order == 2) glUniform2f(uniform_loc, val.x, val.y);
                  if (val.order == 3) glUniform3f(uniform_loc, val.x, val.y, val.z);
                  if (val.order == 4) glUniform4f(uniform_loc, val.x, val.y, val.z, val.w);
               }

               if (glGetError() != GL_NO_ERROR)
               {
                  log(ERROR, "Error assigning value to shader uniform '", name, "'. (type mismatch?)\n");
               }

               if (oldprog != program_in_use)
                  glUseProgram(oldprog);
            }
      };

      private:
         wcl::string vertex_file;
         wcl::string fragment_file;

         wheel::flags_t status;

         uint32_t texture_type;

         int program;

         // currently used program
         static int program_in_use;

         // Maps textures to uniforms
         std::unordered_map<wcl::string, wcl::string> bound_textures;

      public:
         // status flags
         constexpr static uint32_t HAS_FILES = 0x01;
         constexpr static uint32_t COMPILED  = 0x02;
         constexpr static uint32_t LINKED    = 0x04;

         Shader() : status(YAM_CLEAR_FLAGS), texture_type(NO_TEXTURE) {};
         Shader(const wcl::string& vert, const wcl::string& frag, uint32_t t_type = NO_TEXTURE);

         Shader(Shader&& other);
         Shader(const Shader& other) = delete;

         virtual ~Shader();

         uint32_t LoadFiles(const wcl::string& vert, const wcl::string& frag);
         uint32_t LoadBuffers(const char* vert, const char* frag);

         uint32_t Compile();
         uint32_t Compile(const char* vert, const char* frag);

         void Bind(const wcl::string& ident, const wcl::string& uniform);
         void AddBinding(const wcl::string& ident, const wcl::string& uniform);

         void Use(bool bindtexture = true);

         static const int CurrentProgram() { return program_in_use; }

         inline wheel::flags_t Status() { return status; }

         // Reload the shader
         void Reload();

         inline UniformProxy operator[](const wcl::string& name)
         {
            return UniformProxy(name, program);
         }

   };
}

#endif
