#include "UserCard.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

static CardInfo UserDeck = {.DeckSize = 0};

/**
 * @description: 保存卡片信息
 * @return {*}
 */
int UserCardSave(void)
{
    int fd = open(CARD_CONFIG_PATH, O_WRONLY | O_CREAT);
    if (fd < 0)
    {
        printf("write open %s fail \n", CARD_CONFIG_PATH);
        return 0;
    }

    write(fd, &UserDeck, sizeof(CardInfo));

    close(fd);
    system("fsync -d " CARD_CONFIG_PATH);
    return 1;
}

/**
 * @description: 获取卡片信息
 * @return {*}
 */
CardInfo *UserCardGet(void)
{
    return &UserDeck;
}

/**
 * @description: 卡组初始化
 * @return {*}
 */
int UserDeckInit(void)
{
    int fd = open(CARD_CONFIG_PATH, O_RDONLY);
    if (fd < 0)
    {
        UserCardSave();
        return 0;
    }

    read(fd, &UserDeck, sizeof(CardInfo));

    close(fd);

    return 1;
}

/**
 * @description:  卡组格式化
 * @return {*}
 */
int UserDeckFormat(void)
{
    memset(&UserDeck, 0, sizeof(CardInfo));
    UserCardSave();
    return 1;
}

/**
 * @description: 添加卡片
 * @param {int} index   卡片存储索引
 * @param {char} *data  卡片数据
 * @return {*}0-失败，否则返回添加索引
 */
int UserCardAdd(int index, char *data, char permissions)
{
    if (index >= DECK_SIZE_MAX || UserDeck.DeckSize >= DECK_SIZE_MAX)
        return -1;

    if (index == -1)
    {
        for (index = 0; index < DECK_SIZE_MAX; index++)
        {
            if (UserDeck.Deck[index].Perm == 0)
            {
                memcpy(UserDeck.Deck[index].Data, data, sizeof(UserDeck.Deck[index].Data));
                memcpy(UserDeck.Deck[index].Code, CARD_INITIAL_CODE, sizeof(UserDeck.Deck[index].Code));
                UserDeck.DeckSize++;
                break;
            }
        }
    }
    else if (UserDeck.Deck[index].Perm != 0)
    {
        if (memcmp(UserDeck.Deck[index].Data, data, sizeof(UserDeck.Deck[index].Data)) != 0)
        {
            return -1;
        }
    }
    else
    {
        memcpy(UserDeck.Deck[index].Data, data, sizeof(UserDeck.Deck[index].Data));
        memcpy(UserDeck.Deck[index].Code, CARD_INITIAL_CODE, sizeof(UserDeck.Deck[index].Code));
        UserDeck.DeckSize++;
    }
    UserDeck.Deck[index].Perm |= permissions;
    UserCardSave();
    return index;
}

/**
 * @description: 删除卡片权限，卡片权限为0时删除卡片
 * @param {int} index   卡片存储索引
 * @return {*}0-失败，1-成功
 */
int UserCardSetPerm(int index, char permissions)
{
    if (index >= DECK_SIZE_MAX)
        return 0;
    if (permissions < 0 || permissions > 3)
        return 0;

    if (UserDeck.Deck[index].Perm == 0)
        return 0;

    if (!(UserDeck.Deck[index].Perm = permissions))
    {
        memset(UserDeck.Deck[index].Data, 0, sizeof(UserDeck.Deck[index].Data));
        UserDeck.DeckSize--;
    }

    UserCardSave();
    return 1;
}

/**
 * @description: 用户卡搜索
 * @param {char} *data  卡片数据
 * @return {*}卡片索引
 */
int UserCardSearch(char *data)
{
    if (!UserDeck.DeckSize)
        return -1;

    int DeckSize = UserDeck.DeckSize;
    for (int i = 0; i < sizeof(UserDeck.Deck) / sizeof(UserCard); i++)
    {
        if (UserDeck.Deck[i].Perm)
        {
            if (memcmp(UserDeck.Deck[i].Data, data, sizeof(UserDeck.Deck[i].Data)) == 0)
            {
                return i;
            }

            if (!(--DeckSize))
                break;
        }
    }
    return -1;
}

/**
 * @description: 卡片密码校验
 * @param {int} Index   卡片索引
 * @param {char} *Code  卡片密码
 * @param {int} CodeLen 密码长度
 * @return {*}0-失败，x-权限
 */
int CardCodeVerify(int Index, char *Code, int CodeLen)
{
    if (CodeLen != sizeof(UserDeck.Deck[0].Code))
        return 0;

    if (memcmp(UserDeck.Deck[Index].Code, Code, CodeLen) == 0)
    {
        return UserDeck.Deck[Index].Perm;
    }
    return 0;
}

/**
 * @description: 搜索密码权限
 * @param {char} *Code  密码
 * @param {int} CodeLen 密码长度
 * @return {*}0-失败，x-权限
 */
int CardCodePermission(char *Code, int CodeLen)
{
    if (CodeLen != sizeof(UserDeck.Deck[0].Code))
        return 0;
    if (memcmp(Code, CARD_INITIAL_CODE, strlen(CARD_INITIAL_CODE)) == 0)
        return 0;

    int DeckSize = UserDeck.DeckSize;
    for (int i = 0; i < sizeof(UserDeck.Deck) / sizeof(UserCard); i++)
    {
        if (UserDeck.Deck[i].Perm)
        {
            if (memcmp(UserDeck.Deck[i].Code, Code, sizeof(UserDeck.Deck[i].Code)) == 0)
            {
                return UserDeck.Deck[i].Perm;
            }

            if (!(--DeckSize))
                break;
        }
    }
    return 0;
}

/**
 * @description: 获取卡组索引权限
 * @param {char} Index 索引
 * @return {*}0-空，x-权限
 */
char DeckIndexPerm(int Index)
{
    if (Index >= DECK_SIZE_MAX)
        return 0;
    return UserDeck.Deck[Index].Perm;
}

/**
 * @description: 获取卡组权限及卡号
 * @param {char} *Deck  卡组缓存
 * @return {*} 卡组数据大小
 */
int UserDeckPermGet(unsigned char **Deck)
{
    unsigned char Buffer[DECK_SIZE_MAX][6];
    static unsigned char DeckData[1280];
    memset(DeckData, 0, sizeof(DeckData));
    DeckData[0] = UserDeck.DeckSize;
    for (int i = 0; i < DECK_SIZE_MAX; i++)
    {
        memcpy(&Buffer[i], &UserDeck.Deck[i], 6);
    }
    memcpy(&DeckData[1], Buffer, sizeof(Buffer));
    *Deck = DeckData;
    // printf("DeckData[%p]\n", DeckData);
    return sizeof(DeckData);
}