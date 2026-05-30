/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-26 19:52:36
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-09-15 16:26:18
 * @FilePath: /82225-EPC/src/UserCard.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _USER_CARD_H_
#define _USER_CARD_H_

#define CARD_CONFIG_PATH "/etc/config/UserCard.cfg"
#define CARD_DATA_LEN 4
#define CARD_CODE_LEN 4
#define DECK_SIZE_MAX 200
#define CARD_INITIAL_CODE "0000"

#define LOCK_PERM (1 << 0)
#define GATE_PERM (1 << 1)

typedef struct
{
    char Perm;
    char Data[CARD_DATA_LEN];
    char Code[CARD_CODE_LEN];
} UserCard;

typedef struct
{
    char DeckSize;
    UserCard Deck[DECK_SIZE_MAX];
} CardInfo;

/**
 * @description: 保存卡片信息
 * @return {*}
 */
int UserCardSave(void);

/**
 * @description: 获取卡片信息
 * @return {*}
 */
CardInfo *UserCardGet(void);

/**
 * @description: 卡组初始化
 * @return {*}
 */
int UserDeckInit(void);

/**
 * @description:  卡组格式化
 * @return {*}
 */
int UserDeckFormat(void);

/**
 * @description: 添加卡片
 * @param {int} index   卡片存储索引
 * @param {char} *data  卡片数据
 * @return {*}0-失败，1-成功
 */
int UserCardAdd(int index, char *data, char permissions);

/**
 * @description: 删除卡片权限，卡片权限为0时删除卡片
 * @param {int} index   x-卡片存储索引
 * @return {*}0-失败，1-成功
 */
int UserCardSetPerm(int index, char permissions);

/**
 * @description: 用户卡搜索
 * @param {char} *data  卡片数据
 * @return {*}-1 - 失败，x - 卡片索引
 */
int UserCardSearch(char *data);

/**
 * @description: 卡片密码校验
 * @param {int} Index   卡片索引
 * @param {char} *Code  卡片密码
 * @param {int} CodeLen 密码长度
 * @return {*}0-失败，x-权限
 */
int CardCodeVerify(int Index, char *Code, int CodeLen);

/**
 * @description: 搜索密码权限
 * @param {char} *Code  密码
 * @param {int} CodeLen 密码长度
 * @return {*}0-失败，x-权限
 */
int CardCodePermission(char *Code, int CodeLen);

/**
 * @description: 获取卡组索引权限
 * @param {char} Index 索引
 * @return {*}0-空，x-权限
 */
char DeckIndexPerm(int Index);

/**
 * @description: 获取卡组权限及卡号
 * @param {char} *Deck  卡组指针
 * @return {*}
 */
int UserDeckPermGet(unsigned char **Deck);
#endif