// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V4_DETAIL_PAC_H_
#define MSFT_PROXY_V4_DETAIL_PAC_H_

// Pointer authentication (PAC) support for `proxy`'s dispatch metadata.
//
// On Apple silicon the arm64e C++ ABI signs both the v-table pointer of a
// polymorphic object and every virtual function pointer inside the v-table
// using ARMv8.3 pointer authentication, with *address diversity* (the
// signature depends on where the pointer is stored) and *type diversity* (a
// constant discriminator derived from the type). `proxy`'s dispatch metadata
// is a hand-rolled v-table, so on such targets it must be signed similarly to
// be no less secure than a real virtual function.
//
// On every other major platform this is unnecessary: notably, on Linux/AArch64
// (GCC and Clang) the only PAC-based hardening that ships is `pac-ret` (return
// address signing), which does not change the C++ ABI -- v-tables there are
// *not* signed -- so unsigned dispatch metadata is already as secure as a
// virtual call. PAC is therefore enabled exactly when the toolchain signs code
// pointers in the ABI (Clang's `ptrauth_calls`) and exposes the `__ptrauth`
// qualifier (`ptrauth_qualifier`). `PRO4D_PAC` may be predefined to `0` or `1`
// to override this detection.
#ifndef PRO4D_PAC
#if defined(__has_feature)
#if __has_feature(ptrauth_qualifier) && __has_feature(ptrauth_calls)
#define PRO4D_PAC 1
#endif // __has_feature(ptrauth_qualifier) && __has_feature(ptrauth_calls)
#endif // defined(__has_feature)
#ifndef PRO4D_PAC
#define PRO4D_PAC 0
#endif // PRO4D_PAC
#endif // PRO4D_PAC

#if PRO4D_PAC
#include <cstddef> // std::nullptr_t
#include <ptrauth.h>
// clang's `__ptrauth` *qualifier* cannot be used: it requires the discriminator
// to be constant-evaluable even within templates, so a per-convention (hence
// template-dependent) type discriminator is rejected. We therefore sign by hand
// with the runtime ptrauth intrinsics -- which DO accept a dependent
// `ptrauth_type_discriminator` -- via `signed_fn_ptr` / `signed_data_ptr`.
//
// Consequence: manual signing is not constant-evaluable, so PAC builds give up
// `constexpr` metadata -- the signed pointers re-sign on copy (so a proxy is no
// longer bitwise-relocatable) and the out-of-line v-table is an `inline`
// variable signed once at static-initialization time (not a constexpr object,
// but also not a guarded function-local static). These macros select the signed
// member type and the call/deref expression, and drop `constexpr` where signing
// makes it impossible. `proxy.h` uses them and `#undef`s them after use.
#define PRO4D_PAC_FN_MEMBER(fp, disc)                                          \
  ::pro::v4::details::signed_fn_ptr<fp, disc>
#define PRO4D_PAC_VPTR_MEMBER(meta)                                            \
  ::pro::v4::details::signed_data_ptr<meta, void (*)(meta*)>
#define PRO4D_PAC_FN_CALL(f) (f).get()
#define PRO4D_PAC_CONSTEXPR
#else
#define PRO4D_PAC_FN_MEMBER(fp, disc) fp
#define PRO4D_PAC_VPTR_MEMBER(meta) const meta*
#define PRO4D_PAC_FN_CALL(f) (f)
#define PRO4D_PAC_CONSTEXPR constexpr
#endif // PRO4D_PAC

#if PRO4D_PAC
namespace pro::inline v4::details {

// A non-zero 16-bit discriminator unique to the type `T` (type diversity). This
// is what an arm64e v-table uses (`ptrauth_string_discriminator` of the mangled
// function name); we cannot reach that builtin because it needs a string
// *literal*, and `ptrauth_type_discriminator` is useless here -- the default
// arm64e ABI disables function-pointer type discrimination, so it returns the
// same constant for every function type. Instead FNV-hash the spelling of `T`
// into the discriminator range, the way magic_enum/nameof derive type identity.
// The result is consumed by the runtime ptrauth intrinsics, which (unlike the
// `__ptrauth` qualifier) accept a value-dependent discriminator.
//
// Clang spells the instantiation as "...pac_type_disc() [T = <type>]"; hash only
// the "<type>" payload so the discriminator depends on `T` alone, not on this
// helper's own signature/namespace. If the markers are absent (an unexpected
// format) the parsing is broken, which would silently weaken type diversity, so
// fail the build hard (a `throw` in this `consteval` function is reached only
// during constant evaluation -> a compile error). The exact discriminator
// values are pinned by ABI-stability tests in proxy_pac_tests.cpp.
template <class T>
consteval ptrauth_extra_data_t pac_type_disc() {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic" // __PRETTY_FUNCTION__
#endif
  const char* const sig = __PRETTY_FUNCTION__;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
  const char* begin = sig;
  const char* end = sig;
  for (const char* p = sig; *p != '\0'; ++p) {
    if (p[0] == '=' && p[1] == ' ') {
      begin = p + 2; // start of "<type>" (last "= ")
    }
    if (*p == ']') {
      end = p; // last ']' closes "[T = <type>]"
    }
  }
  if (begin >= end) {
    throw "pac_type_disc: could not extract the type name from "
          "__PRETTY_FUNCTION__; the compiler's format is unexpected";
  }
  unsigned long long hash = 14695981039346656037ull; // FNV-1a, 64-bit
  for (const char* p = begin; p < end; ++p) {
    hash = (hash ^ static_cast<unsigned char>(*p)) * 1099511628211ull;
  }
  // Map into [1, 65535]: a 16-bit discriminator of 0 means "no discriminator"
  // (no type diversity) in the ptrauth ABI, so it must be non-zero -- the same
  // range and mapping `ptrauth_string_discriminator` itself uses.
  return static_cast<ptrauth_extra_data_t>(hash % 65535u) + 1u;
}

// A function pointer signed like an arm64e virtual-function slot: IA key, with
// address diversity (the storage address is blended into the discriminator) and
// type diversity (`pac_type_disc<Disc>()`, where `Disc` is a function type
// encoding the convention). The stored value uses a per-storage schema and is
// re-signed on copy for the destination address. `value_` must only be reached
// through `get()`, which re-signs it to the standard schema so it is callable.
//
// The empty state is a *signed* null: a null pointer signed with the slot's own
// schema, exactly as a real entry is. This keeps every hot path branchless --
// copy/assignment and `get()` are each a single unconditional authenticate-and-
// resign, with no null special-case. An empty slot round-trips through that same
// resign (authenticating a slot-signed null succeeds and re-signs it for the
// destination), so copying an empty meta neither branches nor traps. The only
// place that distinguishes empty is `operator==(nullptr)` (hence `has_value()`),
// which strips the signature and compares the raw pointer -- still branchless.
// Storing a *raw* null instead would force the copy to branch to avoid
// authenticating it; signing the null is what removes the branch.
//
// The default constructor must establish the signed null (it cannot be
// `= default`): every slot must always hold an auth-able value, because copying
// an empty proxy memberwise authenticate-and-resigns *all* of its slots, and the
// non-sentinel slots of an empty proxy are initialized only here -- `reset()`
// flips just the single `has_value` sentinel. A trivial default constructor
// leaves the others as garbage and copying an empty proxy then traps (the
// `invoker` raw-pointer analogy does not hold for a signed pointer).
template <class FP, class Disc>
class signed_fn_ptr {
public:
  signed_fn_ptr() noexcept
      : value_(ptrauth_sign_unauthenticated(
            static_cast<FP>(nullptr), ptrauth_key_function_pointer,
            schema(&value_))) {}
  signed_fn_ptr(FP fp) noexcept
      : value_(ptrauth_auth_and_resign(
            fp, ptrauth_key_function_pointer,
            ptrauth_function_pointer_type_discriminator(FP),
            ptrauth_key_function_pointer, schema(&value_))) {}
  signed_fn_ptr(const signed_fn_ptr& rhs) noexcept
      : value_(ptrauth_auth_and_resign(
            rhs.value_, ptrauth_key_function_pointer, schema(&rhs.value_),
            ptrauth_key_function_pointer, schema(&value_))) {}
  signed_fn_ptr& operator=(const signed_fn_ptr& rhs) noexcept {
    value_ = ptrauth_auth_and_resign(
        rhs.value_, ptrauth_key_function_pointer, schema(&rhs.value_),
        ptrauth_key_function_pointer, schema(&value_));
    return *this;
  }
  signed_fn_ptr& operator=(std::nullptr_t) noexcept {
    value_ = ptrauth_sign_unauthenticated(
        static_cast<FP>(nullptr), ptrauth_key_function_pointer, schema(&value_));
    return *this;
  }
  FP get() const noexcept {
    return ptrauth_auth_and_resign(
        value_, ptrauth_key_function_pointer, schema(&value_),
        ptrauth_key_function_pointer,
        ptrauth_function_pointer_type_discriminator(FP));
  }
  friend bool operator==(const signed_fn_ptr& self, std::nullptr_t) noexcept {
    return ptrauth_strip(self.value_, ptrauth_key_function_pointer) == nullptr;
  }

private:
  static ptrauth_extra_data_t schema(const void* addr) noexcept {
    return ptrauth_blend_discriminator(addr, pac_type_disc<Disc>());
  }
  FP value_;
};

// A data pointer signed like an arm64e v-table pointer: DA key, address
// diversity, and type diversity (a constant derived from `Disc`). Used for the
// v-table pointer in the out-of-line `meta_storage`. The empty state is a signed
// null exactly as in `signed_fn_ptr` (and the default constructor likewise signs
// it, for the same reason), so copy/assignment and dereference are each a single
// unconditional sign/authenticate with no null branch; an empty meta round-trips
// through the resigning copy. `operator==` (and thus `meta_storage::has_value()`)
// strips the signature to recognize the null, branchlessly.
template <class T, class Disc>
class signed_data_ptr {
public:
  signed_data_ptr() noexcept
      : value_(ptrauth_sign_unauthenticated(
            static_cast<const T*>(nullptr), ptrauth_key_cxx_vtable_pointer,
            schema(&value_))) {}
  signed_data_ptr(const T* p) noexcept
      : value_(ptrauth_sign_unauthenticated(p, ptrauth_key_cxx_vtable_pointer,
                                            schema(&value_))) {}
  signed_data_ptr(const signed_data_ptr& rhs) noexcept
      : value_(ptrauth_auth_and_resign(
            rhs.value_, ptrauth_key_cxx_vtable_pointer, schema(&rhs.value_),
            ptrauth_key_cxx_vtable_pointer, schema(&value_))) {}
  signed_data_ptr& operator=(const signed_data_ptr& rhs) noexcept {
    value_ = ptrauth_auth_and_resign(
        rhs.value_, ptrauth_key_cxx_vtable_pointer, schema(&rhs.value_),
        ptrauth_key_cxx_vtable_pointer, schema(&value_));
    return *this;
  }
  signed_data_ptr& operator=(std::nullptr_t) noexcept {
    value_ = ptrauth_sign_unauthenticated(
        static_cast<const T*>(nullptr), ptrauth_key_cxx_vtable_pointer,
        schema(&value_));
    return *this;
  }
  const T& operator*() const noexcept {
    return *ptrauth_auth_data(value_, ptrauth_key_cxx_vtable_pointer,
                              schema(&value_));
  }
  friend bool operator==(const signed_data_ptr& self,
                         std::nullptr_t) noexcept {
    return ptrauth_strip(self.value_, ptrauth_key_cxx_vtable_pointer) ==
           nullptr;
  }

private:
  static ptrauth_extra_data_t schema(const void* addr) noexcept {
    return ptrauth_blend_discriminator(addr, pac_type_disc<Disc>());
  }
  const T* value_;
};

} // namespace pro::inline v4::details
#endif // PRO4D_PAC

#endif // MSFT_PROXY_V4_DETAIL_PAC_H_
