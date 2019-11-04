#pragma once

#include <vector>

#include <defs.h>
#include <fs/file/file.h>
#include <orange/index/b_tree.h>

// 同时维护数据和索引，有暴力模式和数据结构模式
// 希望设计的时候索引模块不需要关注完整性约束，而交给其它模块
class Index {
private:
    key_kind_t kind;
    size_t size;
    String prefix;

    bool on;

    File* f_data;
    BTree *tree;

    String data_name() { return prefix + ".data"; }
    String meta_name() { return prefix + ".meta"; }

    byte_arr_t convert(const_bytes_t k_raw) {
        switch (kind) {
            case key_kind_t::BYTES: return byte_arr_t(k_raw, k_raw + size);
            case key_kind_t::NUMERIC: UNIMPLEMENTED
            case key_kind_t::VARCHAR: UNIMPLEMENTED
        }
    }

    int cmp_key(const byte_arr_t& k1, const byte_arr_t& k2) const {
        switch (kind) {
            case key_kind_t::BYTES: return bytesncmp(k1.data(), k2.data(), size);
            case key_kind_t::NUMERIC: UNIMPLEMENTED
            case key_kind_t::VARCHAR: UNIMPLEMENTED
        }
    }

    int cmp(const byte_arr_t& k1, rid_t v1, const byte_arr_t& k2, rid_t v2) const {
        int key_code = cmp_key(k1, k2);
        return key_code == 0 ? v1 - v2 : key_code;
    }

    bool test_pred_lo(const byte_arr_t& k, const pred_t& pred) {
        int code = cmp_key(pred.lo, k);
        return code < 0 || (pred.lo_eq && code == 0);
    }

    bool test_pred_hi(const byte_arr_t& k, const pred_t& pred) {
        int code = cmp_key(k, pred.hi);
        return code < 0 || (pred.hi_eq && code == 0);
    }

    bool test_pred(const byte_arr_t& k, const pred_t& pred) {
        return test_pred_hi(k, pred) && test_pred_hi(k, pred);
    }
public:
    Index(key_kind_t kind, size_t size, const String& prefix, bool on) : kind(kind), size(size), prefix(prefix), on(on) {
        if (!fs::exists(data_name())) File::create(data_name());
        f_data = File::open(data_name());
    }
    Index(Index&& index) : kind(index.kind), size(index.size), prefix(index.prefix), on(index.on), f_data(index.f_data), tree(index.tree) {
        index.f_data = nullptr;
        index.on = 0; 
    }
    ~Index() {
        if (on) turn_off();
        if (f_data) f_data->close();
    }

    void load() {
        if (on) {
            tree = new BTree(kind, size, prefix);
            tree->load();
        }
    }

    void turn_on() {
        if (!on) {
            on = 1;
            tree = new BTree(kind, size, prefix);
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
        if (on) {
            if (kind == key_kind_t::BYTES) {
                tree->insert(val.data(), rid);
            } else {
                UNIMPLEMENTED
            }
        }
        if (kind == key_kind_t::BYTES) {
            f_data->seek_pos(rid * sizeof(rid_t))->write_bytes(val.data(), size);
        } else {
            UNIMPLEMENTED
        }
    }

    // 调用合适应该不会有问题8
    void remove(rid_t rid) {
        if (on) {
            bytes_t bytes = new byte_t[size];
            f_data->seek_pos(rid * size)->read_bytes(bytes, size);
            tree->remove(bytes, rid);
            delete[] bytes;
        }
        if (kind == key_kind_t::BYTES) {
            if (f_data->size() == rid * size) f_data->resize((rid - 1) * size);
            else f_data->seek_pos(rid * size)->write<byte_t>(DATA_INVALID);
        } else {
            UNIMPLEMENTED
        }
    }

    void update(const byte_arr_t& val, rid_t rid) {
        if (on) {
            remove(rid);
            insert(val, rid);
        }
        if (kind == key_kind_t::BYTES) {
            f_data->seek_pos(rid * size)->write_bytes(val.data(), size);
        } else {
            UNIMPLEMENTED
        }
    }

    // lo_eq 为真表示允许等于
    std::vector<rid_t> get_rid(const pred_t& pred, rid_t lim) {
        if (on) return tree->query(pred, lim);
        else {
            std::vector<rid_t> ret;
            bytes_t bytes = new byte_t[size];
            f_data->seek_pos(0);
            for (rid_t i = 0; i * size < f_data->size(); i++) {
                f_data->read_bytes(bytes, size);
                if (*bytes != DATA_INVALID && test_pred(convert(bytes), pred)) ret.push_back(i);
            }
            delete bytes;
        }
    }

    byte_arr_t get_val(rid_t rid) {
        auto bytes = new byte_t[size];
        f_data->seek_pos(rid * size)->read_bytes(bytes, size);
        if (kind == key_kind_t::BYTES) {
            auto ret = byte_arr_t(bytes, bytes + size);
            delete[] bytes;
            return ret;
        } else {
            UNIMPLEMENTED
        }
    }

    friend class BTree;
};
