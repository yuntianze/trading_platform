#include "mem_mgr.h"
#include "shm_mgr.h"


int MemoryPool::init(void) {
    int shm_key = 111;
    int shm_size = 128;
    int assign_size = shm_size;
    mempool_base_attr_ = static_cast<char*>(ShmMgr::instance().create_shm(shm_key, shm_size, assign_size));
    if (NULL == mempool_base_attr_) {
        Logger::log(ERROR, "create shm error, key={0:d}, size={0:d}", shm_key, shm_size);
        return -1;
    }

    pool_size_ = shm_size;
    free_size_ = pool_size_;
    block_num_ = 0;
    cur_offset_ = 0;

    Logger::log(INFO, "mempool init ok: size={0:d}, blocknum={0:d}, offset={0:d}", pool_size_, block_num_, cur_offset_);

    return 0;
}

int MemoryPool::alloc(MemBlockType type, UINT size, UINT num) {
    if (type >= BLOCK_MAX || type < 0) {
        Logger::log(ERROR, "invalid block type: type={0:d}", static_cast<int>(type));
        return -1;
    }

    ULONG need_size = sizeof(MemBlockHead) + (sizeof(UINT) + sizeof(char) + size)*num;
    if (need_size > free_size_) {
        Logger::log(ERROR, "mempool not enough mem: need={0:d}, free={0:d}", need_size, free_size_);
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
    if (NULL == mem_block_head_) {
        Logger::log(ERROR, "mem_block_head is null");
        return -1;
    }

    // 需要区分启动模式
    int shm_key = 111;
    ShmMode mode = ShmMgr::instance().get_shm_mode(shm_key);

    // 索引数组紧随内存块头结点之后
    mem_block_head_->unit_index = reinterpret_cast<UINT*>(mem_base_attr + sizeof(MemBlockHead));
    // 内存块已使用标识数组
    mem_block_head_->unit_used_flag = mem_base_attr + sizeof(MemBlockHead) + sizeof(UINT)*num;
    // 存放数据的起始地址
    mem_block_head_->data = mem_base_attr + sizeof(MemBlockHead) + (sizeof(UINT) + sizeof(char))*num;

    if (MODE_INIT == mode) {
        // 填充内存块头的基本信息
        mem_block_head_->block_type = type;
        mem_block_head_->block_unit_size = size;
        mem_block_head_->block_unit_num = num;
        mem_block_head_->offset = offset;
        mem_block_head_->used_num = 0;
        mem_block_head_->queue_front = 0;
        mem_block_head_->queue_tail = 0;

        // 初始化内存块索引数组，用作将来待分配对象的唯一标识
        for (UINT i = 0; i < num; ++i) {
            mem_block_head_->unit_index[i] = i;
        }

        // 初始化内存块已使用标识数组
        memset(mem_block_head_->unit_used_flag, EMBU_FREE, num);
    } else {
        // 必要的检查
        if (mem_block_head_->block_unit_size != size || mem_block_head_->block_unit_num != num
            || mem_block_head_->block_type != type) {
            Logger::log(ERROR, "invalid shmmem, unitsize={0:d}, num={0:d}, type={0:d}",
                    mem_block_head_->block_unit_size, mem_block_head_->block_unit_num,
                    static_cast<int>(mem_block_head_->block_type));
            return -1;
        }
    }

    Logger::log(INFO, "init mempool, offset={0:d}, head={0:d}, mode={0:d}", offset, sizeof(MemBlockHead),
            static_cast<int>(mode));

    return 0;
}

char* MemoryBlock::get_free_obj(UINT& mem_unit_index) {
    if (NULL == mem_block_head_) {
        Logger::log(ERROR, "block head is null");
        return NULL;
    }

    // 判定内存块队列是否已满
    if (is_full()) {
        Logger::log(ERROR, "MemBlock is full: totolNum={0:d}, usedNum={0:d}",
                mem_block_head_->block_unit_num, mem_block_head_->used_num);
        return NULL;
    }

    // 计算存放当前分配对象索引号的队列位置
    mem_block_head_->queue_front = (mem_block_head_->queue_front + 1) % mem_block_head_->block_unit_num;

    // 获取待分配内存对象的索引号
    mem_unit_index = mem_block_head_->unit_index[mem_block_head_->queue_front];

    // 更新内存块已使用标识
    mem_block_head_->unit_used_flag[mem_unit_index] = EMBU_USED;
    ++mem_block_head_->used_num;

    static UINT s_uiCount = 0;
    ++s_uiCount;

    Logger::log(INFO, "get free obj ok, type={0:d}, index={0:d}, front={0:d}, usednum={0:d}, totalFreeObj={0:d}",
            static_cast<int>(mem_block_head_->block_type),
            mem_unit_index, mem_block_head_->queue_front, mem_block_head_->used_num, s_uiCount);

    return mem_block_head_->data + mem_unit_index*mem_block_head_->block_unit_size;
}

int MemoryBlock::release_obj(UINT mem_unit_index) {
    if (NULL == mem_block_head_) {
        Logger::log(ERROR, "block head is null");
        return -1;
    }

    if (mem_unit_index >= mem_block_head_->block_unit_num) {
        Logger::log(ERROR, "MemUnitIndex beyond bound: curIndex={0:d}, maxIndex={0:d}",
                mem_unit_index, mem_block_head_->block_unit_num);
        return -1;
    }

    // 判定内存块队列是否为空
    if (is_empty()) {
        Logger::log(ERROR, "MemBlock is empty!");
        return -1;
    }

    // 计算存放当前释放对象索引号的队列位置
    mem_block_head_->unit_index[mem_block_head_->queue_tail] = mem_unit_index;
    mem_block_head_->queue_tail = (mem_block_head_->queue_tail + 1) % mem_block_head_->block_unit_num;
    mem_block_head_->unit_used_flag[mem_unit_index] = EMBU_FREE;
    --mem_block_head_->used_num;

    static UINT s_uiCount = 0;
    ++s_uiCount;

    Logger::log(INFO, "release obj ok, type={0:d}, index={0:d}, tail={0:d}, totalReleaseObj={0:d}",
        static_cast<int>(mem_block_head_->block_type), mem_unit_index, mem_block_head_->queue_tail, s_uiCount);

    return 0;
}

char* MemoryBlock::get_obj(UINT mem_unit_index) {
    if (NULL == mem_block_head_) {
        Logger::log(ERROR, "block head is null");
        return NULL;
    }

    if (mem_unit_index >= mem_block_head_->block_unit_num) {
        Logger::log(ERROR, "MemUnitIndex beyond bound: curIndex={0:d}, maxIndex={0:d}",
                mem_unit_index, mem_block_head_->block_unit_num);
        return NULL;
    }

    if (mem_block_head_->unit_used_flag[mem_unit_index] != EMBU_USED) {
        Logger::log(ERROR, "MemBlock use flag error: flag={0:d}, index={0:d}",
                mem_block_head_->unit_used_flag[mem_unit_index], mem_unit_index);
        return NULL;
    }

    return mem_block_head_->data + mem_unit_index*mem_block_head_->block_unit_size;
}

