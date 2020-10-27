/*-
 * Copyright (c) 2014-2020 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include "json.hpp"
#include "wiredtiger.h"

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

using namespace nlohmann;

enum Type { BOOL, NUMBER, STRING, OBJECT, ARRAY, EMPTY };


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

void insert_column(std::string key, uint64_t id, json value) {
    Type t = convert_to_type(value.type_name());
    std::cout << "Inserting: " << key <<" " << id << " " << value << " " << t << std::endl;
}

void insert_row(uint64_t id, std::string key, json value) {
    Type t = convert_to_type(value.type_name());
    std::cout << "Inserting: " << id << " " << key <<" " << value << " " << t << std::endl;
}

std::unordered_map<std::string, json> process_sub_json(json jsn) {
    std::unordered_map<std::string, json> kvs{};

    return kvs;
}

void process_json(json jsn, uint64_t id) {
    std::cout << std::endl;
    for (auto it : jsn.at("data").at(0).items()) {
        //std::cout << it.value().type_name() << " " << it.key()  << std::endl;
        if (it.value().is_boolean()) {
            insert_row(id, it.key(), it.value());
            insert_column(it.key(), id, it.value());
        } else if (it.value().is_number()) {
            insert_row(id, it.key(), it.value());
            insert_column(it.key(), id, it.value());
        } else if (it.value().is_object()) {
            for (auto itt : process_sub_json(it.value())) {
                insert_row(id, itt.first, itt.second);
                insert_column(itt.first, id, itt.second);
            }
        } else if (it.value().is_string()) {
            insert_row(id, it.key(), it.value());
            insert_column(it.key(), id, it.value());
        } else if (it.value().is_array()) {
            insert_row(id, it.key(), it.value());
            insert_column(it.key(), id, it.value());
        } else if (it.value().is_null()) {

        }
    }
}

void load_file() {
    uint64_t id = 0;
    std::string line;
    std::ifstream ifs("../raw_data/rockbench_10rows.json");
    while (std::getline(ifs, line)) {
        process_json(json::parse(line), id);
        id++;
    }
}

int main()
{
    load_file();

    WT_CONNECTION *conn = nullptr;
    std::string dbpath = "wt_test";
    int ret = wiredtiger_open(dbpath.c_str(), nullptr, "create", &conn);
    if (ret != 0)
        std::cout << "wiredtiger_open failed with return code " << ret << '\n';
    else
        std::cout << "Tammy Rocks!\n";
    return 0;
}
