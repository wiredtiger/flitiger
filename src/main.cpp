/*-
 * Copyright (c) 2014-2020 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <chrono>
#include <ctime>
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
std::atomic<uint64_t>  db_document_id = 0;
WT_CONNECTION* db_conn = nullptr;

int insert_column(WT_CURSOR *cursor, const std::string &key, uint64_t id, web::json::value value) {
    int ret = 0;
    WT_ITEM item;
    auto val = value.serialize();
    item.data = val.data();
    item.size = val.length();
    if ((ret = wt::col_table_insert(cursor, key, id, value.type(), item)) == 0) {
        //std::cout << "insert_column: (" << key << ", " << id << "), "
        //          << "(" << value.type() << ", " << val.data() << ")\n";
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
        //std::cout << "insert_row: (" << id << ", " << key << "), "
        //          << "(" << value.type() << ", " << val.data() << ")\n";
    }
    return ret;
}

void process_json(WT_SESSION* session, web::json::object bjct, std::string full_key, WT_CURSOR* rc, WT_CURSOR* cc, uint64_t id) {
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
            process_json(session, val.as_object(), new_key, rc, cc, id);
        } else {
            if (!new_key.empty()) {
                new_key.append("." + key);
                insert_row(rc, id, new_key, val);
                insert_column(cc, new_key, id, val);
            } else {
                insert_row(rc, id, key, val);
                insert_column(cc, key, id, val);
            }
        }
    }
}

uint64_t get_next_id(){
    return db_document_id++;
}

void process_json_top_level(web::json::value jsn) {
    thread_local WT_SESSION *session;
    thread_local WT_CURSOR *rc = nullptr;
    thread_local WT_CURSOR *cc = nullptr;

    if (session == nullptr) {
        wt::open_session(db_conn, &session);
        wt::open_cursor(session, rtbl, &rc);
        wt::open_cursor(session, ctbl, &cc);
    }

    // The top level data is an array so iterate over it.
    for (auto it : jsn.at("data").as_array()) {
        process_json(session, it.as_object(), "", rc, cc, get_next_id());
    }
}

void load_file(const char *filename) {
    std::string line;
    std::ifstream ifs(filename);
    auto start = std::chrono::system_clock::now();
    for (int i = 0; std::getline(ifs, line); i++) {
        if (i != 0 && i % 10000 == 0) {
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;
            std::cout << "Processed " << i << " lines from file in " << elapsed_seconds.count() << "s" <<std::endl;
            start = end;
        }
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

void query_table(WT_SESSION* session, const char* query_field, bool query_col_table) {
    wt::metrics mtr;
    std::string table = query_col_table ? ctbl : rtbl;
    std::cout << "\nQuerying '" << table << "' for field '" << query_field << "' ...\n";
    if (query_col_table)
        wt::query_col_table(session, table, query_field, mtr);
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
    uint64_t start_id;
    const char *filename = "../raw_data/rockbench_10rows.json";
    const char *url = "127.0.0.1:8099";
    const char *query_field = "tiger";
    bool use_file = false;
    bool use_lsm = false;
    bool run_query = false;
    bool use_server = false;

    WT_SESSION *session;
    std::string dbpath = "wt_test";

    // Speed up std::cout
    std::ios::sync_with_stdio(false);

    // Shut GetOpt error messages down (return '?'):
    opterr = 0;
    while ( (opt = getopt(argc, argv, "FLSf:q:s:")) != -1 ) {
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
            case 'L':
                    use_lsm = true;
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

    if ((ret = wt::open_database(dbpath, &db_conn, &session)) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }

    assert(db_conn);
    assert(session);

    std::string base_config = "";
    std::string lsm_config = "type=lsm,lsm=(chunk_size=100MB),";
    if (use_lsm)
        base_config += lsm_config;
    std::string config = base_config + "key_format=QS,value_format=Hu";
    if ((ret = wt::create_table(session, rtbl, config)) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }
    config = base_config + "key_format=SQ,value_format=Hu";
    if ((ret = wt::create_table(session, ctbl, config)) != 0) {
        std::cout << wt::get_error_message(ret) << '\n';
        return ret;
    }

    wt::get_last_row_insert_id(session, rtbl, &start_id);
    if (start_id) start_id++;

    if (run_query) {
        if (start_id) {
            query_table(session, query_field, true);
            query_table(session, query_field, false);
        } else {
            std::cout << "Unable to execute query: Database is empty :(\n";
        }
    } else {
        db_document_id = start_id;
        if (use_file) {
            load_file(filename);
        } else {
            RestIngestServer server(&process_json_top_level);
            server.start();
        }
    }

    //wt::row_table_print(session, rtbl);
    //wt::col_table_print(session, ctbl);

    wt::close_database(db_conn);
    std::cout << "\nTammy Rocks!\n";
    return 0;
}
