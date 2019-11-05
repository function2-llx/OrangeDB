#include <defs.h>
#include <fs/bufpage/bufpage.h>
#include <fs/bufpage/bufpage_stream.h>
#include <fs/file/file.h>
#include <fs/file/file_manage.h>
#include <orange/table/table.h>

#include <catch/catch.hpp>

using namespace std;

TEST_CASE("test fs io", "[fs]") {
    constexpr int TEST_PAGE_NUM = 100000;
    fs::create_directory("test_dir");
    fs::current_path("test_dir");

    String name1 = "testfile1.txt", name2 = "testfile2.txt";
    File::create(name1);
    File::create(name2);

    auto f1 = File::open(name1), f2 = File::open(name2);
    cerr << "file opened" << endl;

    cerr << "writing..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        printf("\r page id: %d", page_id);
        f1->seek_pos(page_id << PAGE_SIZE_IDX)
            ->write(page_id + 666, std::vector<int>{page_id + 233, page_id + 2333});
        f2->seek_pos(page_id << PAGE_SIZE_IDX)
            ->write(page_id - 777, std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }

    cerr << "\nchecking buf..." << endl;
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        f1->seek_pos(page_id << PAGE_SIZE_IDX);
        REQUIRE(f1->read<int>() == page_id + 666);
        REQUIRE(f1->read<std::vector<int>>() == std::vector<int>{page_id + 233, page_id + 2333});
        f2->seek_pos(page_id << PAGE_SIZE_IDX);
        REQUIRE(f2->read<int>() == page_id - 777);
        REQUIRE(f2->read<std::vector<int>>() ==
                std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }
    cerr << GREEN << "success" << RESET << endl;

    cerr << "checking write back..." << endl;
    BufpageManage::write_back_all();
    for (int page_id = 0; page_id < TEST_PAGE_NUM; ++page_id) {
        f1->seek_pos(page_id << PAGE_SIZE_IDX);
        REQUIRE(f1->read<int>() == page_id + 666);
        REQUIRE(f1->read<std::vector<int>>() == std::vector<int>{page_id + 233, page_id + 2333});
        f2->seek_pos(page_id << PAGE_SIZE_IDX);
        REQUIRE(f2->read<int>() == page_id - 777);
        REQUIRE(f2->read<std::vector<int>>() ==
                std::vector<int>{page_id - 62, page_id - 233, page_id - 2333});
    }
    cerr << GREEN << "success" << RESET << endl;

    ensure(f1->close(), "close failed");
    ensure(File::remove(name1), "remove failed");
    ensure(f2->close(), "close failed");
    ensure(File::remove(name2), "remove failed");

    fs::current_path("..");
    fs::remove("test_dir");
    cerr << "save your disk!" << endl;
}

template<typename T>
static byte_arr_t to_bytes(const T& t) {
    byte_arr_t ret(1);
    ret[0] = 1;
    for (unsigned i = 1; i <= sizeof(T); i++) {
        ret.push_back(*((byte_t*)&t + (sizeof(T) - i)));
    }
    return ret;
}

TEST_CASE("table", "[table]") {
    fs::remove_all("db");
    fs::create_directory("db");
    fs::current_path("db");

    Orange::create("test");
    Orange::use("test");

    Table::create("test", {col_t("test", "INT", 0, 0, 1, {DATA_NULL, 0, 0, 0, 0}, {})}, {}, {});
    cerr << "create table test" << endl;
    auto table = Table::get("test");

    std::mt19937 rng(time(0));
    constexpr int lim = 5000;
    static int a[lim];
    std::unordered_multiset<int> all, rm;
    for (int i = 0; i < lim; i++) {
        a[i] = rng() % 5000;
        all.insert(a[i]);
        if (rng() & 1) rm.insert(a[i]);
    }
    std::cerr << "test insert" << std::endl;
    for (int i = 0; i < lim; i++) {
        table->insert({{"test", to_bytes(a[i])}});
    }

    std::cerr << "testing remove" << std::endl;
    int i = 0;
    for (int x: rm) {
        auto pos = table->where("test", pred_t{to_bytes(x), 1, to_bytes(x), 1}, lim);
        REQUIRE(pos.size() == all.count(x));
        all.erase(all.find(x));
        table->remove({pos.front()});
        i++;
        std::cerr << '\r' << i << '/' << rm.size();
    }
    std::cerr << endl;

    Orange::unuse();

    fs::current_path("..");
    REQUIRE(fs::exists("db"));
    fs::remove_all("db");
}
