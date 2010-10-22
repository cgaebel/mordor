#ifndef __MORDOR_HTTP_PARSER_H__
#define __MORDOR_HTTP_PARSER_H__
// Copyright (c) 2009 - Decho Corporation

#include "http.h"
#include "mordor/ragel.h"

namespace Mordor {
namespace HTTP {

class Parser : public RagelParserWithStack
{
public:
    void init();
protected:
    Parser() { init(); }

    const char *earliestPointer() const;
    void adjustPointers(ptrdiff_t offset);

    // Pointers to current headers
    std::string *m_string;
    StringSet *m_set;
    std::vector<std::string> *m_list;
    ParameterizedList *m_parameterizedList;
    AcceptList *m_acceptList;
    AcceptListWithParameters *m_acceptListWithParams;
    StringMap *m_parameters;
    AuthParams *m_auth;
    ChallengeList *m_challengeList;
    unsigned long long *m_ulong;
    ETag *m_eTag;
    Product m_product;
    std::set<ETag> *m_eTagSet;
    ProductAndCommentList *m_productAndCommentList;
    boost::posix_time::ptime *m_date;

    // Temp storage
    std::string m_temp1, m_temp2, m_genericHeaderName;
    const char *mark2;
    ETag m_tempETag;
};

class RequestParser : public Parser
{
public:
    RequestParser(Request& request);

    void init();
    bool final() const;
    bool error() const;
protected:
    void exec();
private:
    Request *m_request;
    Version *m_ver;
    URI *m_uri;
    std::vector<std::string> *m_segments;
    URI::Authority *m_authority;
    GeneralHeaders *m_general;
    EntityHeaders *m_entity;
};

class ResponseParser : public Parser
{
public:
    ResponseParser(Response& response);

    void init();
    bool final() const;
    bool error() const;
protected:
    void exec();
private:
    Response *m_response;
    Version *m_ver;
    URI *m_uri;
    std::vector<std::string> *m_segments;
    URI::Authority *m_authority;
    GeneralHeaders *m_general;
    EntityHeaders *m_entity;
};

class TrailerParser : public Parser
{
public:
    TrailerParser(EntityHeaders& entity);

    void init();
    bool final() const;
    bool error() const;
protected:
    void exec();
private:
    EntityHeaders *m_entity;
};

class ListParser : public RagelParserWithStack
{
public:
    ListParser(StringSet &stringSet);

    void init();
    bool final() const;
    bool error() const;
protected:
    void exec();
private:
    StringSet *m_set;
    std::vector<std::string> *m_list;
};

}}

#endif
