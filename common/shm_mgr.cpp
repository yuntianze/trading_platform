#include <sys/ipc.h>
#include <sys/shm.h>
#include <algorithm>
#include "shm_mgr.h"
#include "logger.h"

void* ShmMgr::create_shm(int shm_key, int shm_size, int assign_size) {
    // Check if shared memory with the given key already exists
    ShmCreateInfo* shm_info = find_shm_create_info(shm_key);
    if (shm_info != NULL) {
        // Check if current shared memory has enough space for allocation
        if ((shm_info->offset + assign_size) > shm_info->size) {
            Logger::log(ERROR, "Not enough shared memory to assign: key={0:d}, shmSize={1:d}, assignSize={2:d}",
                    shm_key, shm_info->size, assign_size);
            return NULL;
        }

        void* assign_shm_mem_addr = static_cast<char*>(shm_info->mem_addr) + shm_info->offset;
        shm_info->offset += assign_size;

        Logger::log(INFO, "Allocated shared memory: key={0:d}, shmSize={1:d}, assignSize={2:d}",
                shm_key, shm_info->size, assign_size);

        return assign_shm_mem_addr;
    }

    // Create new shared memory for the given key
    char* shm_mem = NULL;
    ShmMode start_mode = MODE_INIT;
    int shm_id = shmget(shm_key, shm_size, IPC_CREAT|IPC_EXCL|0666);
    if (shm_id < 0) {
        if (errno != EEXIST) {
            Logger::log(ERROR, "Can't create SHM, key={0:d}, size={1:d}, ErrMsg={2:s}", shm_key, shm_size, strerror(errno));
            return NULL;
        }

        // Shared memory already exists
        start_mode = MODE_RESUME;
        shm_id = shmget(shm_key, shm_size, 0666);
        if (shm_id < 0) {
            Logger::log(ERROR, "shmget get existing shm error, key={0:d}, size={1:d}", shm_key, shm_size);
            return NULL;
        }
    }

    shm_mem = static_cast<char*>(shmat(shm_id, NULL, 0));
    if (shm_mem == NULL) {
        Logger::log(ERROR, "shmat error, key={0:d}, size={1:d}", shm_key, shm_size);
        return NULL;
    }

    // Save successfully created shared memory information
    shm_info = new ShmCreateInfo;
    shm_info->key = shm_key;
    shm_info->size = shm_size;
    shm_info->start_mode = start_mode;
    shm_info->mem_addr = static_cast<void*>(shm_mem);
    shm_info->offset = assign_size;
    shm_info->shm_id = shm_id;
    shm_create_mgr_.push_back(shm_info);

    Logger::log(INFO, "Shared memory created: key={0:d}, shmSize={1:d}, assignSize={2:d}, shmBlockNum={3:d}",
            shm_key, shm_size, assign_size, shm_create_mgr_.size());

    return static_cast<void*>(shm_mem);
}

int ShmMgr::destroy_shm(int shm_key) {
    ShmCreateInfo* shm_info = find_shm_create_info(shm_key);
    if (shm_info != NULL) {
        if (shmctl(shm_info->shm_id, IPC_RMID, NULL) == -1) {
            Logger::log(ERROR, "Failed to destroy shared memory: key={0:d}, error={1:s}", shm_key, strerror(errno));
            return -1;
        }

        auto it = std::find(shm_create_mgr_.begin(), shm_create_mgr_.end(), shm_info);
        if (it != shm_create_mgr_.end()) {
            shm_create_mgr_.erase(it);
        }
        delete shm_info;

        Logger::log(INFO, "Shared memory destroyed: key={0:d}", shm_key);
        return 0;
    }

    Logger::log(ERROR, "Shared memory not found for destruction: key={0:d}", shm_key);
    return -1;
}