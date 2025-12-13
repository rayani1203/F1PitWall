#pragma once
#include <array>
#include <atomic>

template<typename T, size_t N>
class RingBuffer {
public:
    void push(const T& value) {
        size_t idx = writeIndex.fetch_add(1);
        buffer[idx % N] = value;
    }

    // read snapshot (non-blocking)
    size_t copy(T* out, size_t maxCount) const {
        size_t w = writeIndex.load();
        size_t count = (w > maxCount) ? maxCount : w;

        for (size_t i = 0; i < count; i++) {
            out[i] = buffer[(w - count + i) % N];
        }
        return count;
    }

    // Peek the most recent element without copying the whole buffer.
    // Returns false when the buffer is empty.
    bool peekLatest(T& out) const {
        size_t w = writeIndex.load();
        if (w == 0) return false;
        size_t idx = (w - 1) % N;
        out = buffer[idx];
        return true;
    }

private:
    std::array<T, N> buffer{};
    std::atomic<size_t> writeIndex{0};
};
