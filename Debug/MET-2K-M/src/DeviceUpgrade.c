/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-30 09:01:17
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-11-22 08:09:38
 * @FilePath: /82225-EPC/src/DeviceUpgrade.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "DeviceUpgrade.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define UPGRADE_PACK "/tmp/av100.update"
#define UPGRADE_SCRIPT "/tmp/update.sh"

/**
 * @description: 接收软件更新包
 * @param {int} index    数据包索引
 * @param {int} len    数据包长度
 * @param {char} *buf   更新包数据
 * @return {*}
 */
int ReceiveUpgradePack(int index, int len, unsigned char *buf)
{
    static int Sum = 0;
    if (index == 1)
    {
        Sum = 0;
        printf("Upgrade App Version Time %s  %s\n\r", __DATE__, __TIME__);
        system("rm -rf " UPGRADE_PACK);
        system("touch " UPGRADE_PACK);
        system("chmod 777 " UPGRADE_PACK);
    }
    FILE *Fp = fopen(UPGRADE_PACK, "a+");

    if (Fp == NULL)
    {
        printf("open file error\n");
        return 0;
    }
    int WriteLen = fwrite(buf, sizeof(char), len, Fp);
    if ((++Sum != index) || (WriteLen != len))
    {
        printf("@@@Receive Sum = %d  WriteLen = %d\n@@@packbage_idnex = %d packbage_idnex = %d\n", Sum, WriteLen, index, len);

        Sum = 0;

        fclose(Fp);
        return 0;
    }
    else
    {
        printf("###Receive Sum = %d  WriteLen = %d\n###packbage_idnex = %d packbage_idnex = %d\n", Sum, WriteLen, index, len);

        fclose(Fp);
    }
    return 1;
}

#define UPGRADE_RAR_PATH "image/"
#define UPGRADE_PACKAGE_RAR "image.tar.gz"
#define UPGRADE_PACKAGE_SHA1 "image.sha1"
#define UPGRADE_TMP_PATH "/tmp/"

static char *pack_file[] = {UPGRADE_RAR_PATH, UPGRADE_PACKAGE_RAR, UPGRADE_PACKAGE_SHA1};

bool Decompress_check_upgrade(char *upgrade_file)
{
    printf("1.Decompress check upgrade\n");
    int ret = 0;
    int count = 0;
    char str[128] = {0};
    memset(str, 0, sizeof(str));
    sprintf(str, "tar -xzvf %s -C %s ;rm -f %s", upgrade_file, UPGRADE_TMP_PATH, upgrade_file);
    FILE *pf = popen(str, "r");
    if (pf == NULL)
    {
        perror("Unzip the upgrade file fail!\n\r");
        return count;
    }
    memset(str, 0, sizeof(str));

    while (fgets(str, sizeof(str), pf))
    {
        printf("\t file: %s \n\r", str);
        for (int i = 0; i < sizeof(pack_file) / sizeof(char *); i++)
        {
            if (!(ret & (1 << i)))
            {
                char *p_dir = strstr(str, pack_file[i]);
                if (p_dir)
                {
                    ret |= (1 << i);
                    break;
                }
            }
        }
    }

    /* 获取文件个数 */
    while (ret)
    {
        ret &= (ret - 1);
        count++;
    }

    printf("\t A total of %d upgrade files were found.\n\r", count);
    pclose(pf);
    pf = NULL;
    return count != sizeof(pack_file) / sizeof(char *) ? 0 : count;
}

bool VerifyUpgradeSha1sum(void)
{
    printf("2.Verify upgrade sha1sum\n");
    bool ret = false;
    char str[128] = {0};
    memset(str, 0, sizeof(str));
    sprintf(str, "sha1sum %s%s%s", UPGRADE_TMP_PATH, UPGRADE_RAR_PATH, UPGRADE_PACKAGE_RAR);
    FILE *pf = popen(str, "r");
    if (pf == NULL)
    {
        perror("\t Start file integrity check fail!\n\r");
        return ret;
    }

    memset(str, 0, sizeof(str));
    while (fgets(str, sizeof(str), pf))
    {
        char *p = strchr(str, ' ');
        char hash[128] = {0};
        strncpy(hash, str, p - str);
        printf("\t Current hash: %s \n\r", hash);
        ret = true;
    }
    pclose(pf);
    pf = NULL;

    char correct_hash_buff[255] = {0};
    char file_path[128] = {0};
    sprintf(file_path, "%s%s%s", UPGRADE_TMP_PATH, UPGRADE_RAR_PATH, UPGRADE_PACKAGE_SHA1);
    pf = fopen(file_path, "r");
    memset(file_path, 0, sizeof(file_path));
    fscanf(pf, "%s", correct_hash_buff);
    printf("\t Correct hash: %s \n\r", correct_hash_buff);
    fclose(pf);
    pf = NULL;
    if (strncmp(correct_hash_buff, str, strlen(correct_hash_buff)) == 0)
    {
        ret = true;
        printf("\t Hash check passed!!!\n\r");
    }
    return ret;
}

bool DecompressImageUpgrade(void)
{
    char cmd[512] = {0};
    sprintf(cmd, "tar -zxvf %s%s%s -C %s  ;rm -rf %s%s", UPGRADE_TMP_PATH, UPGRADE_RAR_PATH, UPGRADE_PACKAGE_RAR, UPGRADE_TMP_PATH, UPGRADE_TMP_PATH, UPGRADE_RAR_PATH);

    FILE *pf = popen(cmd, "r");
    if (pf == NULL)
    {
        perror("\t upgrade file fail!\n\r");
        return false;
    }
    else
    {
        printf("\t Unzip successfully and start updating\n\r");
    }
    pclose(pf);
    if (access("/tmp/update.sh", F_OK))
    {
        perror("\t /tmp/update.sh inexistence!\n\r");
        return false;
    }
    system("(sleep 2; /tmp/update.sh) &");
    return true;
}

bool StartUpgradeApp(char *upgrade_file)
{
    if (access(upgrade_file, F_OK) != 0)
    {
        printf("upgrade file inexistence.\n\r");
        return false;
    }
    printf("find %s,start upgrade....\n", upgrade_file);

    /* 1.解压校验升级文件 */
    if (Decompress_check_upgrade(upgrade_file))
    {
        /* 2.校验升级压缩sha1值 */
        if (VerifyUpgradeSha1sum())
        {
            /* 3.解压image压缩包升级 */
            return DecompressImageUpgrade();
        }
    }
    return false;
}

/**
 * @description:开始更新
 * @param {int} Flag    升级结果
 * @return {*}
 */
int UpgradeProcess(int Flag)
{
    if (Flag)
    {
        if (access(UPGRADE_PACK, F_OK) != 0)
        {
            printf("upgrade file inexistence.\n\r");
            return false;
        }
        printf("find %s,start upgrade....\n", UPGRADE_PACK);

        /* 1.解压校验升级文件 */
        if (Decompress_check_upgrade(UPGRADE_PACK))
        {
            /* 2.校验升级压缩sha1值 */
            if (VerifyUpgradeSha1sum())
            {
                /* 3.解压image压缩包升级 */
                return DecompressImageUpgrade();
            }
        }
    }
    printf("Upgrade package is incorrect, cancel upgrade !!!\n");
    system("rm -f /tmp/*");
    return 0;
}
