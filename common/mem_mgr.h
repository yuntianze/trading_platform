/*************************************************************************
 * @file   mem_mgr.h
 * @brief  统一内存管理，内存池中包含固定大小不等的内存块，
 *          而每个内存块包含了固定数量和大小相等的内存单元
 * @author stanjiang
 * @date   2024-07-11
 * @copyright
***/
#ifndef _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_MEM_MGR_H_
#define _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_MEM_MGR_H_

#include <new>
#include "tcp_comm.h"
#include "logger.h"


enum MemBlockUseFlag {
    EMBU_FREE = 0,
    EMBU_USED,
    EMBU_MAX
};

// 内存池中每个单元块的类型
enum MemBlockType {
    BLOCK_ORDER_BUY = 0,  // 买单对象
    BLOCK_ORDER_SELL,     // 卖单对象

    BLOCK_MAX
};

// 内存块头节点
struct MemBlockHead {
    UINT block_unit_size;  // 内存块中每个单位的大小
    UINT block_unit_num;  // 内存块中的单位数量
    MemBlockType block_type;  // 内存块类型
    ULONG offset;  // 内存块在内存池中的偏移量
    UINT used_num;  // 内存块已使用的单位数量
    int queue_front;  // 内存块用队列来组织，此为队列头
    int queue_tail;   // 队列尾
    UINT* unit_index;  // 内存单位索引号数组
    char* unit_used_flag;  // 内存单位使用情况数组
    char* data;  // 内存块存放数据起始地址
};


class MemoryBlock {
 public:
    MemoryBlock() : mem_block_head_(nullptr) {}
    ~MemoryBlock() {}

    /***
    * @brief   初始化内存块
    * @param   type: 内存块类型
    * @param   size: 内存块中每个单位的大小
    * @param   num: 内存块中的单位数量
    * @param   offset: 内存块在内存池中的偏移量
    * @return  0: 分配成功，-1：分配失败
    ***/
    int init(MemBlockType type, UINT size, UINT num, ULONG offset);

    /***
    * @brief   获取指定类型内存块空闲的内存单位
    * @param   mem_unit_index: 内存单位索引
    * @return  返回新分配的内存单位地址,null为失败
    ***/
    char* get_free_obj(UINT& mem_unit_index);

    /***
    * @brief   释放指定类型的内存块的内存单位
    * @param   mem_unit_index: 内存单位索引
    * @return  0: 成功，-1：失败
    ***/
    int release_obj(UINT mem_unit_index);

    /***
    * @brief   获取指定类型内存块已存在的内存单位
    * @param   mem_unit_index: 内存单位索引
    * @return  返回内存单位地址,null为失败
    ***/
    char* get_obj(UINT mem_unit_index);

    /***
    * @brief   判定内存块队列为空
    * @return  true:ok ,false:error
    ***/
    bool is_empty();

    /***
    * @brief   判定内存块队列已满
    * @return  true:ok ,false:error
    ***/
    bool is_full();

 private:
    friend class MemoryPool;
    MemBlockHead* mem_block_head_;  // 内存块头指针
};

inline bool MemoryBlock::is_empty() {
    if (NULL == mem_block_head_) {
        Logger::log(ERROR, "block head is null");
        return false;
    }

    if (mem_block_head_->queue_front == mem_block_head_->queue_tail) {
        return true;
    }

    return false;
}

inline bool MemoryBlock::is_full() {
    if (NULL == mem_block_head_) {
        Logger::log(ERROR, "block head is null");
        return false;
    }

    if (0 == mem_block_head_->block_unit_num) {
        Logger::log(ERROR, "block unit num is 0");
        return false;
    }

    int next_index = (mem_block_head_->queue_front + 1) % mem_block_head_->block_unit_num;
    if (next_index == mem_block_head_->queue_tail) {
        return true;
    }

    return false;
}


// 内存池中内存块的基本信息
struct MemBlockInfo {
    MemoryBlock block_obj;  // 内存块对象
    ULONG block_offset;  // 内存块相对于内存池的地址偏移
};


class MemoryPool {
 public:
    static MemoryPool& instance(void) {
        static MemoryPool pool;
        return pool;
    }

    ~MemoryPool() {}

    /***
    * @brief   初始化内存池
    * @return  0: 分配成功，-1：分配失败
    ***/
    int init(void);

    /***
    * @brief   分配内存池中每个指定类型与数量的内存块
    * @param   type: 内存块类型
    * @param   size: 内存块中每个单位的大小
    * @param   num: 内存块中的单位数量
    * @return  0: 分配成功，-1：分配失败
    ***/
    int alloc(MemBlockType type, UINT size, UINT num);

    /***
    * @brief   获取指定类型内存块空闲的内存单位
    * @param   type: 内存块类型
    * @param   index: 内存单位索引
    * @return  返回新分配的内存单位地址,null为失败
    ***/
    char* get_free_obj(MemBlockType type, UINT& index);

    /***
    * @brief   释放指定类型的内存块的内存单位
    * @param   type: 内存块类型
    * @param   index: 内存单位索引
    * @return  0: 成功，-1：失败
    ***/
    int release_obj(MemBlockType type, UINT index);

    /***
    * @brief   获取指定类型内存块已存在的内存单位
    * @param   type: 内存块类型
    * @param   index: 内存单位索引
    * @return  返回内存单位地址,null为失败
    ***/
    char* get_obj(MemBlockType type, UINT index);

    /***
    * @brief   获取内存池起始地址
    * @param   void
    * @return  内存池起始地址
    ***/
    char* get_mempool_base_attr(void) {
        return mempool_base_attr_;
    }

    /***
    * @brief   获取玩家对象池头指针
    * @param   type: 内存块类型
    * @return  头指针
    ***/
    MemBlockHead* get_pool_obj_head(MemBlockType type);

 private:
    MemoryPool() {}
    explicit MemoryPool(const MemoryPool&);
    MemoryPool& operator=(const MemoryPool&);

 private:
    ULONG pool_size_;   // 内存池总尺寸
    ULONG free_size_;   // 内存池空闲尺寸
    ULONG cur_offset_;  // 当前内存池偏移位置
    USHORT block_num_;  // 内存块个数
    MemBlockInfo block_info_[EMBU_MAX];  // 内存块信息
    char* mempool_base_attr_;  // 内存池的起始地址
};


inline char* MemoryPool::get_free_obj(MemBlockType type, UINT &index) {
    return block_info_[type].block_obj.get_free_obj(index);
}

inline int MemoryPool::release_obj(MemBlockType type, UINT index) {
    return block_info_[type].block_obj.release_obj(index);
}

inline char* MemoryPool::get_obj(MemBlockType type, UINT index) {
    return block_info_[type].block_obj.get_obj(index);
}

inline MemBlockHead* MemoryPool::get_pool_obj_head(MemBlockType type) {
    if (type >= BLOCK_MAX) {
        Logger::log(ERROR, "invalid type, type{0:d}", static_cast<int>(type));
        return NULL;
    }

    return block_info_[type].block_obj.mem_block_head_;
}


#endif  // _USERS_JIANGPENG_CODE_TRADING_PLATFORM_COMMON_MEM_MGR_H_
