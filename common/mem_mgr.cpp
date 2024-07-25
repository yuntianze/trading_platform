#include "mem_mgr.h"
#include "shm_mgr.h"

int MemoryPool::init(void) {
    int shm_key = 111;
    int shm_size = 128;
    int assign_size = shm_size;
    mempool_base_attr_ = static_cast<char*>(ShmMgr::instance().create_shm(shm_key, shm_size, assign_size));
    if (mempool_base_attr_ == NULL) {
        Logger::log(ERROR, "Create shm error, key={0:d}, size={1:d}", shm_key, shm_size);
        return -1;
    }

    pool_size_ = shm_size;
    free_size_ = pool_size_;
    block_num_ = 0;
    cur_offset_ = 0;

    Logger::log(INFO, "Memory pool init ok: size={0:d}, blocknum={1:d}, offset={2:d}", pool_size_, block_num_, cur_offset_);

    return 0;
}

int MemoryPool::alloc(MemBlockType type, UINT size, UINT num) {
    if (type >= BLOCK_MAX || type < 0) {
        Logger::log(ERROR, "Invalid block type: type={0:d}", static_cast<int>(type));
        return -1;
    }

    ULONG need_size = sizeof(MemBlockHead) + (sizeof(UINT) + sizeof(char) + size)*num;
    if (need_size > free_size_) {
        Logger::log(ERROR, "Not enough memory in pool: need={0:d}, free={1:d}", need_size, free_size_);
        return -1;
    }

    if (block_info_[type].block_obj.init(type, size, num, cur_offset_) != 0) {
        return -1;
    }

    block_info_[type].block_offset = cur_offset_;
    cur_offset_ += need_size;
    free_size_ -= need_size;

    return 0;
}

int MemoryBlock::init(MemBlockType type, UINT size, UINT num, ULONG offset) {
    char* mem_base_attr = MemoryPool::instance().get_mempool_base_attr();
    mem_base_attr += offset;

    mem_block_head_ = reinterpret_cast<MemBlockHead*>(mem_base_attr);
    if (mem_block_head_ == NULL) {
        Logger::log(ERROR, "Memory block head is null");
        return -1;
    }

    // Distinguish startup mode
    int shm_key = 111;
    ShmMode mode = ShmMgr::instance().get_shm_mode(shm_key);

    // Unit index array follows immediately after the memory block header node
    mem_block_head_->unit_index = reinterpret_cast<UINT*>(mem_base_attr + sizeof(MemBlockHead));
    // Memory block usage flag array
    mem_block_head_->unit_used_flag = mem_base_attr + sizeof(MemBlockHead) + sizeof(UINT)*num;
    // Starting address for data storage
    mem_block_head_->data = mem_base_attr + sizeof(MemBlockHead) + (sizeof(UINT) + sizeof(char))*num;

    if (mode == MODE_INIT) {
        // Fill in basic information of memory block header
        mem_block_head_->block_type = type;
        mem_block_head_->block_unit_size = size;
        mem_block_head_->block_unit_num = num;
        mem_block_head_->offset = offset;
        mem_block_head_->used_num = 0;
        mem_block_head_->queue_front = 0;
        mem_block_head_->queue_tail = 0;

        // Initialize memory block index array, used as unique identifiers for future allocated objects
        for (UINT i = 0; i < num; ++i) {
            mem_block_head_->unit_index[i] = i;
        }

        // Initialize memory block usage flag array
        memset(mem_block_head_->unit_used_flag, EMBU_FREE, num);
    } else {
        // Necessary checks
        if (mem_block_head_->block_unit_size != size || mem_block_head_->block_unit_num != num
            || mem_block_head_->block_type != type) {
            Logger::log(ERROR, "Invalid shared memory, unitsize={0:d}, num={1:d}, type={2:d}",
                    mem_block_head_->block_unit_size, mem_block_head_->block_unit_num,
                    static_cast<int>(mem_block_head_->block_type));
            return -1;
        }
    }

    Logger::log(INFO, "Init memory pool, offset={0:d}, head={1:d}, mode={2:d}", offset, sizeof(MemBlockHead),
            static_cast<int>(mode));

    return 0;
}

char* MemoryBlock::get_free_obj(UINT& mem_unit_index) {
    if (mem_block_head_ == NULL) {
        Logger::log(ERROR, "Block head is null");
        return NULL;
    }

    // Check if the memory block queue is full
    if (is_full()) {
        Logger::log(ERROR, "Memory Block is full: totalNum={0:d}, usedNum={1:d}",
                mem_block_head_->block_unit_num, mem_block_head_->used_num);
        return NULL;
    }

    // Calculate the queue position to store the index number of the currently allocated object
    mem_block_head_->queue_front = (mem_block_head_->queue_front + 1) % mem_block_head_->block_unit_num;

    // Get the index number of the memory object to be allocated
    mem_unit_index = mem_block_head_->unit_index[mem_block_head_->queue_front];

    // Update memory block usage flag
    mem_block_head_->unit_used_flag[mem_unit_index] = EMBU_USED;
    ++mem_block_head_->used_num;

    static UINT s_uiCount = 0;
    ++s_uiCount;

    Logger::log(INFO, "Get free object ok, type={0:d}, index={1:d}, front={2:d}, usednum={3:d}, totalFreeObj={4:d}",
            static_cast<int>(mem_block_head_->block_type),
            mem_unit_index, mem_block_head_->queue_front, mem_block_head_->used_num, s_uiCount);

    return mem_block_head_->data + mem_unit_index*mem_block_head_->block_unit_size;
}

int MemoryBlock::release_obj(UINT mem_unit_index) {
    if (mem_block_head_ == NULL) {
        Logger::log(ERROR, "Block head is null");
        return -1;
    }

    if (mem_unit_index >= mem_block_head_->block_unit_num) {
        Logger::log(ERROR, "Memory Unit Index beyond bound: curIndex={0:d}, maxIndex={1:d}",
                mem_unit_index, mem_block_head_->block_unit_num);
        return -1;
    }

    // Check if the memory block queue is empty
    if (is_empty()) {
        Logger::log(ERROR, "Memory Block is empty!");
        return -1;
    }

    // Calculate the queue position to store the index number of the currently released object
    mem_block_head_->unit_index[mem_block_head_->queue_tail] = mem_unit_index;
    mem_block_head_->queue_tail = (mem_block_head_->queue_tail + 1) % mem_block_head_->block_unit_num;
    mem_block_head_->unit_used_flag[mem_unit_index] = EMBU_FREE;
    --mem_block_head_->used_num;

    static UINT s_uiCount = 0;
    ++s_uiCount;

    Logger::log(INFO, "Release object ok, type={0:d}, index={1:d}, tail={2:d}, totalReleaseObj={3:d}",
        static_cast<int>(mem_block_head_->block_type), mem_unit_index, mem_block_head_->queue_tail, s_uiCount);

    return 0;
}

char* MemoryBlock::get_obj(UINT mem_unit_index) {
    if (mem_block_head_ == NULL) {
        Logger::log(ERROR, "Block head is null");
        return NULL;
    }

    if (mem_unit_index >= mem_block_head_->block_unit_num) {
        Logger::log(ERROR, "Memory Unit Index beyond bound: curIndex={0:d}, maxIndex={1:d}",
                mem_unit_index, mem_block_head_->block_unit_num);
        return NULL;
    }

    if (mem_block_head_->unit_used_flag[mem_unit_index] != EMBU_USED) {
        Logger::log(ERROR, "Memory Block use flag error: flag={0:d}, index={1:d}",
                mem_block_head_->unit_used_flag[mem_unit_index], mem_unit_index);
        return NULL;
    }

    return mem_block_head_->data + mem_unit_index*mem_block_head_->block_unit_size;
}