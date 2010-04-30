// Copyright (c) 2009 - Mozy, Inc.

#include "mordor/pch.h"

#include "basic.h"

#include "mordor/string.h"

namespace Mordor {
namespace HTTP {
namespace BasicAuth {

void authorize(AuthParams &authorization, const std::string &username,
    const std::string &password)
{
    authorization.scheme = "Basic";
    authorization.base64 = base64encode(username + ":" + password);
    authorization.parameters.clear();
}

}}}
