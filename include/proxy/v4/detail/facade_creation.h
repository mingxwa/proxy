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

template <class F>
struct super_facet_primitive {};
template <bool IsDirect, class D, class O>
struct convention_facet_primitive {};
template <bool IsDirect, class R>
struct reflection_facet_primitive {};
template <constraint_level CL>
struct copyability_facet_primitive {};
template <constraint_level CL>
struct relocatability_facet_primitive {};
template <constraint_level CL>
struct destructibility_facet_primitive {};
template <std::size_t MaxSize, std::size_t MaxAlign>
struct layout_facet_primitive {};
template <class Ss, class Cs, class Rs, std::size_t MaxSize,
          std::size_t MaxAlign, constraint_level Copyability,
          constraint_level Relocatability, constraint_level Destructibility>
struct pack_facet_primitive
    : facade_impl<
          Ss, Cs, Rs, MaxSize == invalid_size ? sizeof(ptr_prototype) : MaxSize,
          MaxAlign == invalid_size ? alignof(ptr_prototype) : MaxAlign,
          Copyability == invalid_cl ? constraint_level::none : Copyability,
          Relocatability == invalid_cl ? constraint_level::trivial
                                       : Relocatability,
          Destructibility == invalid_cl ? constraint_level::nothrow
                                        : Destructibility> {};

template <class F>
super_facet_primitive<F> get_facet_primitive(super_facet_primitive<F>*);
template <bool IsDirect, class D, class O>
convention_facet_primitive<IsDirect, D, O>
    get_facet_primitive(convention_facet_primitive<IsDirect, D, O>*);
template <bool IsDirect, class R>
reflection_facet_primitive<IsDirect, R>
    get_facet_primitive(reflection_facet_primitive<IsDirect, R>*);
template <constraint_level CL>
copyability_facet_primitive<CL>
    get_facet_primitive(copyability_facet_primitive<CL>*);
template <constraint_level CL>
relocatability_facet_primitive<CL>
    get_facet_primitive(relocatability_facet_primitive<CL>*);
template <constraint_level CL>
destructibility_facet_primitive<CL>
    get_facet_primitive(destructibility_facet_primitive<CL>*);
template <std::size_t MaxSize, std::size_t MaxAlign>
layout_facet_primitive<MaxSize, MaxAlign>
    get_facet_primitive(layout_facet_primitive<MaxSize, MaxAlign>*);
template <class Ss, class Cs, class Rs, std::size_t MaxSize,
          std::size_t MaxAlign, constraint_level Copyability,
          constraint_level Relocatability, constraint_level Destructibility>
pack_facet_primitive<Ss, Cs, Rs, MaxSize, MaxAlign, Copyability, Relocatability,
                     Destructibility>
    get_facet_primitive(
        pack_facet_primitive<Ss, Cs, Rs, MaxSize, MaxAlign, Copyability,
                             Relocatability, Destructibility>*);
template <class F>
using facet_primitive_t = decltype(get_facet_primitive(std::declval<F*>()));

template <class Primitive>
struct facet_traits_impl;
template <class Ss1, class Cs1, class Rs1, std::size_t MaxSize1,
          std::size_t MaxAlign1, constraint_level Copyability1,
          constraint_level Relocatability1, constraint_level Destructibility1>
struct facet_traits_impl<
    pack_facet_primitive<Ss1, Cs1, Rs1, MaxSize1, MaxAlign1, Copyability1,
                         Relocatability1, Destructibility1>> {
  template <class Ss, class Cs, class Rs, std::size_t MaxSize,
            std::size_t MaxAlign, constraint_level Copyability,
            constraint_level Relocatability, constraint_level Destructibility>
  using push_pack =
      pack_facet_primitive<composite_t<Ss, Ss1>, composite_t<Cs, Cs1>,
                           composite_t<Rs, Rs1>, merge_size(MaxSize, MaxSize1),
                           merge_size(MaxAlign, MaxAlign1),
                           merge_constraint(Copyability, Copyability1),
                           merge_constraint(Relocatability, Relocatability1),
                           merge_constraint(Destructibility, Destructibility1)>;
};
template <class F>
struct facet_traits_impl<super_facet_primitive<F>> {
  template <class Ss, class Cs, class Rs, std::size_t MaxSize,
            std::size_t MaxAlign, constraint_level Copyability,
            constraint_level Relocatability, constraint_level Destructibility>
  using push_pack = pack_facet_primitive<
      composite_t<Ss, F>, Cs, Rs, merge_size(MaxSize, F::max_size),
      merge_size(MaxAlign, F::max_align),
      merge_constraint(Copyability, F::copyability),
      merge_constraint(Relocatability, F::relocatability),
      merge_constraint(Destructibility, F::destructibility)>;
};
template <bool IsDirect, class D, class O>
struct facet_traits_impl<convention_facet_primitive<IsDirect, D, O>> {
  template <class Ss, class Cs, class Rs, std::size_t MaxSize,
            std::size_t MaxAlign, constraint_level Copyability,
            constraint_level Relocatability, constraint_level Destructibility>
  using push_pack =
      pack_facet_primitive<Ss, composite_t<Cs, conv_impl<IsDirect, D, O>>, Rs,
                           MaxSize, MaxAlign, Copyability, Relocatability,
                           Destructibility>;
};
template <bool IsDirect, class R>
struct facet_traits_impl<reflection_facet_primitive<IsDirect, R>> {
  template <class Ss, class Cs, class Rs, std::size_t MaxSize,
            std::size_t MaxAlign, constraint_level Copyability,
            constraint_level Relocatability, constraint_level Destructibility>
  using push_pack =
      pack_facet_primitive<Ss, Cs, composite_t<Rs, refl_impl<IsDirect, R>>,
                           MaxSize, MaxAlign, Copyability, Relocatability,
                           Destructibility>;
};
template <constraint_level CL>
struct facet_traits_impl<copyability_facet_primitive<CL>> {
  template <class Ss, class Cs, class Rs, std::size_t MaxSize,
            std::size_t MaxAlign, constraint_level Copyability,
            constraint_level Relocatability, constraint_level Destructibility>
  using push_pack = pack_facet_primitive<Ss, Cs, Rs, MaxSize, MaxAlign,
                                         merge_constraint(Copyability, CL),
                                         Relocatability, Destructibility>;
};
template <constraint_level CL>
struct facet_traits_impl<relocatability_facet_primitive<CL>> {
  template <class Ss, class Cs, class Rs, std::size_t MaxSize,
            std::size_t MaxAlign, constraint_level Copyability,
            constraint_level Relocatability, constraint_level Destructibility>
  using push_pack =
      pack_facet_primitive<Ss, Cs, Rs, MaxSize, MaxAlign, Copyability,
                           merge_constraint(Relocatability, CL),
                           Destructibility>;
};
template <constraint_level CL>
struct facet_traits_impl<destructibility_facet_primitive<CL>> {
  template <class Ss, class Cs, class Rs, std::size_t MaxSize,
            std::size_t MaxAlign, constraint_level Copyability,
            constraint_level Relocatability, constraint_level Destructibility>
  using push_pack = pack_facet_primitive<Ss, Cs, Rs, MaxSize, MaxAlign,
                                         Copyability, Relocatability,
                                         merge_constraint(Destructibility, CL)>;
};
template <std::size_t MaxSize1, std::size_t MaxAlign1>
struct facet_traits_impl<layout_facet_primitive<MaxSize1, MaxAlign1>> {
  template <class Ss, class Cs, class Rs, std::size_t MaxSize,
            std::size_t MaxAlign, constraint_level Copyability,
            constraint_level Relocatability, constraint_level Destructibility>
  using push_pack =
      pack_facet_primitive<Ss, Cs, Rs, merge_size(MaxSize, MaxSize1),
                           merge_size(MaxAlign, MaxAlign1), Copyability,
                           Relocatability, Destructibility>;
};
template <class F>
struct facet_traits : facet_traits_impl<facet_primitive_t<F>> {};

template <class O, class I>
struct facet_reduction;
template <class Ss, class Cs, class Rs, std::size_t MaxSize,
          std::size_t MaxAlign, constraint_level Copyability,
          constraint_level Relocatability, constraint_level Destructibility,
          class I>
struct facet_reduction<
    pack_facet_primitive<Ss, Cs, Rs, MaxSize, MaxAlign, Copyability,
                         Relocatability, Destructibility>,
    I>
    : std::type_identity<typename facet_traits<I>::template push_pack<
          Ss, Cs, Rs, MaxSize, MaxAlign, Copyability, Relocatability,
          Destructibility>> {};

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
concept facet = requires { typename detail::facet_primitive_t<F>; };

template <facet... Fs>
struct pack
    : detail::recursive_reduction_t<
          detail::reduction_t<detail::facet_reduction>,
          detail::pack_facet_primitive<std::tuple<>, std::tuple<>, std::tuple<>,
                                       detail::invalid_size,
                                       detail::invalid_size, detail::invalid_cl,
                                       detail::invalid_cl, detail::invalid_cl>,
          Fs...> {};

template <facade F>
struct super : detail::super_facet_primitive<F> {};

template <class D, detail::extended_overload... Os>
  requires(sizeof...(Os) > 0u)
struct indirect_convention : pack<indirect_convention<D, Os>...> {};
template <class D, detail::extended_overload O>
struct indirect_convention<D, O>
    : detail::convention_facet_primitive<false, D, O> {};
template <class D, detail::extended_overload... Os>
  requires(sizeof...(Os) > 0u)
struct direct_convention : pack<direct_convention<D, Os>...> {};
template <class D, detail::extended_overload O>
struct direct_convention<D, O>
    : detail::convention_facet_primitive<true, D, O> {};
template <class D, detail::extended_overload... Os>
  requires(sizeof...(Os) > 0u)
struct convention : indirect_convention<D, Os...> {};

template <detail::basic_meta R>
struct indirect_reflection : detail::reflection_facet_primitive<false, R> {};
template <detail::basic_meta R>
struct direct_reflection : detail::reflection_facet_primitive<true, R> {};
template <detail::basic_meta R>
struct reflection : indirect_reflection<R> {};

template <constraint_level CL>
  requires(detail::is_cl_well_formed(CL))
struct copyability : detail::copyability_facet_primitive<CL> {};
template <constraint_level CL>
  requires(detail::is_cl_well_formed(CL))
struct relocatability : detail::relocatability_facet_primitive<CL> {};
template <constraint_level CL>
  requires(detail::is_cl_well_formed(CL))
struct destructibility : detail::destructibility_facet_primitive<CL> {};

template <std::size_t PtrSize,
          std::size_t PtrAlign = detail::max_align_of(PtrSize)>
  requires(detail::is_layout_well_formed(PtrSize, PtrAlign))
struct layout : detail::layout_facet_primitive<PtrSize, PtrAlign> {};

} // namespace facets

} // namespace pro::inline v4

#endif // MSFT_PROXY_V4_DETAIL_FACADE_CREATION_H_
