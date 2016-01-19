#include "include/console.h"

namespace yam
{
   Console::Console(BaseEngine* engine) : target(engine)
   {
   }

   Console::~Console()
   {
   }

   uint32_t Console::RunScript(const wcl::string& file)
   {
      wcl::buffer_t* script_buf = wheel::GetBuffer(file);

      if (script_buf == nullptr)
      {
         std::cout << file << " not found\n";
         return WHEEL_RESOURCE_UNAVAILABLE;
      }

      RunScriptFromBuffer(script_buf);
      wheel::DeleteBuffer(file);

      return 0;
   }

   uint32_t Console::RunScriptFromBuffer(const wcl::buffer_t* buffer)
   {
      if (buffer == nullptr)
         return WHEEL_INVALID_VALUE;

      std::vector<wcl::string> script = ((wcl::string)(buffer->to_stl_string())).split('\n');

      for (auto cmd : script)
      {
         std::cout << cmd;
      }

      return 0;
   }
}
