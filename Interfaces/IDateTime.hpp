/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IDATETIME_HPP
#define IDATETIME_HPP

#include <concepts>
#include <cstdint>

namespace m::ifc {
class IDateTime {
 public:
  virtual ~IDateTime() {}

  enum class Month {
    January = 1,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December
  };
  enum class Weekday {
    Sunday = 1,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday
  };

  struct Date {
    uint16_t year = 2'000;
    Month month = Month::January;
    uint8_t day = 1;
    Weekday weekday = Weekday::Monday;
  };
  struct Time {
    uint8_t hours = 0;
    uint8_t minutes = 0;
    uint8_t seconds = 0;
  };

  virtual Date getDate() = 0;
  virtual bool setDate(Date& date) = 0;

  virtual Time getTime() = 0;
  virtual bool setTime(Time& time) = 0;
};

template <typename T>
concept CDateTime =
    requires(T dt, typename T::Date date, typename T::Time time) {
      typename T::Date;
      typename T::Time;

      { dt.getDate() } -> std::same_as<typename T::Date>;
      { dt.setDate(date) } -> std::same_as<bool>;

      { dt.getTime() } -> std::same_as<typename T::Time>;
      { dt.setTime(time) } -> std::same_as<bool>;
    };

static_assert(CDateTime<IDateTime>, "IDateTime must satisfy CDateTime concept");

}  // namespace m::ifc

#endif  // IDATETIME_HPP
