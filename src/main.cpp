/*-
 * Copyright (c) 2014-2020 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <iostream>
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

int main()
{
    WT_CONNECTION *conn = nullptr;
    std::string dbpath = "wt_test";

    int ret = wiredtiger_open(dbpath.c_str(), nullptr, "create", &conn);
    if (ret != 0)
        std::cout << "wiredtiger_open failed with return code " << ret << '\n';
    else
        std::cout << "Tammy is awesome!\n";
    return 0;
}
