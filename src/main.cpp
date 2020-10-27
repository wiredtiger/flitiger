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

void process_json(json subdoc) {
    std::cout << std::endl;
    for (auto it : subdoc.at("data").at(0).items()) {
        std::cout << it.value().type_name() << " " << it.key()  << std::endl;
        if (it.value().is_boolean()) {

        } else if (it.value().is_number()) {

        } else if (it.value().is_object()) {

        } else if (it.value().is_string()) {

        } else if (it.value().is_array()) {

        } else if (it.value().is_null()) {

        }
    }
}

void load_file() {
    std::string line;
    std::ifstream ifs("../raw_data/rockbench_10rows.json");
    while (std::getline(ifs, line)) {
        process_json(json::parse(line));
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
