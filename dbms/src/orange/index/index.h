#pragma once

#include <vector>

#include <defs.h>
#include <fs/file/file.h>
#include <fs/allocator/allocator.h>
#include <orange/index/b_tree.h>
#include "orange/parser/sql_ast.h"
#include "orange/common.h"

class SavedTable;

// 同时维护数据和索引，有暴力模式和数据结构模式
// 希望设计的时候索引模块不需要关注完整性约束，而交给其它模块
class Index {
private:
    SavedTable &table;
    Column *column;

    datatype_t kind;
    // 每个值的在索引中的大小
    size_t size;
    String prefix;

    bool on;

    File* f_data;
    BTree* tree;

    // 用于 vchar
    FileAllocator *allocator = nullptr;

    String data_name() { return prefix + ".data"; }
    String meta_name() { return prefix + ".meta"; }
    String vchar_name() { return prefix + ".vch"; }

    // 对于 vchar 返回指针，其它直接返回真实值
    byte_arr_t store(const byte_arr_t& key) {
        switch (kind) {
            case ORANGE_VARCHAR: return allocator->allocate_byte_arr(key);
            default: return key;
        }
    }
    byte_arr_t restore(const_bytes_t k_raw) const {
        switch (kind) {
            case ORANGE_VARCHAR: return allocator->read_byte_arr(*(size_t*)(k_raw + 1));
            default: return byte_arr_t(k_raw, k_raw + size);
        }
    }

    int cmp(const byte_arr_t& k1, rid_t v1, const byte_arr_t& k2, rid_t v2) const {
        int key_code = Orange::cmp(k1, k2, kind);
        return key_code == 0 ? v1 - v2 : key_code;
    }

    // 返回所在表的所有正在使用的 rid
    std::vector<rid_t> get_all() const;

    byte_arr_t get_raw(rid_t rid) {
        auto bytes = new byte_t[size];
        f_data->seek_pos(rid * size)->read_bytes(bytes, size);
        auto ret = byte_arr_t(bytes, bytes + size);
        delete[] bytes;
        return ret;
    }

public:
    Index(SavedTable& table, datatype_t kind, size_t size, const String& prefix, bool on) : table(table), kind(kind), size(size), prefix(prefix), on(on) {
        if (!fs::exists(data_name())) File::create(data_name());
        f_data = File::open(data_name());
        if (kind == ORANGE_VARCHAR) allocator = new FileAllocator(vchar_name());
    }
    Index(Index&& index) :
        table(index.table), kind(index.kind), size(index.size), prefix(index.prefix), on(index.on),
        f_data(index.f_data), tree(index.tree), allocator(index.allocator) {
        if (on) tree->index = this;
        index.f_data = nullptr;
        index.allocator = nullptr;
        index.on = 0;
    }
    ~Index() {
        if (on) delete tree;
        if (f_data) f_data->close();
        delete allocator;
    }

    //　返回 rid　记录此列的值
    auto get_val(rid_t rid) const {
        auto bytes = new byte_t[size];
        f_data->seek_pos(rid * size)->read_bytes(bytes, size);
        auto ret = restore(bytes);
        delete[] bytes;
        return ret;
    }

    void load() {
        if (on) {
            tree = new BTree(this, size, prefix);
            tree->load();
        }
    }

    void turn_on() {
        if (!on) {
            on = 1;
            tree = new BTree(this, size, prefix);
            tree->init(f_data);
        }
    }

    void turn_off() {
        if (on) {
            on = 0;
            delete tree;
        }
    }

    void insert(const byte_arr_t& val, rid_t rid) {
        auto raw = store(val);
        if (on) tree->insert(raw.data(), rid, val);
        f_data->seek_pos(rid * size)->write_bytes(raw.data(), size);
    }

    // 调用合适应该不会有问题8
    void remove(rid_t rid) {
        if (on) {
            auto raw = get_raw(rid);
            tree->remove(raw.data(), rid);
        }
        f_data->seek_pos(rid * size)->write<byte_t>(DATA_INVALID);
    }

    void update(const byte_arr_t& val, rid_t rid) {
        remove(rid);
        insert(val, rid);
    }

    // lo_eq 为真表示允许等于
    std::vector<rid_t> get_rid(const pred_t& pred, rid_t lim) {
        if (on)
            return tree->query(pred, lim);
        else {
            std::vector<rid_t> ret;
            bytes_t bytes = new byte_t[size];
            f_data->seek_pos(0);
            for (auto i: get_all()) {
                if (pred.test(get_val(i), kind)) ret.push_back(i);
            }
            delete[] bytes;
            return ret;
        }
    }

    auto get_rids_value(Orange::parser::op op, const Orange::parser::data_value& value) {
        orange_assert(!value.is_null(), "cannot be null here");
        if (on) {

        } else {
            std::vector<rid_t> ret;
            auto bytes = new byte_t[size];
            for (auto i: get_all()) {
                f_data->seek_off(i * size)->read_bytes(bytes, size);
                if (Orange::cmp(byte_arr_t(bytes, bytes + size), kind, op, value)) {
                    ret.push_back(i);
                }
            }            
            delete[] bytes;
            return ret;
        }
    }

    auto get_rids_null(bool not_null) {
        auto bytes = new byte_t[size];
        std::vector<rid_t> ret;
        for (auto i: get_all()) {
            f_data->seek_off(i * size)->read_bytes(bytes, size);
            if (not_null && *bytes != DATA_NULL || !not_null && *bytes == DATA_NULL) {
                ret.push_back(i);
            }
        }
        delete[] bytes;
        return ret;
    }

    friend class BTree;
};
