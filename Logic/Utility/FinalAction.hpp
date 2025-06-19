/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FINAL_ACT_HPP
#define FINAL_ACT_HPP

#include <concepts>
#include <type_traits>
#include <utility>

/*
Example usage of `finally`:

#include <iostream>
#include "FinalAction.hpp"

void example() {
    int* ptr = new int(42);
    auto cleanup = m::finally([&]{
        delete ptr;
        std::cout << "Resource released\n";
    });

    // ... use ptr ...
    std::cout << *ptr << std::endl;

    // When leaving the scope, cleanup will run and release ptr
}
*/

namespace m {

template <std::invocable F>
class FinalAction {
 public:
  FinalAction(const FinalAction&) = delete;
  FinalAction& operator=(const FinalAction&) = delete;
  FinalAction& operator=(FinalAction&&) = delete;
  constexpr FinalAction(FinalAction&& other) = delete;

  constexpr explicit FinalAction(F&& f) : f_(std::move(f)) {}

  constexpr ~FinalAction() { f_(); }

 private:
  F f_;
};

template <std::invocable F>
[[nodiscard]]
constexpr auto finally(F&& f) {
  return FinalAction<std::remove_cvref_t<F>>(std::forward<F>(f));
}

}  // namespace m
#endif