#pragma once

#include <string>
#include <cstring>
#include <filesystem>
#include <type_traits>

static_assert(sizeof(std::size_t) == 8, "x64 only");

namespace fs = std::filesystem;

using rid_t = uint64_t;
using String = std::string;

using byte_t = uint8_t;
using bytes_t = byte_t*;
using const_bytes_t = const byte_t*;
using byte_arr_t = std::vector<byte_t>;
using rec_t = std::vector<byte_arr_t>;

struct page_t {
    int file_id, page_id;
};

struct buf_t {
    bytes_t bytes = nullptr;
    int buf_id;
};

const int MAX_DB_NUM = 5;
const int MAX_TBL_NUM = 12;
const int MAX_COL_NUM = 20;
// 最多同时打开的文件数目
const int MAX_FILE_NUM = MAX_TBL_NUM * (2 * MAX_COL_NUM + 3);

#include <iostream>

constexpr const char* RESET = "\033[0m";
constexpr const char* RED = "\033[31m";   /* Red */
constexpr const char* GREEN = "\033[32m"; /* Green */

void ensure(bool cond, const String& msg);

#ifdef __GNUC__
#include <unistd.h>
#elif _MSC_VER
#include <io.h>
#endif

constexpr int PAGE_SIZE = 8192;
constexpr int PAGE_SIZE_IDX = 13;
static_assert((1 << PAGE_SIZE_IDX) == PAGE_SIZE);

constexpr int BUF_CAP = 60000;
constexpr int IN_DEBUG = 0;
constexpr int DEBUG_DELETE = 0;
constexpr int DEBUG_ERASE = 1;
constexpr int DEBUG_NEXT = 1;

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;
using int64 = long long;
using uint64 = unsigned long long;
using uint8 = uint8_t;

// 没法重载赋值 /cy
const int F_KEY_NAME_LIM = 32;
struct f_key_name_t {
    char data[F_KEY_NAME_LIM + 1];
};
const int COL_NAME_LIM = 32;
struct col_name_t {
    char data[COL_NAME_LIM + 1];
    inline String get() const { return String(data); }
};
const int TBL_NAME_LIM = 32;
struct tbl_name_t {
    char data[TBL_NAME_LIM + 1];
    // tbl_name_t(const String& name) {
    //     memcpy(data, name.data(), name.size());
    //     data[name.size()] = 0;
    // }
    inline String get() { return String(data); }
};
const int COL_NAME_LIST_LIM = 5;
struct col_name_list_t {
    col_name_t data[COL_NAME_LIST_LIM];
    int size = 0;

    void add(col_name_t name) {
        if (size == COL_NAME_LIST_LIM) {
            throw "increase your constant";
        }
        data[size++] = name;
    }
};

template <typename>
struct is_std_vector : std::false_type {};
template <typename T>
struct is_std_vector<std::vector<T>> : std::true_type {};
template <typename T>
constexpr bool is_std_vector_v = is_std_vector<std::remove_cv_t<std::remove_reference_t<T>>>::value;

constexpr int MAX_CHAR_LEN = 256;

#define DATA_NULL 0x0
#define DATA_NORMAL 0x1
#define DATA_INVALID 0xff

#define UNIMPLEMENTED throw "unimplemented";

using cnt_t = int;
template <class Fn, class... Args>
int expand(Fn&& func, Args&&... args) {
    int arr[]{(func(std::forward<Args&&>(args)), 0)...};
    return sizeof(arr) / sizeof(int);
}


struct WhereClause {
    enum cmp_t {
        EQ,
        LT,
        GT,
        LE,
        GE
    };

    String col_name;
    cmp_t cmp;
    byte_arr_t val;

    bool test(const_bytes_t val) const {
        // ensure(val.size() == this->val.size(), "ni chuan de sha dong xi");
        auto code = strncmp((char*)val, (char*)this->val.data(), this->val.size());
        switch (cmp) {
            case EQ: return code == 0;
            case LT: return code < 0;
            case GT: return code > 0;
            case LE: return code <= 0;
            case GE: return code >= 0;
        }
    }
};

class OrangeException : public std::exception {
private:
    String msg;
public:
    OrangeException(const String& msg) : msg(msg) {}
    const char* what() { return msg.c_str(); }
};
