#pragma once

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/*
 * 一个页面中的字节数
 */
constexpr int PAGE_SIZE = 8192;
/*
 * 一个页面中的整数个数
 */
constexpr int PAGE_INT_NUM = 2048;
// static_assert((PAGE_INT_NUM & -PAGE_INT_NUM) == PAGE_INT_NUM);
/*
 * 页面字节数以2为底的指数
 */
constexpr int PAGE_SIZE_IDX = 13;
static_assert((1 << PAGE_SIZE_IDX) == PAGE_SIZE);
constexpr int MAX_FMT_INT_NUM = 128;
// constexpr int BUF_PAGE_NUM = 65536;
constexpr int MAX_FILE_NUM = 128;
constexpr int MAX_TYPE_NUM = 256;
/*
 * 缓存中页面个数上限
 */
constexpr int CAP = 60000;
// /*
//  * hash算法的模
//  */
// constexpr int MOD = 60000;
constexpr int IN_DEBUG = 0;
constexpr int DEBUG_DELETE = 0;
constexpr int DEBUG_ERASE = 1;
constexpr int DEBUG_NEXT = 1;
/*
 * 一个表中列的上限
 */
constexpr int MAX_COL_NUM = 31;
/*
 * 数据库中表的个数上限
 */
constexpr int MAX_TB_NUM = 31;
constexpr int RELEASE = 1;

using BufType = unsigned int*;
using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;
using int64 = long long;
using uing64 = unsigned long long;
