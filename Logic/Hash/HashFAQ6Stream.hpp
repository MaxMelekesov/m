/**
 * @file    HashFAQ6Stream.hpp
 * @brief   Streaming FAQ6 hash — incremental update() + finalize().
 *
 * Satisfies CStreamHash concept. Use for CRC over large flash regions
 * without buffering the entire data.
 *
 * Part of m library.
 */

#ifndef HASHFAQ6STREAM_HPP
#define HASHFAQ6STREAM_HPP

#include <IStreamHash.hpp>
#include <TSerDes.hpp>
#include <cstdint>
#include <span>

namespace m {

class HashFAQ6Stream final : public ifc::IStreamHash<4> {
 public:
  using Storage = ifc::IStreamHash<4>::Storage;

  void update(std::span<const uint8_t> data) override {
    for (std::size_t i = 0; i < data.size(); ++i) {
      state_ += data[i];
      state_ += (state_ << 10);
      state_ ^= (state_ >> 6);
    }
  }

  Storage finalize() override {
    uint32_t h = state_;
    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
    state_ = 0;

    Storage hash;
    m::serialize(hash, h);
    return hash;
  }

 private:
  uint32_t state_ = 0;
};

static_assert(ifc::CStreamHash<HashFAQ6Stream>,
              "HashFAQ6Stream must satisfy CStreamHash");

}  // namespace m

#endif  // HASHFAQ6STREAM_HPP
