#pragma once

#include "../types.h"

class BitField16 {
   public:
    BitField16() : value_(0) {}

    // Sets the value of the specified group to the given value
    void setGroupValue(uint16_t group_index, uint16_t group_value) {
        assert(group_value < 16 && "group_value must be less than 16");
        assert(group_index < 4 && "group_index must be less than 4");

        // calculate the bit position of the start of the group you want to set
        uint16_t startBit = group_index * group_size_;
        auto setMask = static_cast<uint16_t>(group_value << startBit);

        // clear the bits in the group
        value_ &= ~(0xF << startBit);

        // set the bits in the group
        value_ |= setMask;
    }

    [[nodiscard]] uint16_t getGroup(uint16_t group_index) const {
        assert(group_index < 4 && "group_index must be less than 4");
        uint16_t startBit = group_index * group_size_;
        return (value_ >> startBit) & 0xF;
    }

    void clear() { value_ = 0; }
    [[nodiscard]] uint16_t get() const { return value_; }

   private:
    static constexpr uint16_t group_size_ = 4;  // size of each group
    uint16_t value_;
};