#ifndef PT24G_AMIBERRY_COORDINATE_TRACE_H
#define PT24G_AMIBERRY_COORDINATE_TRACE_H
// Private diagnostic only. No input, guest reads, file opens or IPC commands.
// The original caller supplies write_log/_T after its normal sysdeps.h include.
#include <atomic>
namespace pt24g_coordinate_trace {
inline std::atomic<unsigned> requests{0};
inline thread_local bool active = false;
inline thread_local unsigned sequence = 0;
inline thread_local unsigned rows = 0;
class Scope {
    const bool previous_active = active;
    const unsigned previous_sequence = sequence;
    const unsigned previous_rows = rows;
public:
    Scope() {
        active = false;
        if (previous_active) return; // Do not mix nested requests.
        unsigned count = requests.load(std::memory_order_relaxed);
        while (count < 8) {
            if (requests.compare_exchange_weak(count, count + 1, std::memory_order_relaxed)) {
                sequence = count + 1; rows = 0; active = true; return;
            }
        }
    }
    ~Scope() { active = previous_active; sequence = previous_sequence; rows = previous_rows; }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};
inline bool take() {
    if (!active || rows >= 32) return false;
    ++rows;
    return true;
}
}
#define PT24G_COORD_TRACE(stage, format, ...) do { \
    if (pt24g_coordinate_trace::take()) \
        write_log(_T("PT24G_COORD_V1 seq=%u stage=" stage " " format "\n"), \
                  pt24g_coordinate_trace::sequence, __VA_ARGS__); \
} while (0)
#endif
