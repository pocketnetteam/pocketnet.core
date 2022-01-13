// Copyright (c) 2018-2022 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

/**
 * See https://www.boost.org/doc/libs/1_71_0/libs/test/doc/html/boost_test/utf_reference/link_references/link_boost_test_module_macro.html
 */
#define BOOST_TEST_MODULE Pocketcoin Core Test Suite

#include <boost/test/unit_test.hpp>

//#include <test/util/setup_common.h>

#include <iostream>

/** Redirect debug log to unit_test.log files */
const std::function<void(const std::string&)> G_TEST_LOG_FUN = [](const std::string& s) {
    static const bool should_log{std::any_of(
        &boost::unit_test::framework::master_test_suite().argv[1],
        &boost::unit_test::framework::master_test_suite().argv[boost::unit_test::framework::master_test_suite().argc],
        [](const char* arg) {
            return std::string{"DEBUG_LOG_OUT"} == arg;
        })};
    if (!should_log) return;
    std::cout << s;
};
