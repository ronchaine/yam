#ifndef YAM_LUA_HPP
#define YAM_LUA_HPP

#include <wheel.h>

extern "C"
{
   #include "lua.h"
   #include "lualib.h"
   #include "lauxlib.h"
}

namespace wheel
{
   template<typename T>
   struct lua_id{};

   // Begin LuaState

   class LuaState
   {
      private:
         lua_State* state;
      protected:
         template<size_t, typename... Ts>
         struct _pop
         {
            template<typename T>
            static std::tuple<T> worker(const LuaState& state, const int index)
            {
               return std::make_tuple(state.Read<T>(index));
            }

            template<typename T1, typename T2, typename... Tn>
            static std::tuple<T1, T2, Tn...> worker(const LuaState& state, const int index)
            {
               std::tuple<T1> head = std::make_tuple(state.Read<T1>(index));
               return std::tuple_cat(head, worker<T2, Tn...>(state, index+1));
            }

            typedef std::tuple<Ts...> type;
            static type apply(LuaState &s)
            {
               auto rval = worker<Ts...>(s, 1);
               lua_pop(s.state, sizeof...(Ts));
               return rval;
            }
         };

         template<typename T>
         struct _pop<0, T>
         {
            typedef void type;
            static type apply(LuaState& s) {}
         };

         template<typename T>
         struct _pop<1, T>
         {
            typedef T type;
            static type apply(LuaState &s)
            {
               T ret = s.Read<T>(-1);

               lua_pop(s.state, 1);
               return ret;
            }
         };

         template<typename... T>
         typename _pop<sizeof...(T), T...>::type Pop()
         {
            return _pop<sizeof...(T), T...>::apply(*this);
         }


      public:
         void operator()(const std::string& cmd);

         // Remove copy assignment
         LuaState &operator=(const LuaState& other) = delete;

         // Constructors

         // No copy constructors
         LuaState(const LuaState& other) = delete;

         // Normal move constructor
         LuaState(LuaState&& other) : state(other.state) { other.state = nullptr; }

         // Default constructor
         LuaState() : state(luaL_newstate())
         {
            luaL_openlibs(state);
         }

         ~LuaState()
         {
            if (state != nullptr)
               lua_close(state);
         }

         void Load(const wcl::string& file)
         {
            luaL_dofile(state, file.std_str().c_str());
         }

         // Dealing with the stack
         void Push() {}
         void Push(const int);
         void Push(const double);
         void Push(const std::string&);
         void Push(const bool);

         template<typename T, typename... Ts>
         void Push(const T value, const Ts... values)
         {
            Push(value);
            Push(values...);
         }

         template<typename T> T Read(int index) const;

         template<typename... Ret, typename... Args>
         typename _pop<sizeof...(Ret), Ret...>::type Call(const std::string& fun, const Args&... args)
         {
            lua_getglobal(state, fun.c_str());

            const int num_args = sizeof...(Args);
            const int num_rvals = sizeof...(Ret);

            Push(args...);

            lua_call(state, num_args, num_rvals);

            return Pop<Ret...>();
         }

         lua_State* ptr() { return state; }

         template<typename Ret, typename... Args>
         void Register(const std::string& name,
                       std::function<Ret(Args...)> fun)
         {
            auto tmp = std::unique_ptr<LuaBaseFunction>
            {

            };
         }
   };

   inline void LuaState::Push(const int value) { lua_pushinteger(state, value); }
   inline void LuaState::Push(const double value) { lua_pushnumber(state, value); }
   inline void LuaState::Push(const bool value) { lua_pushboolean(state, value); }
   inline void LuaState::Push(const std::string& value)
   {
      lua_pushstring(state, value.c_str());
   }

   inline void LuaState::operator()(const std::string& cmd)
   {
      luaL_dostring(state, cmd.c_str());
   }

   template<>
   inline double LuaState::Read(int index) const
   {
      return lua_tonumber(state, index);
   }

   template<>
   inline int LuaState::Read(int index) const
   {
      return lua_tointeger(state, index);
   }

   template<>
   inline std::string LuaState::Read(int index) const
   {
      return (std::string)lua_tostring(state, index);
   }

   template<>
   inline bool LuaState::Read(int index) const
   {
      return lua_toboolean(state, index);
   }

   // End LuaState

   // Dispatching

   struct LuaBaseFunction
   {
      virtual ~LuaBaseFunction() {}
      virtual int Apply(lua_State* s) = 0;
   };

   int lua_dispatch(lua_State* l)
   {
      LuaBaseFunction* function = (LuaBaseFunction*) lua_touserdata(l, lua_upvalueindex(1));
      return function->Apply(l);
   }

   template <int N, typename Ret, typename... Args>
   class LuaFunction : public LuaBaseFunction
   {
      private:
         std::function<Ret(Args...)> function;
         std::string name;
         lua_State** state;

      public:
         LuaFunction(lua_State* &l,
                     const std::string& name,
                     Ret(*function)(Args...))
                     : LuaFunction(l, name, std::function<Ret(Args...)>{function}) {}

         LuaFunction(lua_State* &l,
                     const std::string& name,
                     std::function<Ret(Args...)> function) : function(function),
                     name(name), state(&l)
         {
            lua_pushlightuserdata(l, (void*)static_cast<LuaBaseFunction *>(this));
            lua_pushcclosure(l, &lua_dispatch, 1);
            lua_setglobal(l, name.c_str());
         }

         LuaFunction(const LuaFunction& other) = delete;
         LuaFunction(LuaFunction&& other) : function(other.function),
                                            name(other.name),
                                            state(other.state)
         {
            other.state = nullptr;
         }

         ~LuaFunction()
         {
            if (state != nullptr && *state != nullptr)
            {
               lua_pushnil(*state);
               lua_setglobal(*state, name.c_str());
            }
         }
   };

   template <typename T>
   struct is_lua_primitive
   { static constexpr bool value = false; };

   template<>
   struct is_lua_primitive<int>
   { static constexpr bool value = true; };

   template<>
   struct is_lua_primitive<unsigned int>
   { static constexpr bool value = true; };

   template<>
   struct is_lua_primitive<bool>
   { static constexpr bool value = true; };

   template<>
   struct is_lua_primitive<double>
   { static constexpr bool value = true; };

   template<>
   struct is_lua_primitive<std::string>
   { static constexpr bool value = true; };

   template<typename T>
   typename std::enable_if<
      !is_lua_primitive<typename std::decay<T>::type>::value, T>::type
      lua_get_value(lua_id<T>, lua_State *l, const int index) {
      return lua_get_value(lua_id<T&>{}, l, index);
   }

   inline bool lua_get_value(lua_id<bool>, lua_State* l, const int index)
   {
      return lua_toboolean(l, index);
   }

   inline int lua_get_value(lua_id<int>, lua_State* l, const int index)
   {
      return lua_tointeger(l, index);
   }

   inline unsigned int lua_get_value(lua_id<unsigned int>, lua_State* l, const int index)
   {
      return static_cast<unsigned>(lua_tointeger(l, index));
   }

   inline double lua_get_value(lua_id<double>, lua_State* l, const int index)
   {
      return lua_tonumber(l, index);
   }

   inline std::string lua_get_value(lua_id<std::string>, lua_State* l, const int index)
   {
      size_t size;
      const char* buffer = lua_tolstring(l, index, &size);
      return std::string(buffer, size);
   }

   template <size_t... Is>
   struct lua_indices{};

   template <size_t N, size_t... Is>
   struct lua_indices_builder : lua_indices_builder<N-1, N-1, Is...> {};

   template <size_t... Is>
   struct lua_indices_builder<0, Is...>
   {
      using type = lua_indices<Is...>;
   };

   template <typename... T, size_t... N>
   std::tuple<T...> lua_get_args(lua_State* state, lua_indices<N...>)
   {
      return std::make_tuple(lua_get_value<T>(state, N+1)...);
   }

   template <typename... T>
   std::tuple<T...> lua_get_args(lua_State* state)
   {
      constexpr size_t num_args = sizeof...(T);
      return lua_get_args<T...>(state, typename lua_indices_builder<num_args>::type());
   }

   template <typename Ret, typename... Args, std::size_t... N>
   Ret lua_lift(std::function<Ret(Args...)> fun,
                std::tuple<Args...> args,
                lua_indices<N...>)
   {
      return fun(std::get<N>(args)...);
   }

   template <typename Ret, typename... Args>
   Ret lua_lift(std::function<Ret(Args...)> fun,
                std::tuple<Args...> args)
   {
      return lua_lift(fun, args, typename lua_indices_builder<sizeof...(Args)>::type());
   }

   // Push to stack

   inline void lua_ipush(lua_State*) {}

   template <typename T>
   inline void lua_ipush(lua_State* l, T* t)
   {
      if (t == nullptr)
         lua_pushnil(l);
      else
      {
         lua_pushlightuserdata(l, t);
      }
   }

   inline void lua_ipush(lua_State* l, bool value)
   { lua_pushboolean(l, value); }

   inline void lua_ipush(lua_State* l, int value)
   { lua_pushinteger(l, value); }

   inline void lua_ipush(lua_State* l, double value)
   { lua_pushnumber(l, value); }

   inline void lua_ipush(lua_State* l, const char* value)
   { lua_pushstring(l, value); }

   inline void lua_ipush(lua_State* l, const std::string& value)
   { lua_pushlstring(l, value.c_str(), value.size()); }
}

#endif
