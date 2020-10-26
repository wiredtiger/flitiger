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
#include <streambuf>
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

int load_file() {
    std::ifstream ifs("../raw_data/rockbench_1row.json");
    json jf = json::parse(ifs);
    std::cout << jf.at("data");
    return 0;
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
