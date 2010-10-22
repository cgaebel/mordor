// Copyright (c) 2009 - Decho Corporation

#include "config.h"

#include <algorithm>

#include "json.h"
#include "string.h"
#include "util.h"

#ifdef WINDOWS
#include "iomanager.h"
#else
#ifndef OSX
extern char **environ;
#endif
#endif

#ifdef OSX
#include <crt_externs.h>
#endif

namespace Mordor {

void
Config::loadFromEnvironment()
{
#ifdef WINDOWS
    wchar_t *enviro = GetEnvironmentStringsW();
    if (!enviro)
        return;
    boost::shared_ptr<wchar_t> environScope(enviro, &FreeEnvironmentStringsW);
    for (const wchar_t *env = enviro; *env; env += wcslen(env) + 1) {
        const wchar_t *equals = wcschr(env, '=');
        if (!equals)
            continue;
        if (equals == env)
            continue;
        std::string key(toUtf8(env, equals - env));
        std::string value(toUtf8(equals + 1));
#else
#ifdef OSX
	char **environ = *_NSGetEnviron();
#endif
    if (!environ)
        return;
    for (const char *env = *environ; *env; env += strlen(env) + 1) {
        const char *equals = strchr(env, '=');
        if (!equals)
            continue;
        if (equals == env)
            continue;
        std::string key(env, equals - env);
        std::string value(equals + 1);
#endif
        std::transform(key.begin(), key.end(), key.begin(), tolower);
        replace(key, '_', '.');
        if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyz.") != std::string::npos)
            continue;
        ConfigVarBase::ptr var = lookup(key);
        if (var)
            var->fromString(value);
    }
}

namespace {
class JSONVisitor : public boost::static_visitor<>
{
public:
    void operator()(const JSON::Object &object)
    {
        std::string prefix;
        if (!m_current.empty())
            prefix = m_current + '.';
        for (JSON::Object::const_iterator it(object.begin());
            it != object.end();
            ++it) {
            std::string key = it->first;
            std::transform(key.begin(), key.end(), key.begin(), tolower);
            if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyz") != std::string::npos)
                continue;
            m_toCheck.push_back(std::make_pair(prefix + key, &it->second));
        }
    }

    void operator()(const JSON::Array &array) const
    {
        // Ignore it
    }

    void operator()(const boost::blank &null) const
    {
        (*this)(std::string());
    }

    void operator()(const std::string &string) const
    {
        if (!m_current.empty()) {
            ConfigVarBase::ptr var = Config::lookup(m_current);
            if (var)
                var->fromString(string);
        }
    }

    template <class T> void operator()(const T &t) const
    {
        (*this)(boost::lexical_cast<std::string>(t));
    }

    std::list<std::pair<std::string, const JSON::Value *> > m_toCheck;
    std::string m_current;
};
}

void
Config::loadFromJSON(const JSON::Value &json)
{
    JSONVisitor visitor;
    visitor.m_toCheck.push_back(std::make_pair(std::string(), &json));
    while (!visitor.m_toCheck.empty()) {
        std::pair<std::string, const JSON::Value *> current =
            visitor.m_toCheck.front();
        visitor.m_toCheck.pop_front();
        visitor.m_current = current.first;
        boost::apply_visitor(visitor, *current.second);
    }
}

ConfigVarBase::ptr
Config::lookup(const std::string &name)
{
    ConfigVarSet::iterator it = vars().find(name);
    if (it != vars().end())
        return *it;
    return ConfigVarBase::ptr();
}

void
Config::visit(boost::function<void (ConfigVarBase::ptr)> dg)
{
    for (ConfigVarSet::const_iterator it = vars().begin();
        it != vars().end();
        ++it) {
        dg(*it);
    }
}

#ifdef WINDOWS
static void loadFromRegistry(HKEY hKey)
{
    std::string buffer;
    std::wstring valueName;
    DWORD type;
    DWORD index = 0;
    DWORD valueNameSize, size;
    LSTATUS status = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        &valueNameSize, &size, NULL, NULL);
    if (status)
        MORDOR_THROW_EXCEPTION_FROM_ERROR_API(status, "RegQueryInfoKeyW");
    valueName.resize(std::max<DWORD>(valueNameSize + 1, 1u));
    buffer.resize(std::max<DWORD>(size, 1u));
    while (true) {
        valueNameSize = (DWORD)valueName.size();
        size = (DWORD)buffer.size();
        status = RegEnumValueW(hKey, index++, &valueName[0], &valueNameSize,
            NULL, &type, (LPBYTE)&buffer[0], &size);
        if (status == ERROR_NO_MORE_ITEMS)
            break;
        if (status == ERROR_MORE_DATA)
            continue;
        if (status)
            MORDOR_THROW_EXCEPTION_FROM_ERROR_API(status, "RegEnumValueW");
        switch (type) {
            case REG_DWORD:
                if (size != 4)
                    continue;
                break;
            case REG_QWORD:
                if (size != 8)
                    continue;
                break;
            case REG_EXPAND_SZ:
            case REG_SZ:
                break;
            default:
                continue;
        }
        std::string varName = toUtf8(valueName.c_str(), valueNameSize);
        ConfigVarBase::ptr var = Config::lookup(varName);
        if (var) {
            std::string data;
            switch (type) {
                case REG_DWORD:
                    data = boost::lexical_cast<std::string>(
                        *(DWORD *)&buffer[0]);
                    break;
                case REG_QWORD:
                    data = boost::lexical_cast<std::string>(
                        *(long long *)&buffer[0]);
                    break;
                case REG_EXPAND_SZ:
                case REG_SZ:
                    if (((wchar_t *)&buffer[0])[size / sizeof(wchar_t) - 1] ==
                        L'\0')
                        size -= sizeof(wchar_t);
                    data = toUtf8((wchar_t *)&buffer[0],
                        size / sizeof(wchar_t));
                    break;
            }
            var->fromString(data);
        }
    }
}

Config::RegistryMonitor::RegistryMonitor(IOManager &ioManager,
    HKEY hKey, const std::wstring &subKey)
    : m_ioManager(ioManager),
      m_hKey(NULL),
      m_hEvent(NULL)
{
    LSTATUS status = RegOpenKeyExW(hKey, subKey.c_str(), 0,
        KEY_QUERY_VALUE | KEY_NOTIFY, &m_hKey);
    if (status)
        MORDOR_THROW_EXCEPTION_FROM_ERROR_API(status, "RegOpenKeyExW");
    try {
        m_hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!m_hEvent)
            MORDOR_THROW_EXCEPTION_FROM_LAST_ERROR_API("CreateEventW");
        status = RegNotifyChangeKeyValue(m_hKey, FALSE,
            REG_NOTIFY_CHANGE_LAST_SET, m_hEvent, TRUE);
        if (status)
            MORDOR_THROW_EXCEPTION_FROM_ERROR_API(status,
                "RegNotifyChangeKeyValue");
    } catch (...) {
        if (m_hKey)
            RegCloseKey(m_hKey);
        if (m_hEvent)
            CloseHandle(m_hEvent);
        throw;
    }
}

Config::RegistryMonitor::~RegistryMonitor()
{
    m_ioManager.unregisterEvent(m_hEvent);
    RegCloseKey(m_hKey);
    CloseHandle(m_hEvent);
}

void
Config::RegistryMonitor::onRegistryChange(
    boost::weak_ptr<RegistryMonitor> self)
{
    RegistryMonitor::ptr strongSelf = self.lock();
    if (strongSelf) {
        LSTATUS status = RegNotifyChangeKeyValue(strongSelf->m_hKey, FALSE,
            REG_NOTIFY_CHANGE_LAST_SET, strongSelf->m_hEvent, TRUE);
        if (status)
            MORDOR_THROW_EXCEPTION_FROM_ERROR_API(status,
                "RegNotifyChangeKeyValue");
        Mordor::loadFromRegistry(strongSelf->m_hKey);
    }
}

void
Config::loadFromRegistry(HKEY hKey, const std::string &subKey)
{
    loadFromRegistry(hKey, toUtf16(subKey));
}

void
Config::loadFromRegistry(HKEY hKey, const std::wstring &subKey)
{
    HKEY localKey;
    LSTATUS status = RegOpenKeyExW(hKey, subKey.c_str(), 0, KEY_QUERY_VALUE,
        &localKey);
    if (status)
        MORDOR_THROW_EXCEPTION_FROM_ERROR_API(status, "RegOpenKeyExW");
    try {
        Mordor::loadFromRegistry(localKey);
    } catch (...) {
        RegCloseKey(localKey);
        throw;
    }
    RegCloseKey(localKey);
}

Config::RegistryMonitor::ptr
Config::monitorRegistry(IOManager &ioManager, HKEY hKey,
    const std::string &subKey)
{
    return monitorRegistry(ioManager, hKey, toUtf16(subKey));
}

Config::RegistryMonitor::ptr
Config::monitorRegistry(IOManager &ioManager, HKEY hKey,
    const std::wstring &subKey)
{
    RegistryMonitor::ptr result(new RegistryMonitor(ioManager, hKey, subKey));
    // Have to wait until after the object is constructed to get the weak_ptr
    // we need
    ioManager.registerEvent(result->m_hEvent,
        boost::bind(&RegistryMonitor::onRegistryChange,
            boost::weak_ptr<RegistryMonitor>(result)), true);
    Mordor::loadFromRegistry(result->m_hKey);
    return result;
}
#endif

}
