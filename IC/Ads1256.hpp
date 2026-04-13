/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef ADS1256_HPP
#define ADS1256_HPP

#include <IIO_Async.hpp>
#include <IIt.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Ic.hpp>
#include <Reg.hpp>
#include <StaticMap.hpp>
#include <Timeout.hpp>
#include <Us.hpp>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace m::ic {

struct Ads1256 {
  struct Data {
    struct Value : public m::BitField<Value, 24> {};

    m::Reg<uint32_t, Value, m::UnusedField<8>> value;
  };

  struct DataC {
    struct Value : public m::BitField<Value, 24> {};

    m::Reg<uint32_t, Value, m::UnusedField<8>> value;
  };

  struct RDataC {
    m::Reg<uint8_t, m::UnusedField<8>> value;
  };

  struct SDataC {
    m::Reg<uint8_t, m::UnusedField<8>> value;
  };

  struct ReadStatus {
    struct Drdy : public m::BitField<Drdy, 1> {};
    struct Bufen : public m::BitField<Bufen, 1> {
      enum : uint8_t { Off = 0, On };
    };
    struct Acal : public m::BitField<Acal, 1> {
      enum : uint8_t { Disable = 0, Enable };
    };
    struct Order : public m::BitField<Order, 1> {
      enum : uint8_t { Msb = 0, Lsb };
    };
    struct Id : public m::BitField<Id, 4> {};

    m::Reg<uint8_t, Drdy, Bufen, Acal, Order, Id> value{0x01};
  };
  struct WriteStatus : public ReadStatus {};

  struct ReadMux {
    struct Nsel : public m::BitField<Nsel, 4> {
      enum : uint8_t {
        Ain_0 = 0,
        Ain_1 = 1,
        Ain_2 = 2,
        Ain_3 = 3,
        Ain_4 = 4,
        Ain_5 = 5,
        Ain_6 = 6,
        Ain_7 = 7,
        Acom = 8
      };
    };
    struct Psel : public m::BitField<Psel, 4> {
      enum : uint8_t {
        Ain_0 = 0,
        Ain_1 = 1,
        Ain_2 = 2,
        Ain_3 = 3,
        Ain_4 = 4,
        Ain_5 = 5,
        Ain_6 = 6,
        Ain_7 = 7,
        Acom = 8
      };
    };

    m::Reg<uint8_t, Nsel, Psel> value{0x01};
  };
  struct WriteMux : public ReadMux {};

  struct ReadAdcon {
    struct Pga : public m::BitField<Pga, 3> {
      enum : uint8_t {
        G_1 = 0,
        G_2 = 1,
        G_4 = 2,
        G_8 = 3,
        G_16 = 4,
        G_32 = 5,
        G_64 = 6
      };
    };
    struct Sdcs : public m::BitField<Sdcs, 2> {
      enum : uint8_t { Off = 0, I_0_5_uA = 1, I_2_uA = 2, I_10_uA = 3 };
    };
    struct Clk : public m::BitField<Clk, 2> {
      enum : uint8_t {
        Off = 0,
        Full_Freq = 1,
        Half_Freq = 2,
        Quarter_Freq = 3
      };
    };

    m::Reg<uint8_t, Pga, Sdcs, Clk, m::UnusedField<1>> value{0x20};
  };
  struct WriteAdcon : public ReadAdcon {};

  struct ReadDrate {
    struct Dr : public m::BitField<Dr, 8> {
      enum : uint8_t {
        R_30000 = 0b1111'0000,
        R_15000 = 0b1110'0000,
        R_7500 = 0b1101'0000,
        R_3750 = 0b1100'0000,
        R_2000 = 0b1011'0000,
        R_1000 = 0b1010'0001,
        R_500 = 0b1001'0010,
        R_100 = 0b1000'0010,
        R_60 = 0b0111'0010,
        R_50 = 0b0110'0011,
        R_30 = 0b0101'0011,
        R_25 = 0b0100'0011,
        R_15 = 0b0011'0011,
        R_10 = 0b0010'0011,
        R_5 = 0b0001'0011,
        R_2_5 = 0b0000'0011
      };
    };

    m::Reg<uint8_t, Dr> value{0xF0};
  };
  struct WriteDrate : public ReadDrate {};

  struct Selfcal {
    m::Reg<uint8_t, m::UnusedField<8>> value;
  };

  using Regs = std::tuple<Data, DataC, RDataC, SDataC, ReadStatus, ReadMux,
                          ReadAdcon, ReadDrate, WriteStatus, WriteMux,
                          WriteAdcon, WriteDrate, Selfcal>;

  using Map = m::StaticMap<uint16_t, m::Pair<Data, 0x01>, m::Pair<DataC, 0x01>,
                           m::Pair<RDataC, 0x03>, m::Pair<SDataC, 0x0F>,
                           m::Pair<ReadStatus, 0x10>, m::Pair<ReadMux, 0x11>,
                           m::Pair<ReadAdcon, 0x12>, m::Pair<ReadDrate, 0x13>,
                           m::Pair<WriteStatus, 0x50>, m::Pair<WriteMux, 0x51>,
                           m::Pair<WriteAdcon, 0x52>, m::Pair<WriteDrate, 0x53>,
                           m::Pair<Selfcal, 0xF0>>;
};

template <m::ifc::CUs Us, m::ifc::CTimeUs Time, m::ifc::CIO_Async Io>
class Ads1256Ic : public Ic<Ads1256Ic<Us, Time, Io>, Ads1256> {
 public:
  Ads1256Ic(Time& time, Io& io, m::ifc::mcu::IPin& cs, Us add_timeout)
      : time_(time), io_(io), cs_(cs), add_timeout_(add_timeout) {}

 private:
  Time& time_;
  Io& io_;
  m::ifc::mcu::IPin& cs_;
  Us add_timeout_;

  std::array<uint8_t, 3> read_buf_;
  std::array<uint8_t, 3> write_buf_;

  template <typename Reg>
    requires std::is_same_v<Reg, Ads1256::WriteStatus> ||
             std::is_same_v<Reg, Ads1256::WriteMux> ||
             std::is_same_v<Reg, Ads1256::WriteAdcon> ||
             std::is_same_v<Reg, Ads1256::WriteDrate>
  bool writeImpl(Reg reg) {
    write_buf_[0] = Ads1256::Map::template value<Reg>();
    write_buf_[1] = 0;  // register count -1
    write_buf_[2] = static_cast<uint8_t>(reg.value.getRaw());

    auto write_span = std::span<const uint8_t>(write_buf_).first(3);
    cs_.write(1);
    bool res = writeSpan(write_span);
    cs_.write(0);

    return res;
  }

  template <typename Reg>
    requires std::is_same_v<Reg, Ads1256::ReadStatus> ||
             std::is_same_v<Reg, Ads1256::ReadMux> ||
             std::is_same_v<Reg, Ads1256::ReadAdcon> ||
             std::is_same_v<Reg, Ads1256::ReadDrate>
  std::optional<Reg> readImpl() {
    write_buf_[0] = Ads1256::Map::template value<Reg>();
    write_buf_[1] = 0;  // register count -1

    auto write_span = std::span<const uint8_t>(write_buf_).first(2);
    cs_.write(1);
    if (!writeSpan(write_span)) {
      cs_.write(0);
      return std::nullopt;
    }

    time_.delay(Us{7});  // t6 datasheet

    read_buf_[0] = 0;
    auto read_span = std::span<uint8_t>(read_buf_).first(1);
    if (!readSpan(read_span)) {
      cs_.write(0);
      return std::nullopt;
    }
    cs_.write(0);

    Reg reg{read_buf_[0]};
    return reg;
  }

  template <typename Reg>
    requires std::is_same_v<Reg, Ads1256::Data>
  std::optional<Reg> readImpl() {
    write_buf_[0] = Ads1256::Map::value<Ads1256::Data>();

    auto write_span = std::span<const uint8_t>(write_buf_).first(1);
    cs_.write(1);
    if (!writeSpan(write_span)) {
      cs_.write(0);
      return std::nullopt;
    }

    time_.delay(Us{7});  // t6 datasheet

    read_buf_.fill(0);
    auto read_span = std::span<uint8_t>(read_buf_).first(3);
    if (!readSpan(read_span)) {
      cs_.write(0);
      return std::nullopt;
    }
    cs_.write(0);
    uint32_t raw = (static_cast<uint32_t>(read_buf_[0]) << 16) |
                   (static_cast<uint32_t>(read_buf_[1]) << 8) |
                   (static_cast<uint32_t>(read_buf_[2]));

    Ads1256::Data reg{raw};
    return reg;
  }

  template <typename Reg>
    requires std::is_same_v<Reg, Ads1256::DataC>
  std::optional<Reg> readImpl() {
    read_buf_.fill(0);

    cs_.write(1);
    auto read_span = std::span<uint8_t>(read_buf_).first(3);
    if (!readSpan(read_span)) {
      cs_.write(0);
      return std::nullopt;
    }
    cs_.write(0);
    uint32_t raw = (static_cast<uint32_t>(read_buf_[0]) << 16) |
                   (static_cast<uint32_t>(read_buf_[1]) << 8) |
                   (static_cast<uint32_t>(read_buf_[2]));

    Ads1256::DataC reg{raw};
    return reg;
  }

  template <typename Reg>
  bool writeImpl(Reg reg) {
    write_buf_[0] = Ads1256::Map::template value<Reg>();

    auto write_span = std::span<const uint8_t>(write_buf_).first(1);
    cs_.write(1);
    bool res = writeSpan(write_span);
    cs_.write(0);
    return res;
  }

  bool writeSpan(std::span<const uint8_t> span) {
    if (!io_.writeAsync(span)) return false;

    if (!m::execWithTimeout(
            time_, [&]() { return io_.writeDone(); },
            span.size() * Us{1'000} / io_.getBaudrate().value() +
                add_timeout_)) {
      return false;
    }

    return true;
  }

  bool readSpan(std::span<uint8_t> span) {
    if (!io_.readAsync(read_buf_)) return false;

    if (!m::execWithTimeout(
            time_, [&]() { return io_.readDone(); },
            span.size() * Us{1'000} / io_.getBaudrate().value() +
                add_timeout_)) {
      return false;
    }

    return true;
  }

  friend class Ic<Ads1256Ic<Us, Time, Io>, Ads1256>;
};

template <m::ifc::CUs Us, m::ifc::CTimeUs Time, m::ifc::CIO_Async Io>
Ads1256Ic(Time& time, Io& io, m::ifc::mcu::IPin& cs, Us add_timeout)
    -> Ads1256Ic<Us, Time, Io>;

template <m::ifc::CUs Us, m::ifc::CTimeUs Time, m::ifc::CIO_Async Io,
          m::ifc::mcu::CIt It>
class Ads1256Reader {
 public:
  Ads1256Reader(Time& time, Io& io, m::ifc::mcu::IPin& cs, It& drdy)
      : time_(time), io_(io), cs_(cs), drdy_(drdy) {
    drdy_.setCallback([&]() {
      if (start_flag_) {
        if (adc_ic_.write(Ads1256::RDataC{})) {
          start_flag_ = false;
        }
        return;
      } else if (stop_flag_) {
        if (adc_ic_.write(Ads1256::SDataC{})) {
          drdy_.stop();
          stop_flag_ = false;
        }
        return;
      } else {
        if (auto value = adc_ic_.template read<Ads1256::DataC>(); value) {
          auto reg = value.value();
          uint32_t reg_raw = reg.value.getRaw();
          if (reg_raw & 0x80'00'00) {
            reg_raw |= 0xFF'00'00'00;
          }
          if (data_.size()) {
            data_[0] = static_cast<int32_t>(reg_raw);
            last_value_ = data_[0];
            data_ = data_.subspan(1);
          } else {
            stopRead();
          }
        }
      }
    });
  }

  bool startRead(std::span<int32_t> data) {
    if (!readDone()) return false;

    data_ = data;
    size_ = data_.size();
    start_flag_ = true;
    if (!drdy_.start()) return false;
    return true;
  }

  bool readDone() { return !drdy_.running(); }

  std::size_t readed() { return size_ - data_.size(); }

  bool stopRead() {
    stop_flag_ = true;
    return true;
  }

  int32_t lastValue() { return last_value_; }

 private:
  Time& time_;
  Io& io_;
  m::ifc::mcu::IPin& cs_;
  It& drdy_;

  Ads1256Ic<Us, Time, Io> adc_ic_{time_, io_, cs_, Us{20}};

  std::span<int32_t> data_;
  std::size_t size_;

  bool start_flag_ = false;
  bool stop_flag_ = false;

  int32_t last_value_ = 0;
};

template <m::ifc::CTimeUs Time, m::ifc::CIO_Async Io, m::ifc::mcu::CIt It>
Ads1256Reader(Time& time, Io& io, m::ifc::mcu::IPin& cs, It& drdy)
    -> Ads1256Reader<
        std::remove_cvref_t<decltype(std::declval<Time&>().getTick())>, Time,
        Io, It>;

template <typename T>
concept CAds1256Reader = requires(T& reader, std::span<int32_t> data) {
  { reader.startRead(data) } -> std::same_as<bool>;
  { reader.readDone() } -> std::same_as<bool>;
  { reader.readed() } -> std::convertible_to<std::size_t>;
  { reader.stopRead() } -> std::same_as<bool>;
  { reader.lastValue() } -> std::same_as<int32_t>;
};

}  // namespace m::ic

#endif  // ADS1256_HPP