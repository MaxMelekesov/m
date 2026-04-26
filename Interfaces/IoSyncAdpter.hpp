/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IIOSYNCADAPTER_HPP
#define IIOSYNCADAPTER_HPP

#include <Bps.hpp>
#include <IIO_Async.hpp>
#include <IIO_Sync.hpp>
#include <ITime.hpp>
#include <Timeout.hpp>
#include <functional>

namespace m::ifc {

template <CBps Baudrate, typename TimeUnitT, m::ifc::CTime<TimeUnitT> TimeT>
class [[deprecated("remove")]] IoSyncAdapter : public IIO_Sync<Baudrate> {
 public:
  IoSyncAdapter(
      IIO_Async<Baudrate>& io, TimeT& time,
      std::function<TimeUnitT(Baudrate baud, std::size_t size)>&& calc_timeout)
      : io_(io), time_(time), calc_timeout_(std::move(calc_timeout)) {}

  bool write(std::span<uint8_t const> data) override {
    if (!io_.startWrite(data)) return false;
    return m::execWithTimeout(
        time_, [&]() { return io_.isWriteDone(); },
        calc_timeout_(io_.getBaudrate(), data.size()));
  }

  bool read(std::span<uint8_t> data) override {
    if (!io_.startRead(data)) return false;
    return m::execWithTimeout(
        time_, [&]() { return io_.isReadDone(); },
        calc_timeout_(io_.getBaudrate(), data.size()));
  }

  Baudrate getBaudrate() override { return io_.getBaudrate(); }
  bool setBaudrate(Baudrate baud) override { return io_.setBaudrate(baud); }

  bool error() override { return io_.error(); }

 private:
  IIO_Async<Baudrate>& io_;
  TimeT& time_;
  std::function<TimeUnitT(Baudrate baud, std::size_t size)> calc_timeout_;
};

static_assert(
    CIO_Sync<IoSyncAdapter<Bps<uint32_t>, uint32_t, m::ifc::ITime<uint32_t>>>,
    "IIO_Sync must satisfy CIO_Sync concept");

}  // namespace m::ifc

#endif  // IIOSYNCADAPTER_HPP