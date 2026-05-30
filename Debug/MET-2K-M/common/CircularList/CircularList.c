/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-12-13 13:48:15
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-22 13:44:45
 * @FilePath: /project_3/common/CircularList.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "CircularList.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/**
 * @description: 创建环形队列,建议事先使用DECLARE_LIST宏定义列表
 * @param {CircularList} *List  队列头指针
 * @return {*}
 */
int CreateCircularList(CircularList *List)
{
    assert(List != NULL);
    if (atomic_load(&List->Inited) == 1)
    {
        return 0;
    }

    List->Inited = ATOMIC_VAR_INIT(1);
    List->RequestIndex = ATOMIC_VAR_INIT(-1);
    List->WriteIndex = ATOMIC_VAR_INIT(0);
    List->ReadIndex = ATOMIC_VAR_INIT(0);
    List->ListLockId = ATOMIC_VAR_INIT(-1);
    List->RecvBlock = ATOMIC_VAR_INIT(1);
    sem_init(&List->Sem, 0, 0);
    INIT_LIST_HEAD(&List->Head);
    pthread_mutex_init(&List->Mutex, NULL);
    memset(List->NodeList, 0, sizeof(ListNode) * LIST_NODE_MAX);

    if (List->ListName)
    {
        printf("\033[0;32;40m[%s]\033[0m%s Init Succeed!!!!\n\n", __func__, List->ListName);
    }

    return 0;
}

/**
 * @description: 设置环形队列接收阻塞状态
 * @param {CircularList} *List
 * @return {*}
 */
int CircularListRecvBolck(CircularList *List, int Block)
{
    assert(List != NULL);
    if (atomic_load(&List->Inited) != 1)
    {
        return -1;
    }
    atomic_store(&List->RecvBlock, Block);
    return 0;
}

/**
 * @description: 写入环形队列
 * @param {CircularList} *List  队列头指针
 * @param {void} *Data  写入数据
 * @return {*}
 */
int CircularListWrite(CircularList *List, ListNode *Node)
{
    assert(List != NULL);
    assert(Node != NULL);
    assert(atomic_load(&List->Inited) == 1);

    pthread_mutex_lock(&List->Mutex);
    /* 缓冲区满检查 */
    if (((List->WriteIndex + 1) % LIST_NODE_MAX == List->ReadIndex) || (Node != &List->NodeList[List->RequestIndex]))
    {
        pthread_mutex_unlock(&List->Mutex);
        return -1;  // 缓冲区满或节点不匹配
    }
    List->RequestIndex = -1;
    list_add_tail(&(Node->Ptr), &(List->Head));  // 添加到链表尾部
    List->WriteIndex = (List->WriteIndex + 1) % LIST_NODE_MAX;
    pthread_mutex_unlock(&List->Mutex);
    sem_post(&List->Sem);
    return 0;
}

/**
 * @description: 读取环形队列节点
 * @param {CircularList} *List  队列头指针
 * @param {void} *Data  读出数据缓存
 * @return {*}
 */
int CircularListRead(CircularList *List, void **Data)
{
    ASSERT(List != NULL, List->ListName);
    ASSERT(atomic_load(&List->Inited) == 1, List->ListName);
    if (atomic_load(&List->RecvBlock))  /* 如果RecvBlock为真（非0），则使用sem_wait进行阻塞等待，直到有数据可用 */
    {
        sem_wait(&List->Sem);
    }
    else   /* 否则，使用sem_timedwait进行带超时（30毫秒）的等待，如果超时则返回-1 */
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 30 * 1000 * 1000; // 设置超时时间为 30ms后
        if (sem_timedwait(&List->Sem, &ts) != 0)
        {
            return -1;
        }
    }

    pthread_mutex_lock(&List->Mutex);  /* 获取到信号量后，加互斥锁（Mutex）保护临界区 */
    if (list_empty(&List->Head))  /* 再次检查链表是否为空 */
    {
        pthread_mutex_unlock(&List->Mutex);
        return -1;
    }

    ListNode *Node = list_first_entry(&(List->Head), ListNode, Ptr);  /* 获取并移除第一个节点 */
    list_del(&(Node->Ptr));
    *Data = Node->Data;
    List->ReadIndex = (List->ReadIndex + 1) % LIST_NODE_MAX; /* 更新读索引 */
    pthread_mutex_unlock(&List->Mutex);
    return 0;
}

/**
 * @description:    环形队列为空
 * @param {CircularList} *List  队列头指针
 * @return {*}
 */
int CircularListEmpty(CircularList *List)
{
    int result = 0;
    pthread_mutex_lock(&List->Mutex);
    result = list_empty(&List->Head);
    pthread_mutex_unlock(&List->Mutex);
    return result;
}

/**
 * @description:    申请队列节点
 * @param {CircularList} *List  队列头指针
 * @return {*}
 */
ListNode *CircularListRequest(CircularList *List)
{
    // assert(List != NULL);
    // assert(List->Inited == 1);
    if (List == NULL)
        return NULL;

    if(atomic_load(&List->Inited) != 1)
    {
        CreateCircularList(List);
    }

    pthread_mutex_lock(&List->Mutex);
    if (List->RequestIndex != -1 || (List->WriteIndex + 1) % LIST_NODE_MAX == List->ReadIndex)
    {
        pthread_mutex_unlock(&List->Mutex);
        return NULL;
    }

    if (List->ListLockId != -1 && List->ListLockId != pthread_self())
    {
        pthread_mutex_unlock(&List->Mutex);
        return NULL;
    }

    List->RequestIndex = List->WriteIndex;
    pthread_mutex_unlock(&List->Mutex);
    return &List->NodeList[atomic_load(&List->WriteIndex)];
}

/**
 * @description: 环形上锁
 * @return {*}
 */
int CircularListLock(CircularList *List)
{
    pthread_mutex_lock(&List->Mutex);
    if (List->ListLockId != -1)
    {
        pthread_mutex_unlock(&List->Mutex);
        return -1;
    }
    List->ListLockId = pthread_self();
    pthread_mutex_unlock(&List->Mutex);
    return atomic_load(&List->ListLockId);
}

/**
 * @description: 环形解锁
 * @return {*}
 */
int CircularListUnlock(CircularList *List)
{
    pthread_mutex_lock(&List->Mutex);
    if (List->ListLockId == -1 || List->ListLockId != pthread_self())
    {
        pthread_mutex_unlock(&List->Mutex);
        return -1;
    }
    List->ListLockId = -1;
    pthread_mutex_unlock(&List->Mutex);
    return 0;
}
