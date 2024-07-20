/*************************************************************************
 * @file    shm_mgr.h
 * @brief   共享内存管理
 * @author  stanjiang
 * @date    2024-07-11
 * @copyright
***/

#ifndef _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_SHM_MGR_H_
#define _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_SHM_MGR_H_

#include <vector>
#include "tcp_comm.h"
#include "logger.h"

using std::vector;

// 共享内存基本信息
struct ShmCreateInfo {
    int key;     // 共享内存key
    int size;    // 共享内存大小
    int shm_id;  // 生成的共享内存ID
    ShmMode start_mode;  // 启动模式
    void* mem_addr;      // 共享内存首地址
    int offset;  // 当前共享内存已分配的偏移
};

class ShmMgr {
 public:
    ~ShmMgr() {}
    static ShmMgr& instance(void) {
        static ShmMgr shm;
        return shm;
    }

    /***
     * @brief   创建系统共享内存
     * @param   shm_key: 共享内存key
     * @param   shm_size: 共享内存大小
     * @param   assign_size: 分配大小
     * @return  返回调用者申请的共享内存地址
     ***/
    void* create_shm(int shm_key, int shm_size, int assign_size);

    /***
     * @brief   删除共享内存
     * @param   shm_key: 共享内存key
     * @return   0:ok, -1: error
     ***/
    int destroy_shm(int shm_key);

    /***
     * @brief   获取共享内存创建模式
     * @param   shm_key: 共享内存key
     * @return  返回调用者申请的共享内存地址
     ***/
    ShmMode get_shm_mode(int shm_key);

 private:
    ShmMgr() {}
    ShmMgr(const ShmMgr&);
    ShmMgr& operator=(const ShmMgr&);

    /***
     * @brief   查找共享内存创建信息
     * @param   shm_key: 共享内存key
     * @return  返回共享内存创建信息地址
     ***/
    ShmCreateInfo* find_shm_create_info(int shm_key);

 private:
    vector<ShmCreateInfo*> shm_create_mgr_;  // 共享内存创建信息
};

inline ShmCreateInfo* ShmMgr::find_shm_create_info(int shm_key) {
    for (auto it = shm_create_mgr_.begin(); it != shm_create_mgr_.end(); ++it) {
        if ((*it)->key == shm_key) {
            Logger::log(INFO, "ShmMgr::find_shm_create_info: find exist key info: key={0:d}", shm_key);
            return *it;
        }
    }

    return NULL;
}

inline ShmMode ShmMgr::get_shm_mode(int shm_key) {
    ShmCreateInfo* mem_info = NULL;
    if ((mem_info = find_shm_create_info(shm_key)) != NULL) {
        return mem_info->start_mode;
    }

    return MODE_NONE;
}

#endif   // _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_SHM_MGR_H_

