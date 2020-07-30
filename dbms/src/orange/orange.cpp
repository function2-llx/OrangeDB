#include "orange/orange.h"
#include "orange/table/table.h"
#include "exceptions.h"

#include <unordered_set>
#include <map>

// database names cache
static std::unordered_set<String> names;
// current using database name, empty if not using any
static std::map<int, String> cur;
const fs::path DB_ROOT;


namespace Orange {
    bool exists(const String& name) { return names.count(name); }

    bool create(const String& name) {
        orange_check(!exists(name), Exception::database_exists(name));
        names.insert(name);
        auto ret = fs::create_directory(DB_ROOT / fs::path(name));
        return ret;
    }

    bool drop(const String& name) {
        orange_check(exists(name), Exception::database_not_exist(name));
        names.erase(name);
        if (name == cur[cur_user_id]) {
            unuse();
        }
        return fs::remove_all(DB_ROOT / fs::path(name));
    }

    bool use(const String& name) {
        if (name == cur[cur_user_id]) return true;
        unuse();
        orange_check(exists(name), Exception::database_not_exist(name));
        cur[cur_user_id] = name;
        return true;
    }

    bool unuse() {
        if (using_db()) {
            SavedTable::close_all();
//            cur.erase(cur.find(cur_user_id));
            cur.erase(cur_user_id);
        }
        return true;
    }

    // FIXME: memory leak here
    bool using_db() { return !cur[cur_user_id].empty(); }

    std::vector<String> all() { return { names.begin(), names.end() }; }

    std::vector<String> all_tables() {
        orange_check(using_db(), Exception::no_database_used());
        std::vector<String> data;
        for (const auto& it : fs::directory_iterator(DB_ROOT / fs::path(cur[cur_user_id]))) {
            if (it.is_directory()) data.push_back(it.path().filename().string());
        }
        return data;
    }

    fs::path cur_db_path() {
        orange_assert(using_db(), "not using any database but require a current database path");
        return DB_ROOT / fs::path(cur[cur_user_id]);
    }

    // must call once at first (in both CLI and server backend)
    void setup() {
        if (!fs::exists("orange-db")) fs::create_directory("orange-db");
        const_cast<fs::path&>(DB_ROOT) = fs::current_path() / fs::path("orange-db");
        for (const auto& entry : fs::directory_iterator(DB_ROOT)) {
            if (entry.is_directory()) {
                names.insert(entry.path().filename().string());
            }
        }
    }

    void paolu() {
        unuse();
        auto tmp = names;
        for (const auto& db_name : tmp) drop(db_name);
        fs::remove_all(DB_ROOT);
    }

    void cur_db_restore(int user_id) {
        cur_user_id = user_id;
        if (cur.find(cur_user_id) != cur.end()) {
            use(cur[cur_user_id]);
        }
    }
}  // namespace Orange