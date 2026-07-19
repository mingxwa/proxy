// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V4_DETAIL_DISPATCH_H_
#define MSFT_PROXY_V4_DETAIL_DISPATCH_H_

#include <exception>
#if __cpp_impl_reflection >= 202506L
#include <meta>
#include <string_view>
#include <vector>
#endif // __cpp_impl_reflection >= 202506L

#include "core.h"

namespace pro::inline v4 {

namespace detail {

template <std::size_t N>
struct sign {
  consteval sign(const char (&str)[N + 1]) {
    if (str[N] != '\0') {
      PRO4D_UNREACHABLE();
    }
    for (std::size_t i = 0; i < N; ++i) {
      value[i] = str[i];
    }
  }

  char value[N];
};
template <std::size_t N>
sign(const char (&str)[N]) -> sign<N - 1u>;

// When std::reference_constructs_from_temporary_v (C++23) is not available, we
// fall back to a conservative approximation that disallows binding a temporary
// to a reference type if the source type is not a reference or if the source
// and target reference types are not compatible.
template <class T, class U>
concept explicitly_convertible =
    std::is_constructible_v<U, T> &&
#if __cpp_lib_reference_from_temporary >= 202202L
    !std::reference_constructs_from_temporary_v<U, T>;
#else
    (!std::is_reference_v<U> ||
     (std::is_reference_v<T> &&
      std::is_convertible_v<std::add_pointer_t<std::remove_reference_t<T>>,
                            std::add_pointer_t<std::remove_reference_t<U>>>));
#endif // __cpp_lib_reference_from_temporary >= 202202L

struct noreturn_conversion {
  template <class T>
  [[noreturn]] PRO4D_STATIC_CALL(T, std::in_place_type_t<T>) {
    PRO4D_UNREACHABLE();
  }
};
using wildcard = converter<noreturn_conversion>;

} // namespace detail

template <detail::sign Sign, bool Rhs = false>
struct operator_dispatch;

#define PRO4D_DEF_LHS_LEFT_OP_ACCESSOR(oq, pq, ne, ...)                        \
  template <class P, class D, class R>                                         \
  struct accessor<P, D, R() oq ne> {                                           \
    PRO4D_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(__VA_ARGS__)                       \
    R __VA_ARGS__() oq ne {                                                    \
      return invoke<D, R() oq ne>(static_cast<P pq>(*this));                   \
    }                                                                          \
  }
#define PRO4D_DEF_LHS_UNARY_OP_ACCESSOR(oq, pq, ne, ...)                       \
  template <class P, class D, class R>                                         \
  struct accessor<P, D, R() oq ne> {                                           \
    PRO4D_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(__VA_ARGS__)                       \
    decltype(auto) __VA_ARGS__() oq ne {                                       \
      invoke<D, R() oq ne>(static_cast<P pq>(*this));                          \
      return static_cast<P pq>(*this);                                         \
    }                                                                          \
  };                                                                           \
  template <class P, class D, class R>                                         \
  struct accessor<P, D, R(int) oq ne> {                                        \
    PRO4D_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(__VA_ARGS__)                       \
    R __VA_ARGS__(int) oq ne {                                                 \
      return invoke<D, R(int) oq ne>(static_cast<P pq>(*this), 0);             \
    }                                                                          \
  }
#define PRO4D_DEF_LHS_BINARY_OP_ACCESSOR PRO4D_DEF_MEM_ACCESSOR
#define PRO4D_DEF_LHS_ALL_OP_ACCESSOR PRO4D_DEF_MEM_ACCESSOR
#define PRO4D_LHS_LEFT_OP_DISPATCH_BODY_IMPL(...)                              \
  template <class T>                                                           \
  PRO4D_STATIC_CALL(decltype(auto), T&& self)                                  \
  PRO4D_DIRECT_FUNC_IMPL(__VA_ARGS__ std::forward<T>(self))
#define PRO4D_LHS_UNARY_OP_DISPATCH_BODY_IMPL(...)                             \
  template <class T>                                                           \
  PRO4D_STATIC_CALL(decltype(auto), T&& self)                                  \
  PRO4D_DIRECT_FUNC_IMPL(__VA_ARGS__ std::forward<T>(self)) template <class T> \
  PRO4D_STATIC_CALL(decltype(auto), T&& self, int)                             \
  PRO4D_DIRECT_FUNC_IMPL(std::forward<T>(self) __VA_ARGS__)
#define PRO4D_LHS_BINARY_OP_DISPATCH_BODY_IMPL(...)                            \
  template <class T, class Arg>                                                \
  PRO4D_STATIC_CALL(decltype(auto), T&& self, Arg&& arg)                       \
  PRO4D_DIRECT_FUNC_IMPL(std::forward<T>(self)                                 \
                             __VA_ARGS__ std::forward<Arg>(arg))
#define PRO4D_LHS_ALL_OP_DISPATCH_BODY_IMPL(...)                               \
  PRO4D_LHS_LEFT_OP_DISPATCH_BODY_IMPL(__VA_ARGS__)                            \
  PRO4D_LHS_BINARY_OP_DISPATCH_BODY_IMPL(__VA_ARGS__)
#define PRO4D_LHS_OP_DISPATCH_IMPL(type, ...)                                  \
  template <>                                                                  \
  struct operator_dispatch<#__VA_ARGS__, false> {                              \
    PRO4D_LHS_##type##_OP_DISPATCH_BODY_IMPL(__VA_ARGS__)                      \
        PRO4D_DEF_ACCESSOR_TEMPLATE(                                           \
            MEM, PRO4D_DEF_LHS_##type##_OP_ACCESSOR, operator __VA_ARGS__)     \
  };

#define PRO4D_DEF_RHS_OP_ACCESSOR(oq, pq, ne, ...)                             \
  template <class P, class D, class R, class Arg>                              \
  struct accessor<P, D, R(Arg) oq ne> {                                        \
    friend R operator __VA_ARGS__(Arg arg, P pq self) ne {                     \
      return invoke<D, R(Arg) oq ne>(static_cast<P pq>(self),                  \
                                     std::forward<Arg>(arg));                  \
    }                                                                          \
    PRO4D_DEBUG(                                                             \
      accessor() noexcept { std::ignore = &pro_symbol_guard; }               \
                                                                             \
    private:                                                                 \
      static inline R pro_symbol_guard(Arg arg, P pq self) {                 \
        return std::forward<Arg>(arg) __VA_ARGS__ static_cast<P pq>(self);   \
      }                                                                      \
    ) \
  }
#define PRO4D_RHS_OP_DISPATCH_IMPL(...)                                        \
  template <>                                                                  \
  struct operator_dispatch<#__VA_ARGS__, true> {                               \
    template <class T, class Arg>                                              \
    PRO4D_STATIC_CALL(decltype(auto), T&& self, Arg&& arg)                     \
    PRO4D_DIRECT_FUNC_IMPL(std::forward<Arg>(arg)                              \
                               __VA_ARGS__ std::forward<T>(self))              \
        PRO4D_DEF_ACCESSOR_TEMPLATE(FREE, PRO4D_DEF_RHS_OP_ACCESSOR,           \
                                    __VA_ARGS__)                               \
  };

#define PRO4D_EXTENDED_BINARY_OP_DISPATCH_IMPL(...)                            \
  PRO4D_LHS_OP_DISPATCH_IMPL(ALL, __VA_ARGS__)                                 \
  PRO4D_RHS_OP_DISPATCH_IMPL(__VA_ARGS__)

#define PRO4D_BINARY_OP_DISPATCH_IMPL(...)                                     \
  PRO4D_LHS_OP_DISPATCH_IMPL(BINARY, __VA_ARGS__)                              \
  PRO4D_RHS_OP_DISPATCH_IMPL(__VA_ARGS__)

#define PRO4D_DEF_LHS_ASSIGNMENT_OP_ACCESSOR(oq, pq, ne, ...)                  \
  template <class P, class D, class R, class Arg>                              \
  struct accessor<P, D, R(Arg) oq ne> {                                        \
    PRO4D_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(__VA_ARGS__)                       \
    decltype(auto) __VA_ARGS__(Arg arg) oq ne {                                \
      invoke<D, R(Arg) oq ne>(static_cast<P pq>(*this),                        \
                              std::forward<Arg>(arg));                         \
      return static_cast<P pq>(*this);                                         \
    }                                                                          \
  }
#define PRO4D_DEF_RHS_ASSIGNMENT_OP_ACCESSOR(oq, pq, ne, ...)                  \
  template <class P, class D, class R, class Arg>                              \
  struct accessor<P, D, R(Arg&) oq ne> {                                       \
    friend Arg& operator __VA_ARGS__(Arg& arg, P pq self) ne {                 \
      invoke<D, R(Arg&) oq ne>(static_cast<P pq>(self), arg);                  \
      return arg;                                                              \
    }                                                                          \
    PRO4D_DEBUG(                                                               \
        accessor() noexcept { std::ignore = &pro_symbol_guard; }               \
                                                                               \
        private : static inline Arg& pro_symbol_guard(                         \
            Arg& arg,                                                          \
            P pq self) { return arg __VA_ARGS__ static_cast<P pq>(self); })    \
  }
#define PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(...)                                 \
  template <>                                                                  \
  struct operator_dispatch<#__VA_ARGS__, false> {                              \
    template <class T, class Arg>                                              \
    PRO4D_STATIC_CALL(decltype(auto), T&& self, Arg&& arg)                     \
    PRO4D_DIRECT_FUNC_IMPL(std::forward<T>(self)                               \
                               __VA_ARGS__ std::forward<Arg>(arg))             \
        PRO4D_DEF_ACCESSOR_TEMPLATE(                                           \
            MEM, PRO4D_DEF_LHS_ASSIGNMENT_OP_ACCESSOR, operator __VA_ARGS__)   \
  };                                                                           \
  template <>                                                                  \
  struct operator_dispatch<#__VA_ARGS__, true> {                               \
    template <class T, class Arg>                                              \
    PRO4D_STATIC_CALL(decltype(auto), T&& self, Arg&& arg)                     \
    PRO4D_DIRECT_FUNC_IMPL(std::forward<Arg>(arg)                              \
                               __VA_ARGS__ std::forward<T>(self))              \
        PRO4D_DEF_ACCESSOR_TEMPLATE(FREE,                                      \
                                    PRO4D_DEF_RHS_ASSIGNMENT_OP_ACCESSOR,      \
                                    __VA_ARGS__)                               \
  };

PRO4D_EXTENDED_BINARY_OP_DISPATCH_IMPL(+)
PRO4D_EXTENDED_BINARY_OP_DISPATCH_IMPL(-)
PRO4D_EXTENDED_BINARY_OP_DISPATCH_IMPL(*)
PRO4D_BINARY_OP_DISPATCH_IMPL(/)
PRO4D_BINARY_OP_DISPATCH_IMPL(%)
PRO4D_LHS_OP_DISPATCH_IMPL(UNARY, ++)
PRO4D_LHS_OP_DISPATCH_IMPL(UNARY, --)
PRO4D_BINARY_OP_DISPATCH_IMPL(==)
PRO4D_BINARY_OP_DISPATCH_IMPL(!=)
PRO4D_BINARY_OP_DISPATCH_IMPL(>)
PRO4D_BINARY_OP_DISPATCH_IMPL(<)
PRO4D_BINARY_OP_DISPATCH_IMPL(>=)
PRO4D_BINARY_OP_DISPATCH_IMPL(<=)
PRO4D_BINARY_OP_DISPATCH_IMPL(<=>)
PRO4D_LHS_OP_DISPATCH_IMPL(LEFT, !)
PRO4D_BINARY_OP_DISPATCH_IMPL(&&)
PRO4D_BINARY_OP_DISPATCH_IMPL(||)
PRO4D_LHS_OP_DISPATCH_IMPL(LEFT, ~)
PRO4D_EXTENDED_BINARY_OP_DISPATCH_IMPL(&)
PRO4D_BINARY_OP_DISPATCH_IMPL(|)
PRO4D_BINARY_OP_DISPATCH_IMPL(^)
PRO4D_BINARY_OP_DISPATCH_IMPL(<<)
PRO4D_BINARY_OP_DISPATCH_IMPL(>>)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(+=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(-=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(*=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(/=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(%=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(&=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(|=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(^=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(<<=)
PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL(>>=)
PRO4D_BINARY_OP_DISPATCH_IMPL(, )
PRO4D_BINARY_OP_DISPATCH_IMPL(->*)

template <>
struct operator_dispatch<"()", false> {
  template <class T, class... Args>
  PRO4D_STATIC_CALL(decltype(auto), T&& self, Args&&... args)
  PRO4D_DIRECT_FUNC_IMPL(std::forward<T>(self)(std::forward<Args>(args)...))
      PRO4D_DEF_ACCESSOR_TEMPLATE(MEM, PRO4D_DEF_MEM_ACCESSOR, operator())
};
template <>
struct operator_dispatch<"[]", false> {
#if __cpp_multidimensional_subscript >= 202110L
  template <class T, class... Args>
  PRO4D_STATIC_CALL(decltype(auto), T&& self, Args&&... args)
  PRO4D_DIRECT_FUNC_IMPL(std::forward<T>(self)[std::forward<Args>(args)...])
#else
  template <class T, class Arg>
  PRO4D_STATIC_CALL(decltype(auto), T&& self, Arg&& arg)
  PRO4D_DIRECT_FUNC_IMPL(std::forward<T>(self)[std::forward<Arg>(arg)])
#endif // __cpp_multidimensional_subscript >= 202110L
      PRO4D_DEF_ACCESSOR_TEMPLATE(MEM, PRO4D_DEF_MEM_ACCESSOR, operator[])
};

#undef PRO4D_ASSIGNMENT_OP_DISPATCH_IMPL
#undef PRO4D_DEF_RHS_ASSIGNMENT_OP_ACCESSOR
#undef PRO4D_DEF_LHS_ASSIGNMENT_OP_ACCESSOR
#undef PRO4D_BINARY_OP_DISPATCH_IMPL
#undef PRO4D_EXTENDED_BINARY_OP_DISPATCH_IMPL
#undef PRO4D_RHS_OP_DISPATCH_IMPL
#undef PRO4D_DEF_RHS_OP_ACCESSOR
#undef PRO4D_LHS_OP_DISPATCH_IMPL
#undef PRO4D_LHS_ALL_OP_DISPATCH_BODY_IMPL
#undef PRO4D_LHS_BINARY_OP_DISPATCH_BODY_IMPL
#undef PRO4D_LHS_UNARY_OP_DISPATCH_BODY_IMPL
#undef PRO4D_LHS_LEFT_OP_DISPATCH_BODY_IMPL
#undef PRO4D_DEF_LHS_ALL_OP_ACCESSOR
#undef PRO4D_DEF_LHS_BINARY_OP_ACCESSOR
#undef PRO4D_DEF_LHS_UNARY_OP_ACCESSOR
#undef PRO4D_DEF_LHS_LEFT_OP_ACCESSOR

#if __cpp_impl_reflection >= 202506L

namespace detail {

template <std::size_t N>
consteval std::string_view as_sv(const sign<N>& s) {
  return std::string_view{s.value, N};
}

template <std::meta::info M, class Self, class... Args>
struct mem_call_candidate {
  PRO4D_STATIC_CALL(decltype(auto), Self self, Args... args) noexcept(
      noexcept(static_cast<Self>(self).[:M:](static_cast<Args>(args)...))) {
    return static_cast<Self>(self).[:M:](static_cast<Args>(args)...);
  }
};

template <std::meta::info M, class Self, class... Args>
struct mem_call_deleted_candidate {
  PRO4D_STATIC_CALL(void, Self, Args...) = delete;
};

template <std::meta::info M, class Self, class... Args>
constexpr bool mem_call_field_applicable =
    requires { std::declval<Self>().[:M:](std::declval<Args>()...); };
template <std::meta::info M, class Self>
struct mem_call_field_candidate {
  template <class... Args>
  PRO4D_STATIC_CALL(decltype(auto), Self self, Args&&... args) noexcept(
      noexcept(std::declval<Self>().[:M:](std::declval<Args>()...)))
    requires(mem_call_field_applicable<M, Self, Args...>)
  {
    return static_cast<Self>(self).[:M:](std::forward<Args>(args)...);
  }
};

template <class... Cs>
struct PRO4D_ENFORCE_EBO mem_call_set : Cs... {
  using Cs::operator()...;
};

template <class T, sign Impl>
struct mem_call_traits : std::type_identity<void> {};
template <class T, sign Impl>
  requires(std::is_class_v<T> || std::is_union_v<T>)
struct mem_call_traits<T, Impl> {
  static consteval std::meta::info compute() {
    std::vector<std::meta::info> fns;
    std::meta::info field;
    {
      auto ctx = std::meta::access_context::unprivileged();
      std::vector<std::meta::info> classes{^^T};
      for (std::size_t i = 0u; i < classes.size(); ++i) {
        bool declared = false;
        for (std::meta::info m : std::meta::members_of(classes[i], ctx)) {
          if (!std::meta::has_identifier(m) ||
              std::meta::identifier_of(m) != as_sv(Impl)) {
            continue;
          }
          declared = true;
          if (std::meta::is_function(m)) {
            if (!std::meta::is_volatile(m)) {
              fns.push_back(m);
            }
          } else if (std::meta::is_nonstatic_data_member(m) ||
                     std::meta::is_variable(m)) {
            if (field != std::meta::info{}) {
              return ^^void;
            }
            field = m;
          }
        }
        if (declared) {
          continue;
        }
        for (std::meta::info b : std::meta::bases_of(classes[i], ctx)) {
          std::meta::info bt = std::meta::type_of(b);
          bool visited = false;
          for (std::meta::info c : classes) {
            visited |= c == bt;
          }
          if (!visited) {
            classes.push_back(bt);
          }
        }
      }
    }
    if (fns.empty() == (field == std::meta::info{})) {
      return ^^void;
    }
    std::meta::info t = ^^T, ct = std::meta::add_const(t);
    std::vector<std::meta::info> all_quals{std::meta::add_lvalue_reference(t),
                                           std::meta::add_rvalue_reference(t),
                                           std::meta::add_lvalue_reference(ct),
                                           std::meta::add_rvalue_reference(ct)};
    std::vector<std::meta::info> candidates;
    if (fns.empty()) {
      for (std::meta::info self : all_quals) {
        candidates.push_back(std::meta::substitute(
            ^^mem_call_field_candidate,
            {
                std::meta::reflect_constant(field), self}));
      }
    } else {
      for (std::meta::info m : fns) {
        std::vector<std::meta::info> params = std::meta::parameters_of(m);
        bool has_explicit_obj =
            !params.empty() &&
            std::meta::is_explicit_object_parameter(params[0u]);
        std::vector<std::meta::info> selfs;
        if (has_explicit_obj) {
          selfs = {std::meta::type_of(params[0u])};
        } else if (std::meta::is_static_member(m)) {
          selfs = all_quals;
        } else {
          std::meta::info base = std::meta::is_const(m) ? ct : t;
          if (std::meta::is_lvalue_reference_qualified(m)) {
            selfs = {std::meta::add_lvalue_reference(base)};
          } else if (std::meta::is_rvalue_reference_qualified(m)) {
            selfs = {std::meta::add_rvalue_reference(base)};
          } else {
            selfs = {std::meta::add_lvalue_reference(base),
                     std::meta::add_rvalue_reference(base)};
          }
        }
        std::meta::info tmpl = std::meta::is_deleted(m)
                                   ? ^^mem_call_deleted_candidate
                                   : ^^mem_call_candidate;
        std::size_t first = has_explicit_obj ? 1u : 0u;
        std::vector<std::meta::info> candidate(params.size() - first + 2u);
        candidate[0] = std::meta::reflect_constant(m);
        for (std::size_t j = first; j < params.size(); ++j) {
          candidate[j - first + 2u] = std::meta::type_of(params[j]);
        }
        do {
          for (std::meta::info self : selfs) {
            candidate[1u] = self;
            candidates.push_back(std::meta::substitute(tmpl, candidate));
          }
          candidate.pop_back();
        } while (candidate.size() >= 2u &&
                 std::meta::has_default_argument(
                     params[first + candidate.size() - 2u]));
      }
    }
    return substitute(^^mem_call_set, candidates);
  }

  using type = [:compute():];
};
template <class T, sign Impl>
using mem_call_set_t = typename mem_call_traits<T, Impl>::type;

template <class A, class P, class D, class O>
struct mem_accessor_op;
#define PRO4D_DEF_MEM_ACCESSOR_OP(oq, pq, ne, ...)                             \
  template <class A, class P, class D, class R, class... Args>                 \
  struct mem_accessor_op<A, P, D, R(Args...) oq ne> {                          \
    R operator()(Args... args) oq ne {                                         \
      return invoke<D, R(Args...) oq ne>(                                      \
          static_cast<P pq>(reinterpret_cast<A pq>(*this)),                    \
          std::forward<Args>(args)...);                                        \
    }                                                                          \
  }
PRO4D_DEF_OVERLOAD_SPECIALIZATIONS(PRO4D_DEF_MEM_ACCESSOR_OP)
#undef PRO4D_DEF_MEM_ACCESSOR_OP

template <class A, class P, class D, class... Os>
struct PRO4D_ENFORCE_EBO mem_accessor_ops : mem_accessor_op<A, P, D, Os>... {
  using mem_accessor_op<A, P, D, Os>::operator()...;
};

template <sign Func, class P, class D, class... Os>
struct mem_accessor_traits {
  struct type;
  consteval {
    std::meta::data_member_options options;
    options.name = as_sv(Func);
    options.no_unique_address = true;
    define_aggregate(
        ^^type,
        {
            data_member_spec(^^mem_accessor_ops<type, P, D, Os...>, options)});
  }
};

} // namespace detail

template <detail::sign Impl, detail::sign Func = Impl>
struct mem_dispatch {
  template <class T, class... Args>
  PRO4D_STATIC_CALL(decltype(auto), T&& self, Args&&... args) noexcept(
      std::is_nothrow_invocable_v<
          detail::mem_call_set_t<std::remove_cvref_t<T>, Impl>, T, Args...>)
    requires(std::is_invocable_v<
             detail::mem_call_set_t<std::remove_cvref_t<T>, Impl>, T, Args...>)
  {
    return detail::mem_call_set_t<std::remove_cvref_t<T>, Impl>{}(
        std::forward<T>(self), std::forward<Args>(args)...);
  }

  template <class P, class D, class... Os>
  using accessor =
      typename detail::mem_accessor_traits<Func, P, D, Os...>::type;
};

#endif // __cpp_impl_reflection >= 202506L

struct implicit_conversion_dispatch : detail::cast_dispatch_base<false, false> {
  template <class T>
  PRO4D_STATIC_CALL(T&&, T&& self) noexcept {
    return std::forward<T>(self);
  }
};
struct explicit_conversion_dispatch : detail::cast_dispatch_base<true, false> {
  template <class T>
  PRO4D_STATIC_CALL(auto, T&& self) noexcept {
    return detail::converter{
        [&self]<class U>(std::in_place_type_t<U>) noexcept(
            std::is_nothrow_constructible_v<U, T>) -> U
          requires(detail::explicitly_convertible < T &&, U >)
        { return static_cast<U>(std::forward<T>(self)); }};
  }
};
using conversion_dispatch = explicit_conversion_dispatch;

class not_implemented : public std::exception {
public:
  char const* what() const noexcept override {
    return "pro::v4::not_implemented";
  }
};

template <class D>
struct weak_dispatch : D {
  using D::operator();
  template <class... Args>
  [[noreturn]] PRO4D_STATIC_CALL(detail::wildcard, Args&&...)
    requires(!std::is_invocable_v<D, Args...>)
  {
    PRO4D_THROW(not_implemented{});
  }
};

} // namespace pro::inline v4

#endif // MSFT_PROXY_V4_DETAIL_DISPATCH_H_
