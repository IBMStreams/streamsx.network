/*********************************************************************
 * Copyright (C) 2018 International Business Machines Corporation
 * All Rights Reserved
 ********************************************************************/

#ifndef PACKETRING_BUFFER_H_
#define PACKETRING_BUFFER_H_

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <atomic>
#include <sys/uio.h>

class PacketRingBuffer {
public:
    // The entry size and entry count must be powers of 2 for everything to work out right.
    static const size_t ENTRY_SIZE_BITS = 7;
    static const size_t ENTRY_COUNT_BITS = 18;    // 2^15 x 128 byte entries would allow 8 rings to fit in the L3 of one socket, but that's pretty small.
    static const size_t ENTRY_SIZE = 1 << ENTRY_SIZE_BITS;
    static const size_t ENTRY_COUNT = 1 << ENTRY_COUNT_BITS;
    static_assert(ENTRY_SIZE > sizeof(uint32_t)*2, "PacketRingBuffer::ENTRY_SIZE must be large enough for the entry header.");
    static_assert(ENTRY_SIZE >= 64, "PacketRingBuffer::ENTRY_SIZE must be at least the size of a cache line.");

    typedef void (*callback_t)(void* user_data, void* pkt_data, uint32_t pkt_len);

protected:
    // This entry represents just the initial entry of a given packet in the ring
    // the other entries are just raw data (in 128 byte chunks), right after this one.
    struct entry {
        uint32_t data_len;
        uint32_t dummy_packet;   // Indicates (=1) this is not actually packet data, and just used to pad the ring.  Ignore on dequeue. 0 otherwise
        uint8_t data[ENTRY_SIZE - sizeof(uint32_t)*2];
    } __attribute__((aligned(128),packed));

protected:
    // Helper to compute lengths in the ring, including around the end
    static constexpr inline size_t length(size_t a, size_t b) {
        return ((b - a) & (ENTRY_COUNT - 1));
    }

    // Helper to compute the appropriate next index in the ring, including around the end
    static constexpr inline size_t next(size_t index, size_t offset) {
        return ((index + offset) & (ENTRY_COUNT - 1));
    }

    // Helper to compute the amount of space currently used in the ring
    // This is used on the consumer side, and yields a lower bound, since
    // the producer may still be adding things.
    static constexpr inline size_t used_size(size_t head, size_t tail) {
        return length(tail, head);
    }

    // Helper to compute the amount of space currently left in the ring
    // This is used on the producer side, and yields a lower bound, since
    // the consumer may be making more room right now.
    static constexpr inline size_t free_space(size_t head, size_t tail) {
        return length(head + 1, tail);
    }

    // Helper to compute the amount of space current left in the ring UP TO BUT NOT PAST THE END OF THE RING!
    // This is used on the producer side, and yields a lower bound, since
    // the consumer may be making more room right now.
    static inline size_t free_space_contiguous(size_t head, size_t tail) {
        size_t tentative_space = ENTRY_COUNT - (head + 1);
        size_t total_free_space = (tentative_space + tail) & (ENTRY_COUNT - 1);  // tricky, but if head/tail turned into references to the actual items, ensures each is only read once.
        return ((total_free_space <= tentative_space) ? total_free_space : (tentative_space + 1));
    }

    // Helper to convert actual data lengths into the count of ENTRY_SIZE items needed to store it.
    // Takes into account the header in front of the first entry.
    static constexpr inline size_t computeEntryCount(size_t data_len) {
        return (((data_len + sizeof(uint32_t)*2 - 1) >> ENTRY_SIZE_BITS) + 1);
    }

    // Helper to convert entry counts of ENTRY_SIZE items into a data length.
    // Takes into account the header in front of the first entry.
    // This is particularly used when padding the end of the ring, before
    // adding an item that is too big for the current space left at the end of the ring.
    static constexpr inline size_t computeDummyLength(size_t count) {
        return ((count << ENTRY_SIZE_BITS) - sizeof(uint32_t)*2);
    }

public:
    PacketRingBuffer(): buffer(new entry[ENTRY_COUNT]), head(0), tail(0) {
        // Make sure the new didn't fail.  We are potentially getting quite a bit of memory here.
        assert(buffer);
    }

    ~PacketRingBuffer() {
        delete [] buffer;
    }


    // Just gets a live estimate of the current number of items in the ring
    // From the consumer thread, this is a lower bound, since the producer
    // could still be adding things to the ring.
    size_t size() {
        return used_size(head.load(std::memory_order_relaxed), tail.load(std::memory_order_relaxed));
    }

    // Produces an item onto the buffer, if there is room for it
    // Returns true if the item could be added, false otherwise
    bool produce(void *data, uint32_t len) {
        size_t needed_space = computeEntryCount(len);
        size_t lhead = head.load(std::memory_order_relaxed);
        size_t ltail = tail.load(std::memory_order_acquire);
        size_t space = free_space(lhead, ltail);
        size_t free_space_end = free_space_contiguous(lhead, ltail);

        // Since we are using a local copy of tail, and the computations of free space, we're somewhat pessimistic about what will fit,
        // but at least it should be self-consistent.

        if(__builtin_expect(needed_space > free_space_end, 0)) {
            // Doesn't fit in the ring contiguously at the end.
            // Unlikely path.
            // Check to see if it would fit in the front (if free space is split).
            if(__builtin_expect(needed_space > (space - free_space_end), 0)) {
                // Nope.
                // Buffer is full.
                // Unlikely path, even given the earlier unlikely path has been taken.
                return false;
            } else {
                // Seems like it will.  Let's insert a dummy to wrap the buffer.
                buffer[lhead].data_len = computeDummyLength(free_space_end);
                buffer[lhead].dummy_packet = 1; // This is just a dummy padding packet

                // Update head (local and actual)
                lhead = next(lhead, free_space_end);
                head.store(lhead, std::memory_order_release);
            }
        }

        // At this point, we know we should fit, contiguously, at the current lhead, or we would
        // have returned already.  We may have already inserted a dummy padding packet.
        buffer[lhead].data_len = len;
        buffer[lhead].dummy_packet = 0; // Real packet.
        memcpy(buffer[lhead].data, data, len);

        // Update head
        head.store(next(lhead, needed_space), std::memory_order_release);

        return true;
    }

    // Produces multiple items onto the buffer, to the extent there is room.
    // Returns the count of produced items.
    // Only updates the tail pointer after producing as many entries as we could.
    // Takes a single initial tail snapshot, so the consumer may be
    // consuming more entries after we start producing, but we will
    // only produce up to the point of entries that were free
    // at the time we started.
    size_t produceMulti(const struct iovec *vector, size_t count) {
        size_t lhead = head.load(std::memory_order_relaxed);
        size_t ltail = tail.load(std::memory_order_acquire);
        size_t slots_available = free_space(lhead, ltail);
        size_t slots_available_end = free_space_contiguous(lhead, ltail);
        size_t slots_consumed = 0;
        size_t packets_produced = 0;

        // If we're calling this function, we can assume at least one packet is in
        // the vector, likely more, and assume best case the ring is not full.
        while(__builtin_expect(slots_available > 0 && packets_produced < count, 1)) {
            // Space is available and we have packets left.
            size_t needed_slots = computeEntryCount(vector[packets_produced].iov_len);

            if(__builtin_expect(needed_slots > slots_available_end, 0)) {
                // Not enough contiguous room for the current packet.
                // Unlikely path.
                // Check to see if it would fit in the front (if free space is split).
                if(__builtin_expect(needed_slots > (slots_available - slots_available_end), 0)) {
                    // Doesn't fit in front, either.
                    // Buffer is full, or full enough that this packet doesn't fit.
                    // Unlikely path, even given the earlier unlikely path has been taken.

                    // Update the global head, if we did anything.
                    // Assuming the "better" case, where we did actually put some packets into
                    // the ring, just not all.
                    if(__builtin_expect(slots_consumed > 0,1)) {
                        head.store(lhead, std::memory_order_release);
                    }

                    return packets_produced;
                } else {
                    // Seems like it will fit in the front.  Let's insert a dummy to wrap the buffer.
                    buffer[lhead].data_len = computeDummyLength(slots_available_end);
                    buffer[lhead].dummy_packet = 1; // This is just a dummy padding packet

                    // Update just local variables for now.
                    lhead = next(lhead, slots_available_end);
                    slots_available -= slots_available_end;
                    slots_consumed += slots_available_end;
                    slots_available_end = slots_available;
                }
            }

            // At this point, we know we should fit, contiguously, at the current lhead, or we would
            // have returned already.  We may have already inserted a dummy padding packet.
            buffer[lhead].data_len = vector[packets_produced].iov_len;
            buffer[lhead].dummy_packet = 0; // Real packet.
            memcpy(buffer[lhead].data, vector[packets_produced].iov_base, vector[packets_produced].iov_len);

            // Update just local variables for now.
            lhead = next(lhead, needed_slots);
            slots_available -= needed_slots;
            slots_available_end -= needed_slots;
            slots_consumed += needed_slots;
            ++packets_produced;
        }

        // All done with this vector of packets!
        // Update the global head, if we did anything.
        // Assume the good case, where there was room in the ring for our packets.
        // Actually, since we return early in the buffer full case, this would only
        // be false if the vector was empty...
        if(__builtin_expect(slots_consumed > 0, 1)) {
            head.store(lhead, std::memory_order_release);
        }

        return packets_produced;
    }

    // Consumes one item from the buffer, if there was an item to consume
    // Returns true if an item was consumed, false otherwise.
    // Calls cb for each packet before removing it from the buffer (if cb specified)
    bool consume(callback_t cb, void *user_data) {
        do {
            size_t ltail = tail.load(std::memory_order_relaxed);
            if(used_size(head.load(std::memory_order_acquire), ltail) >= 1) {
                // Found something in the buffer!
                size_t skip_len = computeEntryCount(buffer[ltail].data_len);
                bool dummy_packet = (buffer[ltail].dummy_packet == 1);

                if(__builtin_expect(!dummy_packet, 1)) {
                    // Real packet!

                    // Use the callback on it (if there is one)
                    if(__builtin_expect(cb != NULL, 1)) {
                        cb(user_data, buffer[ltail].data, buffer[ltail].data_len);
                    } // else no callback? Unlikely.

                } // else a dummy.  Have to keep going.  Unlikely path.

                // Update tail
                tail.store(next(ltail, skip_len), std::memory_order_release);

                if(__builtin_expect(!dummy_packet, 1)) {
                    // Wasn't a dummy. Can return immediately.
                    return true;
                } // else a dummy.  Have to keep going.  Unlikely path.
            } else {
                // buffer is empty.
                // We can just return immediately.
                return false;
            }
        } while(__builtin_expect(true, 1));
    }

    // Consumes all available entries from the buffer, if there is any
    // Returns the count of consumed items.
    // Calls cb for each packet (if specified).
    // Consumes up to max_burst items at once (0 implies as many as possible).
    // Only updates the tail pointer after consuming all of the entries.
    // Takes a single initial head snapshot, so the producer may be
    // producing more entries after we start consuming, but we will
    // only consume up to the point of entries that had been produced
    // at the time we started.
    size_t consumeAll(callback_t cb, void *user_data, size_t max_burst = 0) {
        size_t ltail = tail.load(std::memory_order_relaxed);
        size_t items_available = used_size(head.load(std::memory_order_acquire), ltail);
        size_t items_consumed = 0;
        size_t packets_consumed = 0;

        // We won't attempt to predict this while loop clause ... empty queue might be just as likely as one thing in queue, and a whole bunch in queue.
        while((items_consumed < items_available) && ((max_burst == 0) || (packets_consumed < max_burst))) {
            size_t skip_len = computeEntryCount(buffer[ltail].data_len);
            bool dummy_packet = (buffer[ltail].dummy_packet == 1);

            if(__builtin_expect(!dummy_packet, 1)) {
                // Real packet!

                // Use the callback on it (if there is one)
                if(__builtin_expect(cb != NULL, 1)) {
                    cb(user_data, buffer[ltail].data, buffer[ltail].data_len);
                } // else no callback? Unlikely.

                ++packets_consumed;
            } // else a dummy.  Have to keep going.  Unlikely path.

            // Update local tail
            ltail = next(ltail, skip_len);
            items_consumed += skip_len;
        }

        // All done with this burst.
        // Update the global tail, if we did anything.
        // Won't attempt to predict this one, since the queue might be equally likely to be empty as not-empty.
        if(items_consumed > 0) {
            tail.store(ltail, std::memory_order_release);
        }

        return packets_consumed;
    }

protected:
    struct entry * const buffer __attribute__((aligned(64)));

    std::atomic<size_t> head __attribute__((aligned(64)));
    std::atomic<size_t> tail __attribute__((aligned(64)));
};





#endif
