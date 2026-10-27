#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome {
namespace comfod {

// Incremental decoder for ComfoAir frames: 07 F0 | 00 cmd len data checksum | 07 0F, and ACKs: 07 F3.
// A 0x07 in the data is sent twice; the duplicate is not counted in the length or the checksum.
class ComfoDFrameReader {
 public:
  static const uint8_t PREFIX = 0x07;
  static const uint8_t HEAD = 0xf0;
  static const uint8_t TAIL = 0x0f;
  static const uint8_t ACK = 0xf3;
  static const size_t MAX_DATA_LENGTH = 64;
  static const size_t MAX_RAW_LENGTH = 5 + 2 * MAX_DATA_LENGTH + 2 + 2;
  static const uint32_t TIMEOUT_MS = 100;

  enum Result { NONE, ACK_RECEIVED, FRAME_RECEIVED };

  Result feed(uint8_t byte, uint32_t now) {
    if (state_ != IDLE && now - last_byte_at_ > TIMEOUT_MS)
      drop_("timeout");
    last_byte_at_ = now;

    if (raw_length_ >= MAX_RAW_LENGTH) {
      drop_("frame too long");
      return feed_idle_(byte);
    }

    if (state_ == IDLE)
      return feed_idle_(byte);

    raw_[raw_length_++] = byte;

    if (state_ == START) {
      if (byte == HEAD) {
        begin_frame_();
        return NONE;
      }
      if (byte == ACK) {
        state_ = IDLE;
        return ACK_RECEIVED;
      }
      noise_bytes_ += (uint32_t) raw_length_ - 1;
      raw_length_ = 1;
      raw_[0] = byte;
      if (byte != PREFIX) {
        noise_bytes_++;
        reset_();
      }
      return NONE;
    }

    if (previous_was_prefix_) {
      previous_was_prefix_ = false;
      if (byte == PREFIX)
        return NONE;  // stuffed duplicate
      if (byte == HEAD || byte == ACK) {
        // The frame was cut off and a new message started at the previous 0x07.
        raw_length_--;
        drop_("frame cut off by a new message");
        raw_[0] = PREFIX;
        raw_[1] = byte;
        raw_length_ = 2;
        if (byte == ACK) {
          state_ = IDLE;
          return ACK_RECEIVED;
        }
        begin_frame_();
        return NONE;
      }
      if (byte == TAIL)
        return drop_("frame ended early");
    }

    switch (state_) {
      case ZERO:
        if (byte != 0x00)
          return drop_and_retry_(byte, "invalid header");
        state_ = COMMAND;
        break;
      case COMMAND:
        command_ = byte;
        state_ = LENGTH;
        break;
      case LENGTH:
        if (byte > MAX_DATA_LENGTH)
          return drop_and_retry_(byte, "data length too large");
        expected_length_ = byte;
        state_ = expected_length_ == 0 ? CHECKSUM : DATA;
        break;
      case DATA:
        data_[data_length_++] = byte;
        if (data_length_ == expected_length_)
          state_ = CHECKSUM;
        previous_was_prefix_ = byte == PREFIX;
        break;
      case CHECKSUM:
        if (byte != checksum_)
          return drop_and_retry_(byte, "checksum mismatch");
        state_ = END_PREFIX;
        // Not every sender stuffs a 0x07 checksum, END_TAIL accepts both.
        return NONE;
      case END_PREFIX:
        if (byte != PREFIX)
          return drop_("missing end marker");
        state_ = END_TAIL;
        return NONE;
      case END_TAIL:
        if (byte == TAIL) {
          state_ = IDLE;
          return FRAME_RECEIVED;
        }
        if (byte == PREFIX && checksum_ == PREFIX && !checksum_stuffing_seen_) {
          checksum_stuffing_seen_ = true;
          return NONE;
        }
        return drop_("missing end marker");
      default:
        break;
    }

    checksum_ += byte;
    return NONE;
  }

  bool in_frame() const { return state_ != IDLE; }

  uint8_t command() const { return command_; }
  uint8_t data_length() const { return data_length_; }
  const uint8_t *data() const { return data_; }

  // Bytes of the last frame or ACK exactly as received, stuffing included.
  const uint8_t *raw() const { return raw_; }
  size_t raw_length() const { return raw_length_; }

  // Set when a partial or corrupt frame was discarded; cleared by take_dropped().
  bool take_dropped() {
    bool dropped = dropped_reason_ != nullptr;
    last_dropped_reason_ = dropped_reason_;
    dropped_reason_ = nullptr;
    return dropped;
  }
  const char *dropped_reason() const { return last_dropped_reason_; }
  const uint8_t *dropped_raw() const { return dropped_raw_; }
  size_t dropped_raw_length() const { return dropped_raw_length_; }

  // Bytes outside any frame, e.g. line noise; cleared by take_noise_bytes().
  uint32_t take_noise_bytes() {
    uint32_t count = noise_bytes_;
    noise_bytes_ = 0;
    return count;
  }

 protected:
  enum State { IDLE, START, ZERO, COMMAND, LENGTH, DATA, CHECKSUM, END_PREFIX, END_TAIL };

  Result feed_idle_(uint8_t byte) {
    if (byte != PREFIX) {
      noise_bytes_++;
      return NONE;
    }
    raw_[0] = byte;
    raw_length_ = 1;
    state_ = START;
    return NONE;
  }

  void begin_frame_() {
    state_ = ZERO;
    checksum_ = 0xad;
    command_ = 0;
    expected_length_ = 0;
    data_length_ = 0;
    previous_was_prefix_ = false;
    checksum_stuffing_seen_ = false;
  }

  void reset_() {
    state_ = IDLE;
    previous_was_prefix_ = false;
  }

  Result drop_(const char *reason) {
    dropped_reason_ = reason;
    dropped_raw_length_ = raw_length_;
    memcpy(dropped_raw_, raw_, raw_length_);
    reset_();
    return NONE;
  }

  // A 0x07 that broke the frame may be the start of the next message.
  Result drop_and_retry_(uint8_t byte, const char *reason) {
    drop_(reason);
    if (byte == PREFIX)
      return feed_idle_(byte);
    return NONE;
  }

  State state_{IDLE};
  uint8_t raw_[MAX_RAW_LENGTH];
  size_t raw_length_{0};
  uint8_t data_[MAX_DATA_LENGTH];
  uint8_t data_length_{0};
  uint8_t expected_length_{0};
  uint8_t command_{0};
  uint8_t checksum_{0};
  bool previous_was_prefix_{false};
  bool checksum_stuffing_seen_{false};
  uint32_t last_byte_at_{0};
  uint32_t noise_bytes_{0};

  const char *dropped_reason_{nullptr};
  const char *last_dropped_reason_{nullptr};
  uint8_t dropped_raw_[MAX_RAW_LENGTH];
  size_t dropped_raw_length_{0};
};

}  // namespace comfod
}  // namespace esphome
