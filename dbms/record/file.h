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
#include <record/bytes_io.h>
#include <fs/bufmanager/buf_page.h>

class File {
private:
    int id;
    String name;

    int record_size, record_cnt;
    std::vector<FieldDef> fields;

    void init_metadata(const std::vector<std::pair<String, String>>& name_type_list) {
        assert(name_type_list.size() <= MAX_COL_NUM);
        fields.clear();
        BufPage buf_page = {id, 0};

        record_size = record_cnt = 0;
        size_t offset = 0;
        offset += buf_page.write_obj<byte_t>(name_type_list.size(), offset);
        for (auto &name_type: name_type_list) {
            auto field = FieldDef::parse(name_type);
            offset += buf_page.write_bytes(field.to_bytes(), offset, COL_SIZE);
            record_size += field.get_size();
            fields.push_back(std::move(field));
        }
        offset += buf_page.memset(0, offset, PAGE_SIZE * (MAX_COL_NUM - fields.size()));
        offset += buf_page.write_obj<uint16_t>(record_size, offset);
        offset += buf_page.write_obj(record_cnt, offset);
        offset += buf_page.memset(0, offset, PAGE_SIZE - offset);
        assert(offset == PAGE_SIZE);
    }

    void load_metadata() {
        // TODO
    }

    static File file[MAX_FILE_NUM];
public:
    static bool create(const std::string& name, int record_size, const std::vector<std::pair<String, String>>& name_type_list) {
        int code = FileManager::get_instance()->create_file(name.c_str());
        assert(code == 0);
        int id;
        FileManager::get_instance()->open_file(name.c_str(), id);
        file[id].init_metadata(name_type_list);
        assert(code == 0);
        FileManager::get_instance()->close_file(id);
        return 1;
    }

    static File* open(const std::string& name) {
        int id;
        int code = FileManager::get_instance()->open_file(name.c_str(), id);
        assert(code == 0);
        file[id].id = id;
        file[id].name = name;
        file[id].load_metadata();
        return &file[id];
    }

    static bool close(int id) {
        int code = FileManager::get_instance()->close_file(id);
        assert(code == 0);
        return 1;
    }

    static bool remove(const std::string& name) {
        int code = FileManager::get_instance()->remove_file(name);
        assert(code == 0);
        return 1;
    }
};