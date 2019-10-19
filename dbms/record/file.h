#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cassert>
#include <defs.h>
#include "fs/fileio/FileManager.h"
#include "fs/utils/pagedef.h"
#include <record/filed_def.h>
#include <fs/bufmanager/BufPageManager.h>
#include <fs/bufmanager/buf_page.h>
#include <fs/bufmanager/buf_page_stream.h>

// 打开的文件
class File {
private:
    int id;
    String name;

    // file's metadata
    int record_size, record_cnt;
    std::vector<FieldDef> fields;

    File(int id, String name) : id(id), name(name) {}
    File(const File&) = delete;

    void init_metadata(const std::vector<std::pair<String, String>>& name_type_list) {
        assert(name_type_list.size() <= MAX_COL_NUM);
        fields.clear();
        BufPageStream os(get_buf_page(0));

        record_size = 0;
        os.write_obj<byte_t>(name_type_list.size());
        for (auto &name_type: name_type_list) {
            auto field = FieldDef::parse(name_type);
            os.write_bytes(field.to_bytes(), COL_SIZE);
            record_size += field.get_size();
            fields.push_back(std::move(field));
        }
        os.memset(0, COL_SIZE * (MAX_COL_NUM - fields.size()))
            .write_obj<uint16_t>(record_size)
            .write_obj(record_cnt = 0)
            .memset();
    }

    void load_metadata() {
        fields.clear();
        BufPageStream os(get_buf_page(0));

        fields.reserve(os.get<int>());
        for (int i = 0; i < fields.capacity(); i++) {
            ByteArr tmp;
            os.get_bytes(tmp, COL_SIZE);
            fields.push_back(FieldDef::parse_bytes((bytes_t)tmp.c_str()));
        }
        os.seekoff(COL_SIZE * (MAX_COL_NUM - fields.size()))
            .get_obj<uint16_t>(record_size)
            .get_obj<uint16_t>(record_cnt);
    }

    BufPage get_buf_page(int page_id) { return BufPage(id, page_id); }

    static File *file[MAX_FILE_NUM];
public:
    static bool create(const std::string& name, int record_size, const std::vector<std::pair<String, String>>& name_type_list) {
        auto fm = FileManager::get_instance();
        int code = fm->create_file(name.c_str());
        ensure(code == 0, "file create fail");
        int id;
        code = fm->open_file(name.c_str(), id);
        ensure(code == 0, "file open failed");
        file[id] = new File(id, name);
        file[id]->init_metadata(name_type_list);
        code = fm->close_file(id);
        ensure(code == 0, "file close failed");
        delete file[id];
        return 1;
    }

    // 不存在就错误了
    static File* open(const String& name) {
        auto fm = FileManager::get_instance();
        int id;
        int code = fm->open_file(name.c_str(), id);
        ensure(code == 0, "file open failed");
        if (file[id] == nullptr) {
            file[id] = new File(id, name);
            file[id]->load_metadata();
        }
        return file[id];
    }

    // 调用后 f 变成野指针
    static bool close(File *f) {
        auto fm = FileManager::get_instance();
        int code = fm->close_file(f->id);
        ensure(code == 0, "file close failed");
        ensure(file[f->id] == f, "this is magic");
        file[f->id] = nullptr;
        delete f;
        return 1;
    }

    static bool remove(const String& name) {
        auto fm = FileManager::get_instance();
        int code = fm->remove_file(name);
        ensure(code == 0, "remove file failed");
        return 1;
    }

    String get_name() { return this->name; }
};