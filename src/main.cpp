/*-
 * Copyright (c) 2014-2020 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <iostream>
#include <string>
#include <fstream>
#include <cassert>
#include "json.hpp"

#include "wiredtiger.h"
#include "RestIngestServer.hpp"

#include "wt.h"

using namespace nlohmann;

enum Type { BOOL, NUMBER, STRING, OBJECT, ARRAY, EMPTY };
std::string rtbl = "table:row_table";
std::string ctbl = "table:col_table";

Type convert_to_type(std::string type_name) {
    if (type_name == "string")
        return STRING;
    if (type_name == "object")
        return OBJECT;
    if (type_name == "number")
        return NUMBER;
    if (type_name == "array")
        return ARRAY;
    if (type_name == "boolean")
        return BOOL;
    return STRING;
}

Type get_type(json value) {

    std::string type_name = value.type_name();
    if (type_name == "object")
        return OBJECT;
    if (type_name == "number")
        return NUMBER;
    if (type_name == "array")
        return ARRAY;
    if (type_name == "boolean")
        return BOOL;
    return STRING;
}

int insert_column(WT_CURSOR *cursor, const std::string &key, uint64_t id, json value) {
    
    int ret = 0;    
    WT_ITEM item;
    auto val = value.dump();
    item.data = val.data();
    item.size = val.length();
    if ((ret = wt::col_table_insert(cursor, key, id, get_type(value), item)) == 0) {
        std::cout << "insert_column: (" << key << ", " << id << "), "
                  << "(" << value.type_name() << ", " << val.data() << ")\n";
    }
    return ret;
}

int insert_row(WT_CURSOR *cursor, uint64_t id, const std::string &key, json value) {

    int ret = 0;
    WT_ITEM item;
    auto val = value.dump();
    item.data = val.data();
    item.size = val.length();
    if ((ret = wt::row_table_insert(cursor, id, key, get_type(value), item)) == 0) {
        std::cout << "insert_row: (" << id << ", " << key << "), "
                  << "(" << value.type_name() << ", " << val.data() << ")\n";
    }
    return ret;
}

std::unordered_map<std::string, json> process_sub_json(json jsn) {
    std::unordered_map<std::string, json> kvs{};

    return kvs;
}

void process_json(WT_SESSION *session, json jsn, uint64_t id) {

    WT_CURSOR *rc = nullptr;
    WT_CURSOR *cc = nullptr;
    wt::open_cursor(session, rtbl, &rc);
    wt::open_cursor(session, ctbl, &cc);

    for (auto it : jsn.at("data").at(0).items()) {
        if (it.value().is_null()) {
            continue;
        } else if (it.value().is_object()) {
            for (auto itt : process_sub_json(it.value())) {
                insert_row(rc, id, itt.first, itt.second);
                insert_column(cc, itt.first, id, itt.second);
            }
        } else {
            insert_row(rc, id, it.key(), it.value());
            insert_column(cc, it.key(), id, it.value());
        }
    }
    wt::close_cursor(rc);
    wt::close_cursor(cc);
}

void load_file(WT_SESSION *session, const char *filename) {
    uint64_t id = 0;
    std::string line;
    std::ifstream ifs(filename);
    while (std::getline(ifs, line)) {
        process_json(session, json::parse(line), id);
        id++;
    }
}

int main(int argc, char **argv)
{
    int opt;
    const char *filename = "../raw_data/rockbench_10rows.json";
    const char *url = "127.0.0.1:8099";
    bool use_file = false;
    bool use_server = false;

    WT_CONNECTION *conn = nullptr;
    WT_SESSION *session = nullptr;
    std::string dbpath = "wt_test";

    // Shut GetOpt error messages down (return '?'):
    opterr = 0;
    while ( (opt = getopt(argc, argv, "FSf:s:")) != -1 ) {
        switch ( opt ) {
            case 'f':
                    filename = optarg;
                /* FALLTHROUGH */
            case 'F':
                    use_file = true;
                break;
            case 's':
                    url = optarg;
                /* FALLTHROUGH */
            case 'S':
                    use_server = true;
                break;
            case '?':  // unknown option...
                std::cerr << "Unknown option: '" << char(optopt) << "'!" << std::endl;
                break;
        }
    }

    if (use_file == use_server) {
        std::cout << "Invalid usage. Proper usage: " << std::endl;
        std::cout << argv[0] << "-F. For default file load (" << filename << ")" << std::endl;
        std::cout << argv[0] << "-S. For default server (" << url << ")" << std::endl;
        std::cout << argv[0] << "-f <file_name>. For choosing a file to load" << std::endl;
        std::cout << argv[0] << "-s <listen_uri>. For default file load" << std::endl;
        return (1);
    }

    int ret = 0;

    if ((ret = wt::open_database(dbpath, &conn, &session)) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }

    assert(conn);
    assert(session);
    if ((ret = wt::create_table(session, rtbl, "key_format=QS,value_format=Hu")) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }
    if ((ret = wt::create_table(session, ctbl, "key_format=SQ,value_format=Hu")) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }


    if (use_file) {
        load_file(session, filename);
    } else {
        RestIngestServer server;
        server.start();
    }

#if 1
    wt::row_table_print(session, rtbl);
    wt::col_table_print(session, ctbl);
#endif

    wt::close_database(conn);
    std::cout << "\nTammy Rocks!\n";
    return 0;
}
