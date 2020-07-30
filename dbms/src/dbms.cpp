#ifdef _WIN32
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C"
#endif

#include "orange/orange.h"
#include <orange/syntax/syntax.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace Orange;

// 开buffer是因为只能传指针回去，好处是不需要delete了，坏处是多线程就完蛋了
static char buffer[1048576];

// 服务端自带多线程，所以这里可能要处理一下
API const char* exec(const char* sql, int user_id) {
    parser::sql_parser sql_parser;

    parser::sql_ast ast;
    try {
        ast = sql_parser.parse(sql);
    }
    catch (const parser::parse_error& e) {
        sprintf(buffer, R"([{"error": "在第 %d 个字符附近有语法错误"}])", e.first);
        return buffer;
    }

    auto results = program(ast, user_id);

    using namespace rapidjson;
    Document d(kArrayType);
    auto& allocator = d.GetAllocator();

    for (auto& result : results) {
        if (!result.ok()) {
            Value msg(kObjectType);
            msg.AddMember("error", Value(result.what().data(), allocator).Move(), allocator);
            d.PushBack(msg.Move(), allocator);
            break;
        }
        if (result.has()) {
            Value table(kObjectType);

            Value headers(kArrayType);
            auto& cols = result.get().get_cols();
            for (auto& col : cols) {
                Value v(kStringType);
                v.SetString(col.get_name().c_str(), col.get_name().length(), allocator);
                headers.PushBack(v, allocator);
            }
            table.AddMember("headers", headers.Move(), allocator);

            Value data(kArrayType);
            auto& recs = result.get().get_recs();
            for (auto& rec : recs) {
                Value row(kArrayType);
                for (size_t i = 0; i < rec.size(); i++) {
                    Value elem;
                    switch (cols[i].get_datatype().kind) {
                        case orange_t::Int: elem.SetInt(Orange::bytes_to_int(rec[i])); break;
                        case orange_t::Char:
                        case orange_t::Varchar: {
                            auto s = Orange::bytes_to_string(rec[i]);
                            elem.SetString(s.c_str(), s.size(), allocator);
                            break;
                        }
                        case orange_t::Numeric:
                            elem.SetDouble((double)Orange::bytes_to_numeric(rec[i]));
                            break;
                        case orange_t::Date: {
                            std::tm date = Orange::bytes_to_date(rec[i]);
                            std::ostringstream ss;
                            ss << std::put_time(&date, "%Y-%m-%d");
                            const auto& s = ss.str();
                            elem.SetString(s.data(), s.size(), allocator);
                            break;
                        }
                    }
                    row.PushBack(elem.Move(), allocator);
                }
                data.PushBack(row.Move(), allocator);
            }
            table.AddMember("data", data.Move(), allocator);

            d.PushBack(table.Move(), allocator);
        } else {  // 只有信息
            Value table(kObjectType);

            Value headers(kArrayType);
            headers.PushBack("操作结果", allocator);
            table.AddMember("headers", headers.Move(), allocator);

            Value data(kArrayType);
            Value row(kArrayType);
            row.PushBack(Value("操作成功完成", allocator).Move(), allocator);
            data.PushBack(row.Move(), allocator);
            table.AddMember("data", data.Move(), allocator);

            d.PushBack(table.Move(), allocator);
        }
    }

    StringBuffer strbuf;
    Writer<StringBuffer> writer(strbuf);
    d.Accept(writer);
    memcpy(buffer, strbuf.GetString(), std::min(sizeof(buffer), strbuf.GetSize()));
    buffer[strbuf.GetSize()] = '\0';

    // 返回数据的格式是一个数组，数组每个元素对应每一条语句的执行结果；
    // 一条语句的执行结果包括一个一维数组 "headers"，表头，还有一个二维数组 "data"，代表数据
    // 例子：（假如语句是 "select * from fruit; drop table fruit;"）
    // [
    //   {
    //     "headers": ["名称", "价钱"],
    //     "data": [
    //       ["苹果", 10.5],
    //       ["西瓜", 5],
    //     ],
    //   },
    //   {
    //     "headers": ["操作结果"],
    //     "data": [ ["删除表格成功"] ],
    //   },
    // ]
    // 如果某一条语句失败了，就换成类似于 { "error": "插入失败" }，并且不执行失败语句之后的语句
    return buffer;
}

API const char* info() {
    return "";
}

API void setup() {
    Orange::setup();
}
