// Copyright (c) 2009 - Decho Corporation

#include "basic.h"

#include "http.h"
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

void authorize(Request &nextRequest,
    const std::string &username, const std::string &password,
    bool proxy)
{ authorize(proxy ? nextRequest.request.proxyAuthorization :
    nextRequest.request.authorization, username, password); }


}}}
