// Copyright (c) 2009 - Decho Corporation

#include "mordor/json.h"
#include "mordor/test/test.h"

using namespace Mordor::JSON;

// Examples taken from http://www.ietf.org/rfc/rfc4627.txt

MORDOR_UNITTEST(JSON, example1construct)
{
    Object root;
    Object image;
    image.insert(std::make_pair("Width", Value(800ll)));
    image.insert(std::make_pair("Height", Value(600ll)));
    image.insert(std::make_pair("Title", Value(std::string("View from 15th Floor"))));
    Object thumbnail;
    thumbnail.insert(std::make_pair("Url", Value(std::string("http://www.example.com/image/481989943"))));
    thumbnail.insert(std::make_pair("Height", Value(125ll)));
    thumbnail.insert(std::make_pair("Width", Value(std::string("100"))));
    image.insert(std::make_pair("Thumbnail", thumbnail));
    Array ids;
    ids.push_back(Value(116ll));
    ids.push_back(Value(943ll));
    ids.push_back(Value(234ll));
    ids.push_back(Value(38793ll));
    image.insert(std::make_pair("IDs", ids));
    root.insert(std::make_pair("Image", image));

    std::ostringstream os;
    os << root;
    MORDOR_TEST_ASSERT_EQUAL(os.str(),
        "{\n"
        "    \"Image\" : {\n"
        "        \"Height\" : 600,\n"
        "        \"IDs\" : [\n"
        "            116,\n"
        "            943,\n"
        "            234,\n"
        "            38793\n"
        "        ],\n"
        "        \"Thumbnail\" : {\n"
        "            \"Height\" : 125,\n"
        "            \"Url\" : \"http://www.example.com/image/481989943\",\n"
        "            \"Width\" : \"100\"\n"
        "        },\n"
        "        \"Title\" : \"View from 15th Floor\",\n"
        "        \"Width\" : 800\n"
        "    }\n"
        "}");
}

MORDOR_UNITTEST(JSON, example1)
{
    Value root;
    Parser parser(root);
    parser.run(
        "{\n"
        "    \"Image\": {\n"
        "        \"Width\":  800,\n"
        "        \"Height\": 600,\n"
        "        \"Title\":  \"View from 15th Floor\",\n"
        "        \"Thumbnail\": {\n"
        "          \"Url\":    \"http://www.example.com/image/481989943\",\n"
        "          \"Height\": 125,\n"
        "          \"Width\":  \"100\"\n"
        "        },\n"
        "        \"IDs\": [116, 943, 234, 38793]\n"
        "    }\n"
        "}");
    MORDOR_ASSERT(parser.final());
    MORDOR_ASSERT(!parser.error());

    const Object &rootObject = boost::get<Object>(root);
    MORDOR_TEST_ASSERT_EQUAL(rootObject.size(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(rootObject.begin()->first, "Image");
    const Object &imageObject = boost::get<Object>(rootObject.begin()->second);
    MORDOR_TEST_ASSERT_EQUAL(imageObject.size(), 5u);
    Object::const_iterator it = imageObject.find("Width");
    MORDOR_ASSERT(it != imageObject.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<long long>(it->second), 800);
    it = imageObject.find("Height");
    MORDOR_ASSERT(it != imageObject.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<long long>(it->second), 600);
    it = imageObject.find("Title");
    MORDOR_ASSERT(it != imageObject.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it->second), "View from 15th Floor");
    it = imageObject.find("Thumbnail");
    MORDOR_ASSERT(it != imageObject.end());
    const Object &thumbnailObject = boost::get<Object>(it->second);
    MORDOR_TEST_ASSERT_EQUAL(thumbnailObject.size(), 3u);
    it = thumbnailObject.find("Url");
    MORDOR_ASSERT(it != thumbnailObject.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it->second), "http://www.example.com/image/481989943");
    it = thumbnailObject.find("Height");
    MORDOR_ASSERT(it != thumbnailObject.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<long long>(it->second), 125);
    it = thumbnailObject.find("Width");
    MORDOR_ASSERT(it != thumbnailObject.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it->second), "100");
    it = imageObject.find("IDs");
    MORDOR_ASSERT(it != imageObject.end());
    const Array &ids = boost::get<Array>(it->second);
    MORDOR_TEST_ASSERT_EQUAL(ids.size(), 4u);
    Array::const_iterator it2 = ids.begin();
    MORDOR_TEST_ASSERT_EQUAL(boost::get<long long>(*it2++), 116);
    MORDOR_TEST_ASSERT_EQUAL(boost::get<long long>(*it2++), 943);
    MORDOR_TEST_ASSERT_EQUAL(boost::get<long long>(*it2++), 234);
    MORDOR_TEST_ASSERT_EQUAL(boost::get<long long>(*it2++), 38793);

    // Now verify again, using the convenience syntax
    MORDOR_TEST_ASSERT(!root.empty());
    MORDOR_TEST_ASSERT(!root.isBlank());
    MORDOR_TEST_ASSERT_EQUAL(root.size(), 1u);
    MORDOR_TEST_ASSERT_EQUAL(root.begin()->first, "Image");
    MORDOR_TEST_ASSERT(root["Garbage"].isBlank());
    MORDOR_TEST_ASSERT_EQUAL(root["Image"].size(), 5u);
    MORDOR_TEST_ASSERT_EQUAL(root["Image"]["Width"].get<long long>(), 800);
    MORDOR_TEST_ASSERT_EQUAL(root["Image"]["Title"].get<std::string>(), "View from 15th Floor");
    MORDOR_TEST_ASSERT_EQUAL(root["Image"]["IDs"].size(), 4u);
    MORDOR_TEST_ASSERT_EQUAL(root["Image"]["IDs"][0].get<long long>(), 116);

    MORDOR_TEST_ASSERT_EXCEPTION(root.get<long long>(), boost::bad_get);
    MORDOR_TEST_ASSERT_EXCEPTION(root[0], boost::bad_get);
    MORDOR_TEST_ASSERT_EXCEPTION(root["Image"]["IDs"]["Garbage"], boost::bad_get);
    MORDOR_TEST_ASSERT_EXCEPTION(root["Image"]["Width"].empty(), boost::bad_get);
    MORDOR_TEST_ASSERT_EXCEPTION(root["Image"]["Width"].size(), boost::bad_get);

    std::ostringstream os;
    os << root;
    MORDOR_TEST_ASSERT_EQUAL(os.str(),
        "{\n"
        "    \"Image\" : {\n"
        "        \"Height\" : 600,\n"
        "        \"IDs\" : [\n"
        "            116,\n"
        "            943,\n"
        "            234,\n"
        "            38793\n"
        "        ],\n"
        "        \"Thumbnail\" : {\n"
        "            \"Height\" : 125,\n"
        "            \"Url\" : \"http://www.example.com/image/481989943\",\n"
        "            \"Width\" : \"100\"\n"
        "        },\n"
        "        \"Title\" : \"View from 15th Floor\",\n"
        "        \"Width\" : 800\n"
        "    }\n"
        "}");
}

MORDOR_UNITTEST(JSON, example2)
{
    Value root;
    Parser parser(root);
    parser.run(
        "[\n"
        "    {\n"
        "        \"precision\": \"zip\",\n"
        "        \"Latitude\":  37.7668,\n"
        "        \"Longitude\": -122.3959,\n"
        "        \"Address\":   \"\",\n"
        "        \"City\":      \"SAN FRANCISCO\",\n"
        "        \"State\":     \"CA\",\n"
        "        \"Zip\":       \"94107\",\n"
        "        \"Country\":   \"US\"\n"
        "    },\n"
        "    {\n"
        "        \"precision\": \"zip\",\n"
        "        \"Latitude\":  37.371991,\n"
        "        \"Longitude\": -122.026020,\n"
        "        \"Address\":   \"\",\n"
        "        \"City\":      \"SUNNYVALE\",\n"
        "        \"State\":     \"CA\",\n"
        "        \"Zip\":       \"94085\",\n"
        "        \"Country\":   \"US\"\n"
        "    }\n"
        "]");
    MORDOR_ASSERT(parser.final());
    MORDOR_ASSERT(!parser.error());

    const Array &rootArray = boost::get<Array>(root);
    MORDOR_TEST_ASSERT_EQUAL(rootArray.size(), 2u);
    Array::const_iterator it = rootArray.begin();
    const Object &object1 = boost::get<Object>(*it);
    MORDOR_TEST_ASSERT_EQUAL(object1.size(), 8u);
    Object::const_iterator it2 = object1.find("precision");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "zip");
    it2 = object1.find("precision");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "zip");
    it2 = object1.find("Latitude");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<double>(it2->second), 37.7668);
    it2 = object1.find("Longitude");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<double>(it2->second), -122.3959);
    it2 = object1.find("Address");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_ASSERT(boost::get<std::string>(it2->second).empty());
    it2 = object1.find("City");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "SAN FRANCISCO");
    it2 = object1.find("State");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "CA");
    it2 = object1.find("Zip");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "94107");
    it2 = object1.find("Country");
    MORDOR_ASSERT(it2 != object1.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "US");
    const Object &object2 = boost::get<Object>(*++it);
    MORDOR_TEST_ASSERT_EQUAL(object2.size(), 8u);
    it2 = object2.find("precision");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "zip");
    it2 = object2.find("precision");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "zip");
    it2 = object2.find("Latitude");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<double>(it2->second), 37.371991);
    it2 = object2.find("Longitude");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<double>(it2->second), -122.026020);
    it2 = object2.find("Address");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_ASSERT(boost::get<std::string>(it2->second).empty());
    it2 = object2.find("City");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "SUNNYVALE");
    it2 = object2.find("State");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "CA");
    it2 = object2.find("Zip");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "94085");
    it2 = object2.find("Country");
    MORDOR_ASSERT(it2 != object2.end());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(it2->second), "US");

    std::ostringstream os;
    os << root;
    MORDOR_TEST_ASSERT_EQUAL(os.str(),
        "[\n"
        "    {\n"
        "        \"Address\" : \"\",\n"
        "        \"City\" : \"SAN FRANCISCO\",\n"
        "        \"Country\" : \"US\",\n"
        "        \"Latitude\" : 37.7668,\n"
        "        \"Longitude\" : -122.396,\n"
        "        \"State\" : \"CA\",\n"
        "        \"Zip\" : \"94107\",\n"
        "        \"precision\" : \"zip\"\n"
        "    },\n"
        "    {\n"
        "        \"Address\" : \"\",\n"
        "        \"City\" : \"SUNNYVALE\",\n"
        "        \"Country\" : \"US\",\n"
        "        \"Latitude\" : 37.372,\n"
        "        \"Longitude\" : -122.026,\n"
        "        \"State\" : \"CA\",\n"
        "        \"Zip\" : \"94085\",\n"
        "        \"precision\" : \"zip\"\n"
        "    }\n"
        "]");
}

MORDOR_UNITTEST(JSON, escaping)
{
    MORDOR_TEST_ASSERT_EQUAL(quote("\"\\\b\f\n\r\t\x1b"), "\"\\\"\\\\\\b\\f\\n\\r\\t\\u001b\"");
    MORDOR_TEST_ASSERT_EQUAL(unquote("\"\\\"\\\\\\b\\f\\n\\r\\t\\u001b\\/\""), "\"\\\b\f\n\r\t\x1b/");
}

MORDOR_UNITTEST(JSON, unicodeEscape)
{
    Value root;
    Parser parser(root);
    parser.run(
        "\"\\u2028(2).docx\"");
    MORDOR_ASSERT(parser.final());
    MORDOR_ASSERT(!parser.error());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(root), "\xe2\x80\xa8(2).docx");
}

MORDOR_UNITTEST(JSON, forwardSlashEscape)
{
    Value root;
    Parser parser(root);
    parser.run("\"\\/\"");
    MORDOR_ASSERT(parser.final());
    MORDOR_ASSERT(!parser.error());
    MORDOR_TEST_ASSERT_EQUAL(boost::get<std::string>(root), "/");
}

MORDOR_UNITTEST(JSON, emptyStuff)
{
    Value root;
    Parser parser(root);
    parser.run("{}");
    MORDOR_ASSERT(parser.final());
    MORDOR_ASSERT(!parser.error());
    MORDOR_TEST_ASSERT(boost::get<Object>(root).empty());

    parser.run("[]");
    MORDOR_ASSERT(parser.final());
    MORDOR_ASSERT(!parser.error());
    MORDOR_TEST_ASSERT(boost::get<Array>(root).empty());

    parser.run("\"\"");
    MORDOR_ASSERT(parser.final());
    MORDOR_ASSERT(!parser.error());
    MORDOR_TEST_ASSERT(boost::get<std::string>(root).empty());
}
