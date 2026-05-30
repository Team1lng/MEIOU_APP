#ifndef CMD_MODULE_H
#define CMD_MODULE_H

/**
 * @description: 调试指令模块初始化
 * @param {unsigned int} CmdModuleMax 支持指令最大各个数
 * @param {char} *CmdShellPath  RShell可执行文件路径
 * @param {char} *CmdLinkPath 指令软连接RShell文件路径
 * @return {*} -1 失败      0 成功
 */
int CmdModuleInit(unsigned int CmdModuleMax, char *CmdShellPath, char *CmdLinkPath);
#endif