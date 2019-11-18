#pragma once

#include <cstring>
#include <defs.h>

class File;

class Column {
private:
    String name;
    datatype_t kind;
    int maxsize;
    int p = 18, s = 0;  // kind == ORANGE_NUMERIC 才有效
    bool unique, nullable, index;
    byte_arr_t dft;
    // std::vector<std::pair<byte_arr_t, byte_arr_t>> ranges;
    std::vector<pred_t> ranges;
    bool is_string() const { return kind == ORANGE_CHAR || kind == ORANGE_VARCHAR; }

    bool test_size(byte_arr_t& val) {
        if (int(val.size()) > maxsize) return 0;
        if (int(val.size()) < maxsize) {
            if (is_string()) return 0;
            if (kind == ORANGE_CHAR) val.resize(maxsize);
        }
        return 1;
    }
public:
    Column() {}
    Column(const String& name, const String& raw_type, bool unique, bool index, bool nullable, byte_arr_t dft, 
        std::vector<pred_t> ranges) : name(name), unique(unique), nullable(nullable), index(index), dft(dft), ranges(ranges) {
        if (raw_type == "INT") {
            kind = ORANGE_INT;
            maxsize = 4 + 1;
        } else if (sscanf(raw_type.data(), "VARCHAR(%d)", &maxsize) == 1) {
            ensure(maxsize <= MAX_VARCHAR_LEN, "varchar limit too long");
            kind = ORANGE_VARCHAR;
            maxsize += 2;   // 开头 null，末尾 \0
        } else if (sscanf(raw_type.data(), "CHAR(%d)", &maxsize) == 1) {
            ensure(maxsize <= MAX_CHAR_LEN, "char limit too long");
            kind = ORANGE_CHAR;
            maxsize += 2;
        } else if (strcmp(raw_type.data(), "DATE") == 0) {
            kind = ORANGE_DATE;
            maxsize = 2 + 1;    // 待定
        } else if (strcmp(raw_type.data(), "NUMERIC") == 0) {
            kind = ORANGE_NUMERIC;
            maxsize = 17 + 1;
            p = 18;
            s = 0;
        } else if (sscanf(raw_type.data(), "NUMERIC(%d)", &p) == 1) {
            kind = ORANGE_NUMERIC;
            maxsize = 17 + 1;
            s = 0;
        } else if (sscanf(raw_type.data(), "NUMERIC(%d,%d)", &p, &s) == 2) {
            kind = ORANGE_NUMERIC;
            maxsize = 17 + 1;
        } else {
            throw "ni zhe shi shen me dong xi";
        }
        if (kind == ORANGE_NUMERIC) ensure(0 <= s && s <= p, "bad numeric");
        for (auto &pred: this->ranges) {
            ensure(test_size(pred.lo) && test_size(pred.hi), "bad constraint parameter");
        }
        ensure(test(this->dft), "default value fails constraint");
    }

    // one more bytes for null/valid
    int key_size() const { return kind == ORANGE_VARCHAR ? 1 + sizeof(size_t) : maxsize; }

    String get_name() { return name; }
    bool has_dft() { return nullable || dft[0]; }
    byte_arr_t get_dft() { return dft; }

    // 测试 val 能否插入到这一列；对于 char 会补零
    bool test(byte_arr_t& val) {
        if (val.empty()) return 0;
        if (!test_size(val)) return 0;
        if (!val.front()) return nullable;
        for (auto &pred: ranges) if (!pred.test(val, kind)) return 0;
        return 1;
    }

    bool has_index() { return index; }
    bool is_unique() { return unique; }
    datatype_t get_datatype() { return kind; }

    friend class File;
};

constexpr auto col_size = sizeof(Column);
