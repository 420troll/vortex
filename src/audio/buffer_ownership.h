#pragma once
#include <array>
#include <cstddef>

namespace vortex {
// Worker-owned model: a buffer becomes writable only after its device completion.
template<std::size_t Count>
class BufferOwnership {
    static_assert(Count>0);
public:
    bool is_queued(std::size_t index) const noexcept { return index<Count && queued_[index]; }
    bool submit(std::size_t index) noexcept {
        if(index>=Count || queued_[index]) return false;
        queued_[index]=true; ++queued_count_; return true;
    }
    bool complete(std::size_t index) noexcept {
        if(index>=Count || !queued_[index]) return false;
        queued_[index]=false; --queued_count_; return true;
    }
    std::size_t queued_count() const noexcept { return queued_count_; }
    static constexpr std::size_t capacity() noexcept { return Count; }
private:
    std::array<bool,Count> queued_{};
    std::size_t queued_count_=0;
};
}
