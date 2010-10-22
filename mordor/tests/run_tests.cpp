// Copyright (c) 2009 - Decho Corporation

#include <iostream>

#include "mordor/config.h"
#include "mordor/main.h"
#include "mordor/version.h"
#include "mordor/statistics.h"
#include "mordor/test/antxmllistener.h"
#include "mordor/test/stdoutlistener.h"

using namespace Mordor;
using namespace Mordor::Test;

static ConfigVar<std::string>::ptr g_xmlDirectory = Config::lookup<std::string>(
    "test.antxml.directory", std::string(), "Location to put XML files");

MORDOR_MAIN(int argc, char *argv[])
{
    Config::loadFromEnvironment();

    boost::shared_ptr<TestListener> listener;
    std::string xmlDirectory = g_xmlDirectory->val();
    if (!xmlDirectory.empty()) {
        if (xmlDirectory == ".")
            xmlDirectory.clear();
        listener.reset(new AntXMLListener(xmlDirectory));
    } else {
        listener.reset(new StdoutListener());
    }
    bool result;
    if (argc > 1) {
        result = runTests(testsForArguments(argc - 1, argv + 1), *listener);
    } else {
        result = runTests(*listener);
    }
    std::cout << Statistics::dump();
    return result ? 0 : 1;
}
