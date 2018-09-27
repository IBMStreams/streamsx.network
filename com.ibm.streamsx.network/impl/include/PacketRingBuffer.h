/*********************************************************************
 * Copyright (C) 2018 International Business Machines Corporation
 * All Rights Reserved
 ********************************************************************/

#ifndef PACKETRING_BUFFER_H_
#define PACKETRING_BUFFER_H_

#include <stdint.h>
#include <string.h>
#include <atomic>


class PacketRingBuffer {
public:
    static const size_t ENTRY_SIZE = 2048;
    static const size_t MAX_DATA = ENTRY_SIZE - sizeof(uint32_t)*2;
    static const size_t ENTRY_COUNT = 1024*1024;     // Must be a power of 2 for the modulo shortcuts (&) below to be valid.

    typedef void (*callback_t)(void* user_data, void* pkt_data, uint32_t pkt_len);

protected:
    struct entry {
        uint32_t data_len;
        uint32_t reserved;
        uint8_t data[MAX_DATA];
    } __attribute__((aligned(128)));

public:
    PacketRingBuffer(): buffer(new entry[ENTRY_COUNT]), head(0), tail(0) {
        // Make sure the new didn't fail.  We are potentially getting quite a bit of memory here.
        assert(buffer);
    }

    ~PacketRingBuffer() {
        delete [] buffer;
    }


    // Just gets a live estimate of the current number of items in the ring
    size_t size() {
        size_t lhead = head.load(std::memory_order_relaxed);
        size_t ltail = tail.load(std::memory_order_relaxed);
        return ((lhead - ltail) & (ENTRY_COUNT - 1));
    }

    // Produces an item onto the buffer, if there is room for it
    // Returns true if the item could be added, false otherwise
    bool produce(void *data, uint32_t len) {
        if(__builtin_expect(len <= MAX_DATA, 1)) {
            // Should fit into an entry

            size_t lhead = head.load(std::memory_order_relaxed);
            if(__builtin_expect(((tail.load(std::memory_order_acquire) - (lhead + 1)) & (ENTRY_COUNT - 1)) >= 1, 1)) {
                // Room in the buffer!
                buffer[lhead].data_len = len;
                memcpy(buffer[lhead].data, data, len);

                // Update head
                head.store(((lhead + 1) & (ENTRY_COUNT - 1)), std::memory_order_release);

                return true;
            } else {
                // buffer is full
                // Unlikely path
                return false;
            }
        } else {
            // data doesn't fit in one entry.
            // Caller should have already checked for this,
            // since we're just going to drop it.
            // Unlikely path
            return false;
        }
    }

    // Consumes one item from the buffer, if there was an item to consume
    // Returns true if an item was consumed, false otherwise.
    // Calls cb for each packet before removing it from the buffer (if cb specified)
    bool consume(callback_t cb, void *user_data) {
        size_t lhead = head.load(std::memory_order_acquire);
        size_t ltail = tail.load(std::memory_order_relaxed);
        if(((lhead - ltail) & (ENTRY_COUNT - 1)) >= 1) {
            // Found something in the buffer, so execute the callback on it, if there is one
            if(__builtin_expect(cb != NULL, 1)) {
                cb(user_data, buffer[ltail].data, buffer[ltail].data_len);
            } // else no callback? Unlikely.

            // Update tail
            tail.store(((ltail + 1) & (ENTRY_COUNT - 1)), std::memory_order_release);

            return true;
        } else {
            // buffer is empty
            return false;
        }
    }

protected:
    struct entry * const buffer __attribute__((aligned(64)));

    std::atomic<size_t> head __attribute__((aligned(64)));
    std::atomic<size_t> tail __attribute__((aligned(64)));
};





#endif
