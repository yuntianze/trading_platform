/*************************************************************************
 * @file    shm_mgr.h
 * @brief   Shared memory management
 * @author  stanjiang
 * @date    2024-07-11
 * @copyright
***/

#ifndef _TRADING_PLATFORM_COMMON_SHM_MGR_H_
#define _TRADING_PLATFORM_COMMON_SHM_MGR_H_

#include <vector>
#include "tcp_comm.h"
#include "logger.h"

using std::vector;

// Basic information of shared memory
struct ShmCreateInfo {
    int key;     // Shared memory key
    int size;    // Size of shared memory
    int shm_id;  // Generated shared memory ID
    ShmMode start_mode;  // Start mode
    void* mem_addr;      // Base address of shared memory
    int offset;  // Current allocated offset in shared memory
};

class ShmMgr {
 public:
    ~ShmMgr() {}
    static ShmMgr& instance(void) {
        static ShmMgr shm;
        return shm;
    }

    /**
     * @brief   Create system shared memory
     * @param   shm_key: Shared memory key
     * @param   shm_size: Size of shared memory
     * @param   assign_size: Allocation size
     * @return  Pointer to the allocated shared memory
     */
    void* create_shm(int shm_key, int shm_size, int assign_size);

    /**
     * @brief   Delete shared memory
     * @param   shm_key: Shared memory key
     * @return  0: success, -1: error
     */
    int destroy_shm(int shm_key);

    /**
     * @brief   Get shared memory creation mode
     * @param   shm_key: Shared memory key
     * @return  Shared memory creation mode
     */
    ShmMode get_shm_mode(int shm_key);

 private:
    ShmMgr() {}
    ShmMgr(const ShmMgr&);
    ShmMgr& operator=(const ShmMgr&);

    /**
     * @brief   Find shared memory creation information
     * @param   shm_key: Shared memory key
     * @return  Pointer to shared memory creation information
     */
    ShmCreateInfo* find_shm_create_info(int shm_key);

 private:
    vector<ShmCreateInfo*> shm_create_mgr_;  // Shared memory creation information
};

inline ShmCreateInfo* ShmMgr::find_shm_create_info(int shm_key) {
    for (auto it = shm_create_mgr_.begin(); it != shm_create_mgr_.end(); ++it) {
        if ((*it)->key == shm_key) {
            Logger::log(INFO, "ShmMgr::find_shm_create_info: Found existing key info: key={0:d}", shm_key);
            return *it;
        }
    }

    return NULL;
}

inline ShmMode ShmMgr::get_shm_mode(int shm_key) {
    ShmCreateInfo* mem_info = find_shm_create_info(shm_key);
    if (mem_info != NULL) {
        return mem_info->start_mode;
    }

    return MODE_NONE;
}

#endif   // _TRADING_PLATFORM_COMMON_SHM_MGR_H_