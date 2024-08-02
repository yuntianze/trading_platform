/*************************************************************************
 * @file   mem_mgr.h
 * @brief  Unified memory management. The memory pool contains memory blocks 
 *         of fixed but unequal sizes, and each memory block contains a fixed 
 *         number of memory units of equal size.
 * @author stanjiang
 * @date   2024-07-11
 * @copyright
***/
#ifndef _TRADING_PLATFORM_COMMON_MEM_MGR_H_
#define _TRADING_PLATFORM_COMMON_MEM_MGR_H_

#include <new>
#include "tcp_comm.h"
#include "logger.h"

// Memory block usage flag
enum MemBlockUseFlag {
    EMBU_FREE = 0,
    EMBU_USED,
    EMBU_MAX
};

// Types of units in the memory pool
enum MemBlockType {
    BLOCK_ORDER_BUY = 0,  // Buy order object
    BLOCK_ORDER_SELL,     // Sell order object

    BLOCK_MAX
};

// Memory block header node
struct MemBlockHead {
    UINT block_unit_size;  // Size of each unit in the memory block
    UINT block_unit_num;   // Number of units in the memory block
    MemBlockType block_type;  // Memory block type
    ULONG offset;  // Offset of the memory block in the memory pool
    UINT used_num;  // Number of used units in the memory block
    int queue_front;  // Front of the queue organizing the memory block
    int queue_tail;   // Tail of the queue
    UINT* unit_index;  // Array of memory unit index numbers
    char* unit_used_flag;  // Array indicating usage of memory units
    char* data;  // Starting address for data storage in the memory block
};

class MemoryBlock {
 public:
    MemoryBlock() : mem_block_head_(nullptr) {}
    ~MemoryBlock() {}

    /**
     * @brief   Initialize the memory block
     * @param   type: Memory block type
     * @param   size: Size of each unit in the memory block
     * @param   num: Number of units in the memory block
     * @param   offset: Offset of the memory block in the memory pool
     * @return  0: Success, -1: Failure
     */
    int init(MemBlockType type, UINT size, UINT num, ULONG offset);

    /**
     * @brief   Get a free memory unit from the specified type of memory block
     * @param   mem_unit_index: Memory unit index
     * @return  Address of the newly allocated memory unit, NULL if failed
     */
    char* get_free_obj(UINT& mem_unit_index);

    /**
     * @brief   Release a memory unit of the specified type of memory block
     * @param   mem_unit_index: Memory unit index
     * @return  0: Success, -1: Failure
     */
    int release_obj(UINT mem_unit_index);

    /**
     * @brief   Get an existing memory unit from the specified type of memory block
     * @param   mem_unit_index: Memory unit index
     * @return  Address of the memory unit, NULL if failed
     */
    char* get_obj(UINT mem_unit_index);

    /**
     * @brief   Check if the memory block queue is empty
     * @return  true: Empty, false: Not empty
     */
    bool is_empty();

    /**
     * @brief   Check if the memory block queue is full
     * @return  true: Full, false: Not full
     */
    bool is_full();

 private:
    friend class MemoryPool;
    MemBlockHead* mem_block_head_;  // Pointer to memory block header
};

inline bool MemoryBlock::is_empty() {
    if (mem_block_head_ == NULL) {
        LOG(ERROR, "Block head is null");
        return false;
    }

    return (mem_block_head_->queue_front == mem_block_head_->queue_tail);
}

inline bool MemoryBlock::is_full() {
    if (mem_block_head_ == NULL) {
        LOG(ERROR, "Block head is null");
        return false;
    }

    if (mem_block_head_->block_unit_num == 0) {
        LOG(ERROR, "Block unit num is 0");
        return false;
    }

    int next_index = (mem_block_head_->queue_front + 1) % mem_block_head_->block_unit_num;
    return (next_index == mem_block_head_->queue_tail);
}

// Basic information of memory blocks in the memory pool
struct MemBlockInfo {
    MemoryBlock block_obj;  // Memory block object
    ULONG block_offset;     // Address offset of the memory block relative to the memory pool
};

class MemoryPool {
 public:
    static MemoryPool& instance(void) {
        static MemoryPool pool;
        return pool;
    }

    ~MemoryPool() {}

    /**
     * @brief   Initialize the memory pool
     * @return  0: Success, -1: Failure
     */
    int init(void);

    /**
     * @brief   Allocate memory blocks of specified type and quantity in the memory pool
     * @param   type: Memory block type
     * @param   size: Size of each unit in the memory block
     * @param   num: Number of units in the memory block
     * @return  0: Success, -1: Failure
     */
    int alloc(MemBlockType type, UINT size, UINT num);

    /**
     * @brief   Get a free memory unit from the specified type of memory block
     * @param   type: Memory block type
     * @param   index: Memory unit index
     * @return  Address of the newly allocated memory unit, NULL if failed
     */
    char* get_free_obj(MemBlockType type, UINT& index);

    /**
     * @brief   Release a memory unit of the specified type of memory block
     * @param   type: Memory block type
     * @param   index: Memory unit index
     * @return  0: Success, -1: Failure
     */
    int release_obj(MemBlockType type, UINT index);

    /**
     * @brief   Get an existing memory unit from the specified type of memory block
     * @param   type: Memory block type
     * @param   index: Memory unit index
     * @return  Address of the memory unit, NULL if failed
     */
    char* get_obj(MemBlockType type, UINT index);

    /**
     * @brief   Get the starting address of the memory pool
     * @return  Starting address of the memory pool
     */
    char* get_mempool_base_attr(void) {
        return mempool_base_attr_;
    }

    /**
     * @brief   Get the header pointer of the player object pool
     * @param   type: Memory block type
     * @return  Header pointer
     */
    MemBlockHead* get_pool_obj_head(MemBlockType type);

 private:
    MemoryPool() {}
    explicit MemoryPool(const MemoryPool&);
    MemoryPool& operator=(const MemoryPool&);

 private:
    ULONG pool_size_;   // Total size of memory pool
    ULONG free_size_;   // Free size of memory pool
    ULONG cur_offset_;  // Current offset position in memory pool
    USHORT block_num_;  // Number of memory blocks
    MemBlockInfo block_info_[EMBU_MAX];  // Memory block information
    char* mempool_base_attr_;  // Starting address of memory pool
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
        LOG(ERROR, "Invalid type, type={0:d}", static_cast<int>(type));
        return NULL;
    }

    return block_info_[type].block_obj.mem_block_head_;
}

#endif  // _TRADING_PLATFORM_COMMON_MEM_MGR_H_