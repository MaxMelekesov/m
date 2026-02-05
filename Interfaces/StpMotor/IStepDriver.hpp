/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ISTEP_DRIVER_CTRL_HPP
#define ISTEP_DRIVER_CTRL_HPP

#include <MilliAmpere.hpp>
#include <cstdint>

namespace m::ifc {

template <CmA mAT>
class IStepDriver {
 public:
  enum class Microstep : uint8_t {
    M_1 = 0,
    M_2,
    M_4,
    M_8,
    M_16,
    M_32,
    M_64,
    M_128,
    M_256
  };

  enum class Dir : uint8_t { Forward = 0, Backward };
  enum class DirInversion : uint8_t { No = 0, Yes };

  virtual ~IStepDriver() {}

  virtual bool setEnable(bool value) = 0;
  virtual bool getEnable() = 0;

  virtual bool setSleep(bool value) = 0;
  virtual bool getSleep() = 0;

  virtual bool setReset(bool value) = 0;
  virtual bool getReset() = 0;

  virtual bool setDirection(Dir dir) = 0;
  virtual Dir getDirection() = 0;

  virtual bool setDirectionInversion(DirInversion inv) = 0;
  virtual DirInversion getDirectionInversion() = 0;

  virtual bool setMicrostep(Microstep m) = 0;
  virtual Microstep getMicrostep() = 0;

  virtual bool setCurrent(mAT ma) = 0;
  virtual mAT getCurrent() = 0;
};

template <typename T>
concept CStepDriver =
    requires(T ctrl, bool b, typename T::Dir dir, typename T::DirInversion inv,
             typename T::Microstep ms) {
      { ctrl.setEnable(b) } -> std::same_as<bool>;
      { ctrl.getEnable() } -> std::same_as<bool>;
      { ctrl.setSleep(b) } -> std::same_as<bool>;
      { ctrl.getSleep() } -> std::same_as<bool>;
      { ctrl.setReset(b) } -> std::same_as<bool>;
      { ctrl.getReset() } -> std::same_as<bool>;
      { ctrl.setDirection(dir) } -> std::same_as<bool>;
      { ctrl.getDirection() } -> std::same_as<typename T::Dir>;
      { ctrl.setDirectionInversion(inv) } -> std::same_as<bool>;
      {
        ctrl.getDirectionInversion()
      } -> std::same_as<typename T::DirInversion>;
      { ctrl.setMicrostep(ms) } -> std::same_as<bool>;
      { ctrl.getMicrostep() } -> std::same_as<typename T::Microstep>;
      requires CmA<std::remove_cvref_t<decltype(ctrl.getCurrent())>>;
      requires requires(
          typename std::remove_cvref_t<decltype(ctrl.getCurrent())> ma) {
        { ctrl.setCurrent(ma) } -> std::same_as<bool>;
      };
    };

static_assert(CStepDriver<IStepDriver<mA<uint32_t>>>,
              "IStepDriver must satisfy CStepDriver concept");

}  // namespace m::ifc

#endif  // ISTEP_DRIVER_CTRL_HPP
