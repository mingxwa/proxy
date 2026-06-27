// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

// Subjects for the debugger-visualizer regression tests. Build with debug info
// (`-g`); a debugger breaks on `proxy_visualizer_break()` with every proxy live
// and asserts that the visualizer recovers the contained pointer type and can
// call through the stored pointer. Kept free of compiler-specific extensions so
// the same file can also feed the MSVC/natvis test.

#include <memory>
#include <string>

#include <proxy/proxy.h>

struct Cat {
  std::string name;
  int legs = 4;
  std::string Speak() const { return name + " says meow"; }
};

PRO4_DEF_MEM_DISPATCH(MemSpeak, Speak);

struct Animal : pro::facade_builder                             //
                ::add_convention<MemSpeak, std::string() const> //
                ::build {};
struct AnimalDirectRtti
    : pro::facade_builder //
      ::add_convention<MemSpeak, std::string() const>::add_skill<
          pro::skills::direct_rtti> //
      ::build {};
struct AnimalRtti
    : pro::facade_builder //
      ::add_convention<MemSpeak,
                       std::string() const>::add_skill<pro::skills::rtti> //
      ::build {};
struct AnimalBothRtti
    : pro::facade_builder //
      ::add_convention<MemSpeak, std::string() const>::add_skill<
          pro::skills::rtti>::add_skill<pro::skills::direct_rtti> //
      ::build {};

// Defined out of line and never inlined so the debugger has a stable
// breakpoint.
void proxy_visualizer_break() {}

// Portable "don't optimize this away" sink (no inline asm, for MSVC parity).
template <class T>
void keep_alive(const T& value) {
  static const volatile void* sink;
  sink = &value;
}

int main() {
  static Cat felix{"Felix", 4};

  pro::proxy<Animal> p_raw = &felix;
  pro::proxy<Animal> p_unique = std::make_unique<Cat>(Cat{"Tom", 4});
  pro::proxy<Animal> p_shared = std::make_shared<Cat>(Cat{"Sasha", 4});
  pro::proxy<Animal> p_made = pro::make_proxy<Animal>(Cat{"Milo", 4});
  pro::proxy<Animal> p_empty;
  pro::proxy<AnimalDirectRtti> p_direct_rtti =
      std::make_unique<Cat>(Cat{"Direct", 4});
  pro::proxy<AnimalRtti> p_rtti = std::make_unique<Cat>(Cat{"Indirect", 4});
  pro::proxy<AnimalBothRtti> p_both = std::make_unique<Cat>(Cat{"Both", 4});

  keep_alive(p_raw);
  keep_alive(p_unique);
  keep_alive(p_shared);
  keep_alive(p_made);
  keep_alive(p_empty);
  keep_alive(p_direct_rtti);
  keep_alive(p_rtti);
  keep_alive(p_both);

  proxy_visualizer_break();
  return 0;
}
