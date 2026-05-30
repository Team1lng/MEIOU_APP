/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-13 13:47:42
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-18 08:58:53
 * @FilePath: /project_3/common/CircularList.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _CIRCULAR_LIST_H_
#define _CIRCULAR_LIST_H_
#include "List.h"
#include <unistd.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <pthread.h>

#define ASSERT(condition, message)                                                         \
    do                                                                                     \
    {                                                                                      \
        if (!(condition))                                                                  \
        {                                                                                  \
            if (message)                                                                   \
            {                                                                              \
                fprintf(stderr, "Assertion failed: %s\nFile: %s\nLine: %d\nMessage: %s\n", \
                        #condition, __FILE__, __LINE__, message);                          \
            }                                                                              \
            abort();                                                                       \
        }                                                                                  \
    } while (0)

#define LIST_NODE_MAX 10
typedef struct
{
    void *Data;
    struct list_head Ptr;
} ListNode;

typedef struct
{
    atomic_int Inited;         /* 缓冲区初始化标志（0=未初始化，1=已初始化） */
    atomic_int WriteIndex;     /* 生产者写入位置索引（原子操作保证线程安全） */
    atomic_int ReadIndex;      /* 消费者读取位置索引（原子操作保证线程安全） */
    atomic_int RequestIndex;   /* 特殊请求索引（如预读取位置/特殊操作标记） */
    atomic_int RecvBlock;      /* 接收阻塞标志（0=非阻塞，1=阻塞模式） */
    pthread_t ListLockId;      /* 当前持有锁的线程ID（用于锁的持有者检查） */
    struct list_head Head;     /* Linux内核风格的双向链表头（管理节点） */
    ListNode NodeList[LIST_NODE_MAX];   /* 固定大小的节点存储池（预分配内存） */
    sem_t Sem;                 /* 线程同步信号量（控制生产/消费的等待/唤醒） */
    pthread_mutex_t Mutex;     /* 保护链表操作的互斥锁（防止并发冲突） */
    const char *ListName;      /* 缓冲区名称（用于调试和日志标识） */
} CircularList;

#define CIRCULAR_LIST_INIT(LIST) { \
    .Inited = 0,                   \
    .WriteIndex = 0,               \
    .ReadIndex = 0,                \
    .RequestIndex = 0,             \
    .ListName = #LIST,             \
};

#define DECLARE_LIST(LIST) \
    static CircularList LIST = CIRCULAR_LIST_INIT(LIST);

/**
 * @description: 创建环形队列,建议事先使用DECLARE_LIST宏定义列表
 * @param {CircularList} *List  队列头指针
 * @return {*}
 */
int CreateCircularList(CircularList *List);

/**
 * @description: 设置环形队列接收阻塞状态
 * @param {CircularList} *List
 * @return {*}
 */
int CircularListRecvBolck(CircularList *List, int Block);

/**
 * @description: 数据写入环形队列
 * @param {CircularList} *List  队列头指针
 * @param {void} *Node  写入数据
 * @return {*}
 */
int CircularListWrite(CircularList *List, ListNode *Node);

/**
 * @description: 读取环形队列数据
 * @param {CircularList} *List  队列头指针
 * @param {void} *Node  读出数据缓存
 * @return {*}
 */
int CircularListRead(CircularList *List, void **Data);

/**
 * @description:    环形队列为空
 * @param {CircularList} *List  队列头指针
 * @return {*}
 */
int CircularListEmpty(CircularList *List);

/**
 * @description:    申请队列节点
 * @param {CircularList} *List  队列头指针
 * @return {*}
 */
ListNode *CircularListRequest(CircularList *List);

/**
 * @description: 环形上锁
 * @return {*}
 */
int CircularListLock(CircularList *List);

/**
 * @description: 环形解锁
 * @return {*}
 */
int CircularListUnlock(CircularList *List);
#endif