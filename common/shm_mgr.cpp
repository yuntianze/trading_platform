/********************************************************************
 * @file    shm_mgr.cpp
 * @brief   共享内存管理
 * @author  stanjiang
 * @date    2024-07-17
 * @copyright
***/

#include <sys/ipc.h>
#include <sys/shm.h>
#include <algorithm>
#include "shm_mgr.h"
#include "logger.h"


void* ShmMgr::create_shm(int shm_key, int shm_size, int assign_size) {
    // 查找是否已经创建有该key的共享内存,若有则尝试在该共享内存上分配空间
    ShmCreateInfo* shm_info = find_shm_create_info(shm_key);
    if (NULL != shm_info) {
        // 检查当前共享内存是否足够分配
        if ((shm_info->offset + assign_size) > shm_info->size) {
            Logger::log(ERROR, "shmmem isn't enough to assign: key={0:d}, shmSize={0:d}, assignSize={0:d}",
                    shm_key, shm_info->size, assign_size);
            return NULL;
        }

        void* assign_shm_mem_addr = static_cast<void*>(static_cast<char*>(shm_info->mem_addr) + shm_info->offset);
        shm_info->offset += assign_size;

        Logger::log(ERROR, "shmmem isn't enough to assign: key={0:d}, shmSize={0:d}, assignSize={0:d}",
                shm_key, shm_info->size, assign_size);

        return assign_shm_mem_addr;
    }

    // 全新创建key所对应的共享内存
    char* shm_mem = NULL;
    ShmMode start_mode = MODE_INIT;
    int shm_id = shmget(shm_key, shm_size, IPC_CREAT|IPC_EXCL|0666);
    if (shm_id < 0) {
        if (errno != EEXIST) {
            Logger::log(ERROR, "Can't create SHM, key={0:d}, size={0:d}, ErrMsg={0:s}", shm_key, shm_size, strerror(errno));
            return NULL;
        }

        // 共享内存已经创建
        start_mode = MODE_RESUME;
        shm_id = shmget(shm_key, shm_size, 0666);
        if (shm_id < 0) {
            Logger::log(ERROR, "shmget get exist shm error, key={0:d}, size={0:d}", shm_key, shm_size);
            return NULL;
        }
    }

    shm_mem = static_cast<char*>(shmat(shm_id, NULL, 0));
    if (NULL == shm_mem) {
        Logger::log(ERROR, "shmat error, key={0:d}, size={0:d}", shm_key, shm_size);
        return NULL;
    }

    // 保存成功创建的共享内存信息
    shm_info = new ShmCreateInfo;
    shm_info->key = shm_key;
    shm_info->size = shm_size;
    shm_info->start_mode = start_mode;
    shm_info->mem_addr = static_cast<void*>(shm_mem);
    shm_info->offset = assign_size;
    shm_info->shm_id = shm_id;
    shm_create_mgr_.push_back(shm_info);

    Logger::log(INFO, "shmmem create ok: key={0:d}, shmSize={0:d}, assignSize={0:d}, shmBlockNum={0:d}",
            shm_key, shm_size, assign_size, shm_create_mgr_.size());

    return static_cast<void*>(shm_mem);
}


int ShmMgr::destroy_shm(int shm_key) {
    ShmCreateInfo* shm_info = NULL;
    if ((shm_info = find_shm_create_info(shm_key)) != NULL) {
        shmctl(shm_info->shm_id, IPC_RMID, NULL);

        remove(shm_create_mgr_.begin(), shm_create_mgr_.end(), shm_info);
        delete shm_info;
    }

    return -1;
}


