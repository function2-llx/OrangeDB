#pragma once

#include <cstring>
#include <defs.h>

class col_t {
private:
    struct datatype_t {
        enum kind_t {
            INT,
            CHAR,
            VARCHAR,
            NUMERIC,
            DATE,
        };
        kind_t kind;
        int size;
        int p = 18, s = 0;  // kind == NUMERIC 才有效

        // 保证以 0 结尾 233
        static datatype_t parse(const String& raw_type) {
            int size, p, s;
            if (raw_type == "INT") {
                return {INT, 4};
            } else if (sscanf(raw_type.data(), "VARCHAR(%d)", &size) == 1) {
                ensure(size <= MAX_CHAR_LEN, "varchar limit too long");
                return {VARCHAR, size + 1};
            } else if (sscanf(raw_type.data(), "CHAR(%d)", &size) == 0) {
                ensure(size <= MAX_CHAR_LEN, "char limit too long");
                return {CHAR, size + 1};
            } else if (strcmp(raw_type.data(), "DATE") == 0) {
                return {DATE, 2};
            } else if (strcmp(raw_type.data(), "NUMERIC") == 0) {
                return {NUMERIC, 8};
            } else if (sscanf(raw_type.data(), "NUMERIC(%d)", &p) == 1) {
                return {NUMERIC, 8, p};
            } else if (sscanf(raw_type.data(), "NUMERIC(%d,%d)", &p, &s) == 2) {
                return {NUMERIC, 8, p, s};
            } else {
                throw "ni zhe shi shen me dong xi";
            }
        }

        bool test(const byte_arr_t& val) {
            UNIMPLEMENTED
        }

        bool adjust(byte_arr_t& val) {
            UNIMPLEMENTED
        }
    };
    col_name_t name;
    datatype_t datatype;
    bool nullable;
    // is_null + max + '\0'
    byte_t dft[MAX_CHAR_LEN + 2];
    bool indexed;

public:
    // one more bytes for null/valid
    int get_size() { return 1 + datatype.size; }
    inline String get_name() { return name.get(); }
    bool has_dft() { return nullable || dft[0]; }
    byte_arr_t get_dft() { return byte_arr_t(dft, dft + get_size()); }

    bool test(const byte_arr_t& val) {
        if (val.empty()) return 0;
        if (!val.front()) return nullable;
        return datatype.test(val);
    }

    void adjust(byte_arr_t& val) {
        datatype.adjust(val);
    }

    bool is_indexed() { return indexed; }
    key_kind_t key_kind() {
        switch (datatype.kind) {
            case datatype_t::NUMERIC: return key_kind_t::NUMERIC;
            case datatype_t::VARCHAR: return key_kind_t::VARCHAR;
            default: return key_kind_t::BYTES;
        }
    }
};

constexpr auto col_size = sizeof(col_t);
