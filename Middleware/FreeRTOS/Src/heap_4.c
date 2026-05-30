/**
 * @file    heap_4.c
 * @brief   FreeRTOS 内存管理 — Heap 4 (最佳匹配 + 碎片合并)
 *
 * Heap 4 使用首次适应算法分配内存，并自动合并相邻的空闲块。
 * 适用于需要频繁分配/释放不同大小内存块的应用。
 *
 * configTOTAL_HEAP_SIZE 在 FreeRTOSConfig.h 中定义为 24KB。
 * 堆内存由链接器分配在 .bss 段中。
 */

#include "FreeRTOS.h"
#include "task.h"

/* ========================== 常量 ========================== */
#define heapMINIMUM_BLOCK_SIZE  ((size_t)(xHeapStructSize << 1))

/* 块结构必须对齐到 portBYTE_ALIGNMENT 的倍数 */
#define heapBITS_PER_BYTE       ((size_t)8)

/* ========================== 内存块结构 ========================== */

/**
 * @brief  堆内存块链表节点。
 *         每个空闲块在开头包含此结构。
 *         pxNextFreeBlock 指向下一个空闲块 (按地址升序排列)。
 *         xBlockSize 包含块大小 (含结构体本身，bit0 标记是否空闲)。
 */
typedef struct A_BLOCK_LINK {
    struct A_BLOCK_LINK *pxNextFreeBlock;   /* 下一个空闲块    */
    size_t               xBlockSize;         /* 块大小 (bit0=1 表示空闲) */
} BlockLink_t;

/* ========================== 静态变量 ========================== */

/* 堆空间 (由链接器分配在 .bss) */
static uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((aligned(8)));

/* 空闲块链表头 */
static BlockLink_t xStart, *pxEnd = NULL;

/* 已分配字节计数 */
static size_t xFreeBytesRemaining = 0U;

/* 宏: heapSTRUCT_SIZE = sizeof(BlockLink_t) 向上对齐到 portBYTE_ALIGNMENT */
static const size_t xHeapStructSize =
    ((sizeof(BlockLink_t) + (size_t)(portBYTE_ALIGNMENT - 1)) &
     ~((size_t)portBYTE_ALIGNMENT_MASK));

/* ========================== 内部函数 ========================== */

static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert);
static void prvHeapInit(void);

/* 用于简化对齐计算 */
#define heapADJUSTED_HEAP_SIZE  (configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT)

/* ========================== 初始化 ========================== */

/**
 * @brief  初始化堆内存。仅在首次分配时调用一次。
 *         将整个堆区域设置为一个大的空闲块。
 */
static void prvHeapInit(void)
{
    BlockLink_t *pxFirstFreeBlock;
    uint8_t     *pucAlignedHeap;
    size_t       uxAddress;
    size_t       xTotalHeapSize = configTOTAL_HEAP_SIZE;

    /* 对齐堆起始地址到 portBYTE_ALIGNMENT */
    uxAddress = (size_t)ucHeap;

    if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0)
    {
        uxAddress += (portBYTE_ALIGNMENT - 1);
        uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
        xTotalHeapSize -= uxAddress - (size_t)ucHeap;
    }

    pucAlignedHeap = (uint8_t *)uxAddress;

    /* 初始化链表头 */
    xStart.pxNextFreeBlock = (void *)pucAlignedHeap;
    xStart.xBlockSize      = (size_t)0;

    /* 在堆末尾放置 pxEnd 标记 */
    uxAddress  = (size_t)pucAlignedHeap + xTotalHeapSize;
    uxAddress -= xHeapStructSize;
    uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
    pxEnd      = (BlockLink_t *)uxAddress;
    pxEnd->xBlockSize      = 0;
    pxEnd->pxNextFreeBlock = NULL;

    /* 创建第一个空闲块 — 整个堆空间 */
    pxFirstFreeBlock = (void *)pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize      = (size_t)(uxAddress - (size_t)pxFirstFreeBlock);
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

    /* 可用空间 = 总堆 — 块结构大小 */
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize - xHeapStructSize;
}

/* ========================== 内存分配 ========================== */

/**
 * @brief  从堆中分配内存。
 *
 * @param xWantedSize  请求分配的字节数
 * @return             分配的内存指针，失败返回 NULL
 */
void *pvPortMalloc(size_t xWantedSize)
{
    BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
    void        *pvReturn = NULL;

    vTaskSuspendAll();
    {
        /* 首次调用时初始化堆 */
        if (pxEnd == NULL)
        {
            prvHeapInit();
        }

        /* 检查请求是否合理 */
        if ((xWantedSize > 0) &&
            (xWantedSize <= xFreeBytesRemaining))
        {
            /* 所需最小块大小 = 请求大小 + 块结构 */
            xWantedSize += xHeapStructSize;

            /* 确保块大小满足最小要求 */
            if (xWantedSize < heapMINIMUM_BLOCK_SIZE)
            {
                xWantedSize = heapMINIMUM_BLOCK_SIZE;
            }
            else
            {
                /* 对齐到 portBYTE_ALIGNMENT */
                if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00)
                {
                    xWantedSize += (portBYTE_ALIGNMENT -
                                    (xWantedSize & portBYTE_ALIGNMENT_MASK));
                }
            }

            /* 遍历空闲链表，寻找足够大的块 */
            pxPreviousBlock = &xStart;
            pxBlock         = xStart.pxNextFreeBlock;

            while ((pxBlock->xBlockSize < xWantedSize) &&
                   (pxBlock->pxNextFreeBlock != NULL))
            {
                pxPreviousBlock = pxBlock;
                pxBlock         = pxBlock->pxNextFreeBlock;
            }

            /* 检查是否找到足够的块 (非 pxEnd) */
            if (pxBlock != pxEnd)
            {
                /* 返回块数据区: 跳过块结构 */
                pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock) +
                                    xHeapStructSize);

                /* 从空闲链表中移除该块 */
                pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                /* 如果块远大于请求大小，进行分割 */
                if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)
                {
                    /* 在剩余部分创建新的空闲块 */
                    pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);
                    pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                    pxBlock->xBlockSize = xWantedSize;

                    /* 将剩余部分插入空闲链表 */
                    prvInsertBlockIntoFreeList(pxNewBlockLink);
                }

                xFreeBytesRemaining -= pxBlock->xBlockSize;

                /* 标记为已分配 (bit0 = 0) */
                pxBlock->xBlockSize &= ~((size_t)0x01);
            }
        }
    }
    (void)xTaskResumeAll();

    return pvReturn;
}

/* ========================== 内存释放 ========================== */

/**
 * @brief  释放之前分配的内存块。
 *
 * @param pv  要释放的指针 (NULL 时无操作)
 */
void vPortFree(void *pv)
{
    uint8_t     *puc = (uint8_t *)pv;
    BlockLink_t *pxLink;

    if (pv != NULL)
    {
        /* 回退到块结构 */
        puc   -= xHeapStructSize;
        pxLink = (void *)puc;

        vTaskSuspendAll();
        {
            /* 标记为空闲 */
            pxLink->xBlockSize |= (size_t)0x01;

            xFreeBytesRemaining += pxLink->xBlockSize;

            /* 插入空闲链表 (自动合并相邻块) */
            prvInsertBlockIntoFreeList(pxLink);
        }
        (void)xTaskResumeAll();
    }
}

/* ========================== 空闲链表插入 (带合并) ========================== */

/**
 * @brief  将空闲块插入空闲链表，并自动合并相邻的空闲块。
 *
 * @param pxBlockToInsert  要插入的空闲块
 */
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert)
{
    BlockLink_t *pxIterator;
    uint8_t     *puc;

    /* 按地址顺序遍历空闲链表 */
    for (pxIterator = &xStart;
         pxIterator->pxNextFreeBlock < pxBlockToInsert;
         pxIterator = pxIterator->pxNextFreeBlock)
    {
        /* 仅遍历，不操作 */
    }

    /* ---- 与相邻块合并 ---- */

    /* 检查是否与前一个块相邻 */
    puc = (uint8_t *)pxIterator;
    if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert)
    {
        /* 与前一块合并 */
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        /* 不合并，仅插入链表 */
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
        pxIterator->pxNextFreeBlock      = pxBlockToInsert;
    }

    /* 检查是否与后一个块相邻 */
    puc = (uint8_t *)pxBlockToInsert;
    if ((puc + pxBlockToInsert->xBlockSize) ==
        (uint8_t *)pxBlockToInsert->pxNextFreeBlock)
    {
        /* 内存连续 → 合并 */
        pxBlockToInsert->xBlockSize +=
            pxBlockToInsert->pxNextFreeBlock->xBlockSize;
        pxBlockToInsert->pxNextFreeBlock =
            pxBlockToInsert->pxNextFreeBlock->pxNextFreeBlock;
    }
}

/* ========================== 查询接口 ========================== */

/**
 * @brief  获取当前可用堆空间 (字节)
 */
size_t xPortGetFreeHeapSize(void)
{
    return xFreeBytesRemaining;
}

/**
 * @brief  获取历史最小可用堆空间 (字节)
 */
size_t xPortGetMinimumEverFreeHeapSize(void)
{
    /* Heap 4 未实现此功能, 返回当前值 */
    return xFreeBytesRemaining;
}
