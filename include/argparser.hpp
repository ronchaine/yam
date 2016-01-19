#ifndef ARGUMENT_PARSER_HEADER
#define ARGUMENT_PARSER_HEADER

#include <unordered_map>
#include <string>

namespace argp
{
   struct {
      std::string    id_short;
      std::string    id_long;
      std::string    description;

      bool           takes_value;
      bool           enabled;
   } argument_t;

   // map from id_long to value
   static std::unordered_map<std::string, std::string> argument;
   static std::string error;

   inline void add_argument(std::string id_long, std::string id_short, std::string description = "")
   {
   }

   inline int parse_cmdline(int argc, char* argv[])
   {
      return 0;
   }
}

#endif
