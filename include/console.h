#ifndef YAM_CONSOLE_H
#define YAM_CONSOLE_H

#include <wheel.h>

#include "baseengine.h"

namespace yam
{
   class Console
   {
      private:
         BaseEngine*    target;

      protected:
      public:
         uint32_t       RunScript(const wcl::string& filename);
         uint32_t       RunScriptFromBuffer(const wcl::buffer_t* buffer);
         uint32_t       ParseCmd(const wcl::string& cmd);

         Console(BaseEngine* engine = nullptr);
        ~Console();
   };
}

#endif
