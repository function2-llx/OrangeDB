#pragma once

#include "defs.h"
#include "orange/parser/sql_ast.h"

static String to_string(orange_t type) {
    switch (type) {
        case orange_t::Int: return "int";
        case orange_t::Numeric: return "numeric";
        case orange_t::Char:
        case orange_t::Varchar: return "string";
        case orange_t::Date: return "date";
    }
    return "fuck warning";
}

namespace Orange {
    // 直接按照 op 比较
    template<typename T1, typename T2>
    inline bool cmp(const T1& t1, Orange::parser::op op, const T2& t2) {
        using op_t = decltype(op);
        switch (op) {
            case op_t::Eq: return t1 == t2;
            case op_t::Ge: return t1 >= t2;
            case op_t::Gt: return t1 > t2;
            case op_t::Le: return t1 <= t2;
            case op_t::Lt: return t1 < t2;
            case op_t::Neq: return t1 != t2;
        }
        return (bool)"fuck warning";
    }

    // bytes 与 value 按照 op 比较
    inline bool cmp(const byte_arr_t& v1_bytes, orange_t t1, Orange::parser::op op, const Orange::parser::data_value& v2) {
        using op_t = Orange::parser::op;
        orange_assert(!v2.is_null(), "value should not be null here");
        if (v1_bytes.front() == DATA_NULL) {
            return op == op_t::Le || op == op_t::Lt || op == op_t::Neq;
        }
        switch (t1) {
            case orange_t::Int: {
                int v1 = bytes_to_int(v1_bytes);
                if (v2.is_int()) return cmp(v1, op, v2.to_int());
                else if (v2.is_float()) {
                    ORANGE_UNIMPL
                } else {
                    ORANGE_UNREACHABLE
                }
            } break;
            case orange_t::Varchar:
            case orange_t::Char: {
                auto v1 = bytes_to_string(v1_bytes);
                if (v2.is_string()) return cmp(v1, op, v2.to_string());
                else {
                    ORANGE_UNREACHABLE
                }
            } break;
            case orange_t::Numeric: {
                ORANGE_UNIMPL
            } break;
            case orange_t::Date: {
                ORANGE_UNIMPL
            } break;
        }
        return (bool)"fuck warning";
    }

    // bytes 和 bytes 按照 op 比较，保证可以比较
    inline bool cmp(const byte_arr_t& v1_bytes, orange_t t1, Orange::parser::op op, const byte_arr_t& v2_bytes, orange_t t2) {
        using op_t = Orange::parser::op;
        if (v1_bytes.front() == DATA_NULL) {
            switch (op) {
                case op_t::Le:return true;
                case op_t::Eq:
                case op_t::Ge: return v2_bytes.front() == DATA_NULL;
                case op_t::Lt:
                case op_t::Neq: return v2_bytes.front() != DATA_NULL;
                case op_t::Gt: return false;
            }
        }
        // 此时 v1　一定不是 null
        if (v2_bytes.front() == DATA_NULL) return op == op_t::Neq || op == op_t::Gt || op == op_t::Ge;
        switch (t1) {
            case orange_t::Int: {
                int v1 = Orange::bytes_to_int(v1_bytes);
                switch (t2) {
                    case orange_t::Int: {
                        return cmp(v1, op, bytes_to_int(v2_bytes));
                    } break;
                    case orange_t::Numeric:
                        ORANGE_UNIMPL
                    break;
                    default: 
                        throw OrangeError("types `" + to_string(t1) + "` and `" + to_string(t2) + "` are not comparable");
                }
            } break;
            case orange_t::Varchar:
            case orange_t::Char: {
                String v1 = Orange::bytes_to_string(v1_bytes);
                switch (t2) {
                    case orange_t::Char:
                    case orange_t::Varchar: {
                        int code = v1.compare(Orange::bytes_to_string(v2_bytes));
                        switch (op) {
                            case op_t::Eq: return code == 0;
                            case op_t::Ge: return code >= 0;
                            case op_t::Gt: return code > 0;
                            case op_t::Le: return code <= 0;
                            case op_t::Lt: return code < 0;
                            case op_t::Neq: return code != 0;
                        }
                    } break;
                    default:
                        throw OrangeError("types `" + to_string(t1) + "` and `" + to_string(t2) + "` are not comparable");
                }
            } break;
            case orange_t::Numeric:
                ORANGE_UNIMPL
            case orange_t::Date:
                ORANGE_UNIMPL
        }
        ORANGE_UNREACHABLE
        return 1;
    }

    // 同种 bytes 比较
    inline int cmp(const byte_arr_t& k1, const byte_arr_t& k2, orange_t kind) {
        if (k1.front() != k2.front()) return int(k1.front()) - int(k2.front());
        switch (kind) {
            case orange_t::Int: return *(int_t*)(k1.data() + 1) - *(int_t*)(k2.data() + 1);
            case orange_t::Char:
                for (int i = 1; i < int(k1.size()); i++) {
                    if (k1[i] != k2[i]) return int(k1[i]) - int(k2[i]);
                }
                return 0;
            case orange_t::Varchar:
                for (int i = 1; i < int(std::min(k1.size(), k2.size())); i++) {
                    if (k1[i] != k2[i]) return int(k1[i]) - int(k2[i]);
                }
                if (k1.size() == k2.size()) return 0;
                return k1.size() < k2.size() ? -int(k2[k1.size()]) : k1[k2.size()];
            default: ORANGE_UNIMPL
        }
    }
};
