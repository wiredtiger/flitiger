/*-
 * Copyright (c) 2014-2020 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include "wiredtiger.h"
#include "wt.h"

// Some files included by wt_internal.h have some C-ism's that don't work in C++.
extern "C" {
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "error.h"
#include "misc.h"
}

namespace wt {
enum value_type { Number, Boolean, String, Object, Array, Null };

int query_table(WT_SESSION *session, const std::string &uri, const char *query_field,
  bool use_col_table, metrics &mtr) {

    int ret = 0;
    int exact = 0;
    uint16_t type;
    uint64_t id;
    uint64_t read_count = 0;
    uint64_t match_count = 0;
    uint64_t bytes_read = 0;
    double sum = 0;
    const char *key;
    WT_ITEM item;
    WT_CURSOR *cursor = nullptr;

    auto start = std::chrono::steady_clock::now();
    wt::open_cursor(session, uri.c_str(), &cursor);
    use_col_table ? cursor->set_key(cursor, query_field, 0) :
                    cursor->set_key(cursor, 0, query_field);
    if ((ret = cursor->search_near(cursor, &exact)) == 0) {
        if (exact == 0) {
            match_count++;
            cursor->get_value(cursor, &type, &item);
            bytes_read += (sizeof(type) + item.size);
        }
        read_count++;
        while ((ret = cursor->next(cursor)) == 0) {
            read_count++;
            if (use_col_table) {
                cursor->get_key(cursor, &key, &id);
                bytes_read += (strlen((const char*) key) + sizeof(id));
                if (strcmp(key, query_field))
                    break;
            } else {
                cursor->get_key(cursor, &id, &key);
                bytes_read += (strlen((const char*) key) + sizeof(id));
                if (strcmp(key, query_field))
                    continue;
            }
            cursor->get_value(cursor, &type, &item);
            bytes_read += (sizeof(type) + item.size);
            match_count++;
        }
    }
    cursor->close(cursor);
    auto stop = std::chrono::steady_clock::now();

    mtr.query_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count();
    mtr.read_count = read_count;
    mtr.bytes_read = bytes_read;
    mtr.match_count = match_count;

    return ret;
}

int get_last_row_insert_id(WT_SESSION *session, const std::string &uri, uint64_t *id) {

    int ret = 0;
    *id = 0;
    const char *key;
    WT_CURSOR *cursor = nullptr;
    if ((ret = session->open_cursor(session, uri.c_str(), nullptr, nullptr, &cursor)) == 0) {
        if ((ret = cursor->prev(cursor)) == 0) {
            ret = cursor->get_key(cursor, id, &key);
        }
    }
    cursor->close(cursor);
    return ret;
}

int open_cursor(WT_SESSION *session, const std::string &uri, WT_CURSOR **cursorp) {

    WT_CURSOR *cursor = nullptr;
    int ret = session->open_cursor(session, uri.c_str(), nullptr, nullptr, &cursor);
    *cursorp = cursor;
    return ret;    
}

int open_database(const std::string& path, WT_CONNECTION **connp, WT_SESSION **sessionp) {

    WT_CONNECTION *conn = nullptr;
    WT_SESSION *session = nullptr;

    // Open a connection to the database, creating it if necessary.
    int ret = wiredtiger_open(path.c_str(), nullptr, "create", &conn);
    assert(conn);

    // Open a session handle for the database.
    if (ret == 0) {
        conn->open_session(conn, NULL, NULL, &session);
        assert(session);
    }

    *connp = conn;
    *sessionp = session;
    return ret;
}

int row_table_insert(WT_CURSOR *cursor, uint64_t document_id, const std::string &key,
    uint16_t type, WT_ITEM item) {

    cursor->set_key(cursor, document_id, key.c_str());
    cursor->set_value(cursor, type, &item);
    return (cursor->insert(cursor));
}

int col_table_insert(WT_CURSOR *cursor, const std::string &key, uint64_t document_id,
    uint16_t type, WT_ITEM item) {

    cursor->set_key(cursor, key.c_str(), document_id);
    cursor->set_value(cursor, type, &item);
    return (cursor->insert(cursor));
}

int row_table_print(WT_SESSION *session, const std::string &uri) {

    int ret = 0;
    WT_CURSOR *cursor = nullptr;
    WT_ITEM item;
    uint64_t id;
    const char *key;
    uint16_t type;

    wt::open_cursor(session, uri, &cursor);
    while ((ret = cursor->next(cursor)) == 0) {
        cursor->get_key(cursor, &id, &key);
        cursor->get_value(cursor, &type, &item);
        std::cout << "id: " << id << " key: " << key;
        std::cout << " type: " << type << " value: " << (const char*) item.data << "\n";
    }
    cursor->close(cursor);

    return ret;
}

int col_table_print(WT_SESSION *session, const std::string &uri) {

    int ret = 0;
    WT_CURSOR *cursor = nullptr;
    WT_ITEM item;
    uint64_t id;
    const char *key;
    uint16_t type;

    open_cursor(session, uri, &cursor);
    while ((ret = cursor->next(cursor)) == 0) {
        cursor->get_key(cursor, &key, &id);
        cursor->get_value(cursor, &type, &item);
        std::cout << "key: " << key << " id: " << id;
        std::cout << " type: " << type << " value: " << (const char*) item.data << "\n";
    }
    cursor->close(cursor);

    return ret;
}

} // namespace wt
