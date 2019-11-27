#pragma once

#include "sql_ast.h"
#include <defs.h>
#include <exception>


namespace Orange {
    namespace parser {
        // 使用 boost::spirit 写的 parser
        class sql_parser {
        public:
            sql_ast parse(const std::string& sql);
        };

        struct parse_error : public std::runtime_error {
            int first, last;
            std::string expected;

            parse_error(const char* msg, int first, int last, const std::string& expected) :
                std::runtime_error(msg), first(first), last(last), expected(expected) {}
        };
    }  // namespace parser
}  // namespace Orange
