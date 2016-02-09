#ifndef YAM_DEBUG
#define YAM_DEBUG

#include <cstdint>
#include <wheel_core_string.h>

namespace yam
{
   constexpr static uint32_t FATAL     = 12;
   constexpr static uint32_t ERROR     = 10;
   constexpr static uint32_t WARNING   = 7;
   constexpr static uint32_t NOTE      = 5;
   constexpr static uint32_t FAILURE   = 2;
   constexpr static uint32_t SUCCESS   = 1;
   constexpr static uint32_t MESSAGE   = 0;

   class OutputTarget
   {
      private:
         bool output_to_stdout;
         uint32_t  priority;
         uint64_t* frame_ptr;

      public:
         void set_stdout(bool value)
         {
            output_to_stdout = value;
         }

         void set_frameptr(const uint64_t* ptr)
         {
            frame_ptr = (uint64_t*)ptr;
         }

         OutputTarget() : output_to_stdout(true), priority(0), frame_ptr(nullptr) {}

         template <typename T>
         void worf(T fmt)
         {
            std::cout << fmt;
         }

         template <typename T, typename... Args>
         void worf(T value, Args... args)
         {
            std::cout << value;
            worf(args...);
         }

         template <typename... Args>
         void operator()(uint32_t log_priority, Args... args)
         {
            if ((log_priority < priority) && log_priority != MESSAGE)
               return;

            if (frame_ptr != nullptr)
               std::cout << "[" << *frame_ptr << "] ";

            if       (log_priority == FATAL)     std::cout << "-\033[1;91m☠\033[0m- ";
            else if  (log_priority == ERROR)     std::cout << "-\033[1;31m!\033[0m- ";
            else if  (log_priority == WARNING)   std::cout << "-\033[1;93m!\033[0m- ";
            else if  (log_priority == FAILURE)   std::cout << "-\033[1;31m✗\033[0m- ";
            else if  (log_priority == SUCCESS)   std::cout << "-\033[1;32m✔\033[0m- ";
            else if  (log_priority == NOTE)      std::cout << "-\033[1;94mℹ\033[0m- ";

            worf(args...);
         }
   };

   extern OutputTarget log;
}

#endif
