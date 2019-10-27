#pragma once

#include <defs.h>
#include <unordered_set>

namespace Orange {
    void init();
    bool exists(const String& name);
    bool create(const String& name);
    bool drop(const String& name);
    bool use(const String& name);
    //  正在使用某个数据库
    bool using_db();
    String get_cur();
    std::vector<String> all();
    std::vector<String> all_tables();
    
};
