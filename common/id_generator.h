/*************************************************************************
 * @file    id_generator.h
 * @brief   Generate global unique id
 * @author  stanjiang
 * @date    2024-08-17
 * @copyright
***/

#ifndef _COMMON_ID_GENERATOR_H_
#define _COMMON_ID_GENERATOR_H_

#include <cstdint>
#include <atomic>
#include <chrono>

class IdGenerator {
public:
    static IdGenerator& instance();

    // Generate a new unique 64-bit ID
    uint64_t generateId();

private:
    IdGenerator();
    ~IdGenerator();
    IdGenerator(const IdGenerator&) = delete;
    IdGenerator& operator=(const IdGenerator&) = delete;

    std::atomic<uint32_t> sequence_;
    int32_t machineId_;
    const int64_t epoch_;
};

#endif // _COMMON_ID_GENERATOR_H_