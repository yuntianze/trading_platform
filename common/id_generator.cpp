#include "id_generator.h"
#include <unistd.h>
#include <thread>

IdGenerator::IdGenerator()
    : sequence_(0),
      machineId_(gethostid() & 0x3FF),  // Use last 10 bits of host id
      epoch_(1672531200000)  // 2023-01-01 00:00:00 UTC
{
}

IdGenerator::~IdGenerator() {
}

IdGenerator& IdGenerator::instance() {
    static IdGenerator instance;
    return instance;
}

uint64_t IdGenerator::generateId() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    int64_t timestamp = milliseconds - epoch_;
    
    uint32_t seq = sequence_.fetch_add(1, std::memory_order_relaxed) & 0xFFF;
    
    if (seq == 0) {
        // If we've rolled over, sleep for 1 millisecond
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        now = std::chrono::system_clock::now();
        duration = now.time_since_epoch();
        milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        timestamp = milliseconds - epoch_;
    }
    
    return ((timestamp & 0x1FFFFFFFFFLL) << 22) | ((int64_t)machineId_ << 12) | seq;
}