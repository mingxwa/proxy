module;

#include <proxy/proxy_macros.h>

#include <tuple>
#include <type_traits>
#include <utility>

export module foo;

import proxy.v5;

extern "C++" {
PRO_ENABLE_MEMBER(GetFoo);
PRO_DEF_FREE_DISPATCH(FreeGetBar, GetBar);
PRO_DEF_FREE_AS_MEM_DISPATCH(FreeGetBaz, GetBaz);
}

export struct Foo : pro::facade_builder::build {
  struct Convention {
    static constexpr bool is_direct = false;
    using dispatch_type = pro::member_dispatch<"GetFoo">;
    using overload_type = int() const;
    using access_type = pro::member_access<"GetFoo">;
  };
  using convention_types = std::tuple<Convention>;
};

export struct Bar
    : pro::facade_builder::add_convention<FreeGetBar, int() const>::build {};

export struct Baz
    : pro::facade_builder::add_convention<FreeGetBaz, int() const>::build {};
