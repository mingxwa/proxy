// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V4_PROXY_FMT_H_
#define MSFT_PROXY_V4_PROXY_FMT_H_

#include <string_view>
#include <type_traits>

#ifndef __msft_lib_proxy4
#error Please ensure that proxy.h is included before proxy_fmt.h.
#endif // __msft_lib_proxy4

#if FMT_VERSION < 60100
#error Please ensure that the appropriate {fmt} headers (version 6.1.0 or \
later) are included before proxy_fmt.h.
#endif // FMT_VERSION < 60100

namespace pro::inline v4 {

namespace details {

template <class CharT>
#if FMT_VERSION >= 110000
using fmt_buffered_context = fmt::buffered_context<CharT>;
#else
using fmt_buffered_context = fmt::buffer_context<CharT>;
#endif // FMT_VERSION

struct fmt_format_dispatch {
  template <class T, class CharT>
  PRO4D_STATIC_CALL(auto, const T& self, std::basic_string_view<CharT> spec,
                    fmt_buffered_context<CharT>& fc)
    requires(std::is_default_constructible_v<fmt::formatter<T, CharT>>)
  {
    fmt::formatter<T, CharT> impl;
    {
      fmt::basic_format_parse_context<CharT> pc{spec};
      impl.parse(pc);
    }
    return impl.format(self, fc);
  }
};

template <class CharT>
struct fmt_format_traits
    : format_traits<fmt_format_dispatch, std::basic_string_view<CharT>,
                    fmt::basic_format_parse_context<CharT>,
                    fmt_buffered_context<CharT>> {};

} // namespace details

namespace skills {

template <class FB>
using fmt_format = typename FB::template add_convention<
    details::fmt_format_dispatch, details::fmt_format_traits<char>::overload>;

template <class FB>
using fmt_wformat = typename FB::template add_convention<
    details::fmt_format_dispatch,
    details::fmt_format_traits<wchar_t>::overload>;

} // namespace skills

} // namespace pro::inline v4

namespace fmt {

template <pro::v4::facade F, class CharT>
  requires(pro::v4::details::fmt_format_traits<CharT>::template applicable<F>)
struct formatter<pro::v4::proxy_indirect_accessor<F>, CharT>
    : pro::v4::details::fmt_format_traits<CharT>::template formatter<F> {};

} // namespace fmt

#endif // MSFT_PROXY_V4_PROXY_FMT_H_
