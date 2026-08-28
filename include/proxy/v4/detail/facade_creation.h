// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V4_DETAIL_FACADE_CREATION_H_
#define MSFT_PROXY_V4_DETAIL_FACADE_CREATION_H_

#include <limits>

#include "core.h"

namespace pro::inline v4 {

namespace detail {

inline constexpr std::size_t invalid_size =
    std::numeric_limits<std::size_t>::max();
inline constexpr constraint_level invalid_cl = static_cast<constraint_level>(
    std::numeric_limits<std::underlying_type_t<constraint_level>>::min());
consteval std::size_t merge_size(std::size_t a, std::size_t b) {
  return a < b ? a : b;
}
consteval constraint_level merge_constraint(constraint_level a,
                                            constraint_level b) {
  return a < b ? b : a;
}
consteval std::size_t max_align_of(std::size_t value) {
  value &= ~value + 1u;
  return value < alignof(std::max_align_t) ? value : alignof(std::max_align_t);
}

using ptr_prototype = void* [2];

struct facet_primitive_resolver;

} // namespace detail

template <class Ss, class Cs, class Rs, std::size_t MaxSize,
          std::size_t MaxAlign, constraint_level Copyability,
          constraint_level Relocatability, constraint_level Destructibility>
struct basic_facade_builder {
  template <class D, detail::extended_overload... Os>
    requires(sizeof...(Os) > 0u)
  using add_indirect_convention = basic_facade_builder<
      Ss, detail::composite_t<Cs, detail::conv_impl<false, D, Os>...>, Rs,
      MaxSize, MaxAlign, Copyability, Relocatability, Destructibility>;
  template <class D, detail::extended_overload... Os>
    requires(sizeof...(Os) > 0u)
  using add_direct_convention = basic_facade_builder<
      Ss, detail::composite_t<Cs, detail::conv_impl<true, D, Os>...>, Rs,
      MaxSize, MaxAlign, Copyability, Relocatability, Destructibility>;
  template <class D, detail::extended_overload... Os>
    requires(sizeof...(Os) > 0u)
  using add_convention = add_indirect_convention<D, Os...>;
  template <class R>
  using add_indirect_reflection = basic_facade_builder<
      Ss, Cs, detail::composite_t<Rs, detail::refl_impl<false, R>>, MaxSize,
      MaxAlign, Copyability, Relocatability, Destructibility>;
  template <class R>
  using add_direct_reflection = basic_facade_builder<
      Ss, Cs, detail::composite_t<Rs, detail::refl_impl<true, R>>, MaxSize,
      MaxAlign, Copyability, Relocatability, Destructibility>;
  template <class R>
  using add_reflection = add_indirect_reflection<R>;
  template <facade F>
  using add_facade = basic_facade_builder<
      detail::composite_t<Ss, F>, Cs, Rs,
      detail::merge_size(MaxSize, F::max_size),
      detail::merge_size(MaxAlign, F::max_align),
      detail::merge_constraint(Copyability, F::copyability),
      detail::merge_constraint(Relocatability, F::relocatability),
      detail::merge_constraint(Destructibility, F::destructibility)>;
  template <std::size_t PtrSize,
            std::size_t PtrAlign = detail::max_align_of(PtrSize)>
    requires(detail::is_layout_well_formed(PtrSize, PtrAlign))
  using restrict_layout =
      basic_facade_builder<Ss, Cs, Rs, detail::merge_size(MaxSize, PtrSize),
                           detail::merge_size(MaxAlign, PtrAlign), Copyability,
                           Relocatability, Destructibility>;
  template <constraint_level CL>
    requires(detail::is_cl_well_formed(CL))
  using support_copy =
      basic_facade_builder<Ss, Cs, Rs, MaxSize, MaxAlign,
                           detail::merge_constraint(Copyability, CL),
                           Relocatability, Destructibility>;
  template <constraint_level CL>
    requires(detail::is_cl_well_formed(CL))
  using support_relocation =
      basic_facade_builder<Ss, Cs, Rs, MaxSize, MaxAlign, Copyability,
                           detail::merge_constraint(Relocatability, CL),
                           Destructibility>;
  template <constraint_level CL>
    requires(detail::is_cl_well_formed(CL))
  using support_destruction =
      basic_facade_builder<Ss, Cs, Rs, MaxSize, MaxAlign, Copyability,
                           Relocatability,
                           detail::merge_constraint(Destructibility, CL)>;
  template <template <class> class Skill>
  using add_skill = Skill<basic_facade_builder>;
  using build = detail::facade_impl<
      Ss, Cs, Rs,
      MaxSize == detail::invalid_size ? sizeof(detail::ptr_prototype) : MaxSize,
      MaxAlign == detail::invalid_size ? alignof(detail::ptr_prototype)
                                       : MaxAlign,
      Copyability == detail::invalid_cl ? constraint_level::none : Copyability,
      Relocatability == detail::invalid_cl ? constraint_level::trivial
                                           : Relocatability,
      Destructibility == detail::invalid_cl ? constraint_level::nothrow
                                            : Destructibility>;
  basic_facade_builder() = delete;
};
using facade_builder =
    basic_facade_builder<std::tuple<>, std::tuple<>, std::tuple<>,
                         detail::invalid_size, detail::invalid_size,
                         detail::invalid_cl, detail::invalid_cl,
                         detail::invalid_cl>;

namespace facets {

template <class F>
concept facet = std::is_invocable_v<detail::facet_primitive_resolver, F*>;

template <facet... Fs>
struct pack {};

template <facade F>
struct super {};

template <class D, detail::extended_overload... Os>
  requires(sizeof...(Os) > 0u)
struct indirect_convention {};
template <class D, detail::extended_overload... Os>
  requires(sizeof...(Os) > 0u)
struct direct_convention {};
template <class D, detail::extended_overload... Os>
  requires(sizeof...(Os) > 0u)
using convention = indirect_convention<D, Os...>;

template <detail::basic_meta R>
struct indirect_reflection {};
template <detail::basic_meta R>
struct direct_reflection {};
template <detail::basic_meta R>
using reflection = indirect_reflection<R>;

template <constraint_level CL>
  requires(detail::is_cl_well_formed(CL))
struct copyability {};
template <constraint_level CL>
  requires(detail::is_cl_well_formed(CL))
struct relocatability {};
template <constraint_level CL>
  requires(detail::is_cl_well_formed(CL))
struct destructibility {};

template <std::size_t PtrSize,
          std::size_t PtrAlign = detail::max_align_of(PtrSize)>
  requires(detail::is_layout_well_formed(PtrSize, PtrAlign))
struct layout {};

} // namespace facets

namespace detail {

struct facet_primitive_resolver {
  template <class... Fs>
  facets::pack<Fs...> operator()(facets::pack<Fs...>*) const;
  template <class F>
  facets::super<F> operator()(facets::super<F>*) const;
  template <class D, class... Os>
  facets::indirect_convention<D, Os...>
      operator()(facets::indirect_convention<D, Os...>*) const;
  template <class D, class... Os>
  facets::direct_convention<D, Os...>
      operator()(facets::direct_convention<D, Os...>*) const;
  template <class R>
  facets::indirect_reflection<R>
      operator()(facets::indirect_reflection<R>*) const;
  template <class R>
  facets::direct_reflection<R> operator()(facets::direct_reflection<R>*) const;
  template <constraint_level CL>
  facets::copyability<CL> operator()(facets::copyability<CL>*) const;
  template <constraint_level CL>
  facets::relocatability<CL> operator()(facets::relocatability<CL>*) const;
  template <constraint_level CL>
  facets::destructibility<CL> operator()(facets::destructibility<CL>*) const;
  template <std::size_t PtrSize, std::size_t PtrAlign>
  facets::layout<PtrSize, PtrAlign>
      operator()(facets::layout<PtrSize, PtrAlign>*) const;
};
template <class O, class I>
struct facet_reduction
    : facet_reduction<O, std::invoke_result_t<facet_primitive_resolver, I*>> {};

template <class F1, class F2>
struct facet_reduction_pack_impl;
template <class Ss1, class Cs1, class Rs1, std::size_t Sz1, std::size_t Al1,
          constraint_level Cp1, constraint_level Rl1, constraint_level Ds1,
          class Ss2, class Cs2, class Rs2, std::size_t Sz2, std::size_t Al2,
          constraint_level Cp2, constraint_level Rl2, constraint_level Ds2>
struct facet_reduction_pack_impl<
    facade_impl<Ss1, Cs1, Rs1, Sz1, Al1, Cp1, Rl1, Ds1>,
    facade_impl<Ss2, Cs2, Rs2, Sz2, Al2, Cp2, Rl2, Ds2>>
    : std::type_identity<
          facade_impl<composite_t<Ss1, Ss2>, composite_t<Cs1, Cs2>,
                      composite_t<Rs1, Rs2>, merge_size(Sz1, Sz2),
                      merge_size(Al1, Al2), merge_constraint(Cp1, Cp2),
                      merge_constraint(Rl1, Rl2), merge_constraint(Ds1, Ds2)>> {
};

template <class... Fs>
using incomplete_facade_t = recursive_reduction_t<
    reduction_t<facet_reduction>,
    facade_impl<std::tuple<>, std::tuple<>, std::tuple<>, invalid_size,
                invalid_size, invalid_cl, invalid_cl, invalid_cl>,
    Fs...>;

template <class O, class... Fs>
struct facet_reduction<O, facets::pack<Fs...>>
    : facet_reduction_pack_impl<O, incomplete_facade_t<Fs...>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          class F>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::super<F>>
    : std::type_identity<facade_impl<
          composite_t<Ss, F>, Cs, Rs, merge_size(Sz, F::max_size),
          merge_size(Al, F::max_align), merge_constraint(Cp, F::copyability),
          merge_constraint(Rl, F::relocatability),
          merge_constraint(Ds, F::destructibility)>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          class D, class... Os>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::indirect_convention<D, Os...>>
    : std::type_identity<
          facade_impl<Ss, composite_t<Cs, conv_impl<false, D, Os>...>, Rs, Sz,
                      Al, Cp, Rl, Ds>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          class D, class... Os>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::direct_convention<D, Os...>>
    : std::type_identity<
          facade_impl<Ss, composite_t<Cs, conv_impl<true, D, Os>...>, Rs, Sz,
                      Al, Cp, Rl, Ds>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          class R>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::indirect_reflection<R>>
    : std::type_identity<facade_impl<
          Ss, Cs, composite_t<Rs, refl_impl<false, R>>, Sz, Al, Cp, Rl, Ds>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          class R>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::direct_reflection<R>>
    : std::type_identity<facade_impl<
          Ss, Cs, composite_t<Rs, refl_impl<true, R>>, Sz, Al, Cp, Rl, Ds>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          constraint_level CL>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::copyability<CL>>
    : std::type_identity<
          facade_impl<Ss, Cs, Rs, Sz, Al, merge_constraint(Cp, CL), Rl, Ds>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          constraint_level CL>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::relocatability<CL>>
    : std::type_identity<
          facade_impl<Ss, Cs, Rs, Sz, Al, Cp, merge_constraint(Rl, CL), Ds>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          constraint_level CL>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::destructibility<CL>>
    : std::type_identity<
          facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, merge_constraint(Ds, CL)>> {};
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds,
          std::size_t PtrSize, std::size_t PtrAlign>
struct facet_reduction<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>,
                       facets::layout<PtrSize, PtrAlign>>
    : std::type_identity<facade_impl<Ss, Cs, Rs, merge_size(Sz, PtrSize),
                                     merge_size(Al, PtrAlign), Cp, Rl, Ds>> {};

template <class F>
struct defaulted_facade_traits;
template <class Ss, class Cs, class Rs, std::size_t Sz, std::size_t Al,
          constraint_level Cp, constraint_level Rl, constraint_level Ds>
struct defaulted_facade_traits<facade_impl<Ss, Cs, Rs, Sz, Al, Cp, Rl, Ds>>
    : std::type_identity<facade_impl<
          Ss, Cs, Rs, Sz == invalid_size ? sizeof(ptr_prototype) : Sz,
          Al == invalid_size ? alignof(ptr_prototype) : Al,
          Cp == invalid_cl ? constraint_level::none : Cp,
          Rl == invalid_cl ? constraint_level::trivial : Rl,
          Ds == invalid_cl ? constraint_level::nothrow : Ds>> {};

} // namespace detail

template <facets::facet... Fs>
using make_facade =
    detail::defaulted_facade_traits<detail::incomplete_facade_t<Fs...>>::type;

} // namespace pro::inline v4

#endif // MSFT_PROXY_V4_DETAIL_FACADE_CREATION_H_
