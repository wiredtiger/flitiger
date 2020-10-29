/*-
 * Copyright (c) 2014-2020 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <iostream>
#include <iomanip>
#include <locale>
#include <string>
#include <fstream>
#include <cassert>
#include <cpprest/json.h>

#include "wiredtiger.h"
#include "RestIngestServer.hpp"
#include "wt.h"

std::string rtbl = "table:row_table";
std::string ctbl = "table:col_table";
uint64_t db_document_id = 0;
WT_SESSION* session = NULL;

int insert_column(WT_CURSOR *cursor, const std::string &key, uint64_t id, web::json::value value) {
    int ret = 0;
    WT_ITEM item;
    auto val = value.serialize();
    item.data = val.data();
    item.size = val.length();
    if ((ret = wt::col_table_insert(cursor, key, id, value.type(), item)) == 0) {
    //    std::cout << "insert_column: (" << key << ", " << id << "), "
    //              << "(" << value.type() << ", " << val.data() << ")\n";
    }
    return ret;
}

int insert_row(WT_CURSOR *cursor, uint64_t id, const std::string &key, web::json::value value) {
    int ret = 0;
    WT_ITEM item;
    auto val = value.serialize();
    item.data = val.data();
    item.size = val.length();
    if ((ret = wt::row_table_insert(cursor, id, key, value.type(), item)) == 0) {
    //    std::cout << "insert_row: (" << id << ", " << key << "), "
    //              << "(" << value.type() << ", " << val.data() << ")\n";
    }
    return ret;
}

void process_json(web::json::object bjct, std::string full_key, WT_CURSOR* rc, WT_CURSOR* cc) {
    for (auto it : bjct) {
        auto val = it.second;
        auto key = it.first;
        std::string new_key = full_key;
        if (val.is_null()) {
            continue;
        } else if (val.is_object()) {
            if (!new_key.empty())
                new_key.append(".");
            new_key.append(key);
            process_json(val.as_object(), new_key, rc, cc);
        } else {
            if (!new_key.empty()) {
                new_key.append("." + key);
                insert_row(rc, db_document_id, new_key, val);
                insert_column(cc, new_key, db_document_id, val);
            } else {
                insert_row(rc, db_document_id, key, val);
                insert_column(cc, key, db_document_id, val);
            }
        }
    }
}

void process_json_top_level(web::json::value jsn) {
    WT_CURSOR *rc = nullptr;
    WT_CURSOR *cc = nullptr;
    wt::open_cursor(session, rtbl, &rc);
    wt::open_cursor(session, ctbl, &cc);

    // The top level data is an array so iterate over it.
    for (auto it : jsn.at("data").as_array()) {
        process_json(it.as_object(), "", rc, cc);
        db_document_id++;
    }
    wt::close_cursor(rc);
    wt::close_cursor(cc);
}

void load_file(const char *filename) {
    std::string line;
    std::ifstream ifs(filename);
    while (std::getline(ifs, line)) {
        process_json_top_level(web::json::value::parse(line));
    }
}

template<class T>
std::string commafy(T value) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << value;
    return ss.str();
}

void query_table(const char* query_field, bool query_col_table) {
    wt::metrics mtr;
    std::string table = query_col_table ? ctbl : rtbl;
    std::cout << "\nQuerying '" << table << "' for field '" << query_field << "' ...\n";
    if (query_col_table)
        wt::query_table(session, table, query_field, query_col_table, mtr);
    else
        wt::query_row_table(session, table, query_field, mtr);
    std::cout << "-------------------------------\n";
    std::cout << " Query time:  " << mtr.query_time << " ms\n";
    std::cout << " Bytes read:  " << commafy(mtr.bytes_read) << '\n';
    std::cout << " Read count:  " << commafy(mtr.read_count) << '\n';
    std::cout << " Match count: " << commafy(mtr.match_count) << '\n';
    std::cout << "-------------------------------\n";
}

int main(int argc, char **argv)
{
    int opt;
    const char *filename = "../raw_data/rockbench_10rows.json";
    const char *url = "127.0.0.1:8099";
    const char *query_field = "tiger";
    bool use_file = false;
    bool run_query = false;
    bool use_server = false;

    WT_CONNECTION *conn = nullptr;
    std::string dbpath = "wt_test";

    // Shut GetOpt error messages down (return '?'):
    opterr = 0;
    while ( (opt = getopt(argc, argv, "FSf:q:s:")) != -1 ) {
        switch ( opt ) {
            case 'q':
                    run_query = true;
                    query_field = optarg;
                break;
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

    if (use_file == use_server && !run_query) {
        std::cout << "Invalid usage. Proper usage: " << std::endl;
        std::cout << argv[0] << " -F. For default file load (" << filename << ")" << std::endl;
        std::cout << argv[0] << " -S. For default server (" << url << ")" << std::endl;
        std::cout << argv[0] << " -f <file_name>. For choosing a file to load" << std::endl;
        std::cout << argv[0] << " -q <field_name>. Query field and output metrics" << std::endl;
        std::cout << argv[0] << " -s <listen_uri>. For default file load" << std::endl;
        return (1);
    }

    int ret = 0;

    if ((ret = wt::open_database(dbpath, &conn, &session)) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }

    assert(conn);
    assert(session);

    std::string config = "type=lsm,key_format=QS,value_format=Hu";
    if ((ret = wt::create_table(session, rtbl, config)) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }
    config = "type=lsm,key_format=SQ,value_format=Hu";
    if ((ret = wt::create_table(session, ctbl, config)) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }

    wt::get_last_row_insert_id(session, rtbl, &db_document_id);
    if (db_document_id) db_document_id++;

    if (run_query) {
        if (db_document_id) {
            query_table(query_field, true);
            query_table(query_field, false);
        } else {
            std::cout << "Unable to execute query: Database is empty :(\n";
        }
    } else {
        if (use_file) {
            load_file(filename);
        } else {
            RestIngestServer server(&process_json_top_level);
            server.start();
        }
    }

    //wt::row_table_print(session, rtbl);
    //wt::col_table_print(session, ctbl);

    wt::close_database(conn);
    std::cout << "\nTammy Rocks!\n";
    return 0;
}
