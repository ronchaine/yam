#ifndef YAM_SHADER
#define YAM_SHADER

#include "common.h"

namespace yam
{
   class Shader
   {
      private:
         wcl::string vertex_file;
         wcl::string fragment_file;

         wheel::flags_t status;

         int program;

         // currently used program
         static int program_in_use;

      public:
         // status flags
         constexpr static uint32_t HAS_FILES = 0x01;
         constexpr static uint32_t COMPILED  = 0x02;
         constexpr static uint32_t LINKED    = 0x04;

         Shader() : status(YAM_CLEAR_FLAGS) {};
         Shader(const wcl::string& vert, const wcl::string& frag);

         virtual ~Shader();

         uint32_t LoadFiles(const wcl::string& vert, const wcl::string& frag);
         uint32_t LoadBuffers(const char* vert, const char* frag);

         uint32_t Compile();
         uint32_t Compile(const char* vert, const char* frag);

         inline void Use()
         {
            if (status & (COMPILED | LINKED))
            {
               program_in_use = program;
               glUseProgram(program);
            }
            else yam::log(yam::WARNING, "shader_not_linked\n");
         }

         static const int CurrentProgram() { return program_in_use; }

         inline wheel::flags_t Status() { return status; }

         // Reload the shader
         void Reload();
   };
}

#endif
