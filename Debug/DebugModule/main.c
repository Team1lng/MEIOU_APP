#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "DebugModule.h"

// /* 定义调试模块 */
// #define ModuleList(MODULE)       \
//     MODULE(AI,1)       \
//     MODULE(AO,0)       \
//     MODULE(VENC,1)       \

// #define DefineMod(_Module,En) _Module,
// typedef enum
// {
//     ModuleList(DefineMod)
// } DEBUG_MODULE;

// #define DefineModuleFunc(MODULE,EN) [MODULE].ModuleEnable = EN, [MODULE].ModuleName = #MODULE,
// static DebugModule ModuleGroup[] = {ModuleList(DefineModuleFunc)};

static int ModuleMap[4];

/* 虚函数重载 */
int WeakDebugLog(int Module, DebugLevel level,const char *format, ...)
{
    va_list args;
    va_start(args, format);  // 初始化原始参数列表
    
    // 调用 DebugLog 并传递参数列表
    DebugLogV(Module,level, format, args);
    
    // 清理资源
    va_end(args);
    return 0;
}

// 线程函数：打印当前时间
void* print_time(void* arg) {
    int interval = *(int*)arg; // 获取打印间隔
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];
    
    /* 通过间隔时间映射对应的模块 */
    static char Mstr[4][16] = {0};
    sprintf(Mstr[interval],"M%d",interval);
    ModuleMap[interval] = DebugLogRegister(Mstr[interval],1);

    while (1) {
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        #if 0   /* 效果一致 */
        WeakDebugLog(ModuleMap[interval],DEBUG_NORMAL*/,"线程%d(间隔%d秒): %s", interval, interval, buffer);
        #else
        DebugLog(ModuleMap[interval], DEBUG_NORMAL,"线程%d(间隔%d秒): %s", interval, interval, buffer);
        #endif

        sleep(interval); // 按指定间隔休眠
    }
    
    return NULL;
}

int main() {
    pthread_t tid1, tid2, tid3;
    int intervals[] = {1, 2, 3}; // 三个线程的打印间隔

    /* 初始化调试模块*/
    DebugModuleInit(DEBUG_NORMAL,32, 32, NULL, NULL);
    // DebugModuleRegInit(ModuleGroup,sizeof(ModuleGroup)/sizeof(DebugModule),DEBUG_DEBUG);
    #if 0
    while (1)
    {
        sleep(1);
    }
    #else
    //创建线程1（1秒间隔）
    if (pthread_create(&tid1, NULL, print_time, &intervals[0]) != 0) {
        perror("创建线程1失败");
        return 1;
    }
    
    // 创建线程2（2秒间隔）
    if (pthread_create(&tid2, NULL, print_time, &intervals[1]) != 0) {
        perror("创建线程2失败");
        return 1;
    }
    
    // 创建线程3（3秒间隔）
    if (pthread_create(&tid3, NULL, print_time, &intervals[2]) != 0) {
        perror("创建线程3失败");
        return 1;
    }
    
    // 等待所有线程结束（实际上不会结束，因为线程是无限循环）
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    #endif
    return 0;
}