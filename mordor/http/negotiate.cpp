// Copyright (c) 2009 - Decho Corporation

#include "negotiate.h"

#include "http.h"
#include "mordor/log.h"
#include "mordor/string.h"

#pragma comment(lib, "secur32.lib")

namespace Mordor {
namespace HTTP {

static Logger::ptr g_log = Log::lookup("mordor:http:negotiate");

NegotiateAuth::NegotiateAuth(const std::string &username,
                             const std::string &password)
    : m_username(toUtf16(username)),
      m_password(toUtf16(password))
{
    SecInvalidateHandle(&m_creds);
    SecInvalidateHandle(&m_secCtx);
    size_t pos = m_username.find(L'\\');
    if (pos != std::wstring::npos) {
        m_domain = m_username.substr(0, pos);
        m_username = m_username.substr(pos + 1);
    }
}

NegotiateAuth::~NegotiateAuth()
{
    if (SecIsValidHandle(&m_creds)) {
        FreeCredentialHandle(&m_creds);
        SecInvalidateHandle(&m_creds);
    }
    if (SecIsValidHandle(&m_secCtx)) {
        FreeCredentialHandle(&m_secCtx);
        SecInvalidateHandle(&m_secCtx);
    }
}

bool
NegotiateAuth::authorize(const AuthParams &challenge, AuthParams &authorization,
    const URI &uri)
{
    SECURITY_STATUS status;
    std::wstring packageW = toUtf16(challenge.scheme);
    std::string param = challenge.base64;

    std::string outboundBuffer;
    SecBufferDesc outboundBufferDesc;
    SecBuffer outboundSecBuffer;
    TimeStamp lifetime;
    ULONG contextAttributes;

    outboundBuffer.resize(4096);
    outboundBufferDesc.ulVersion = 0;
    outboundBufferDesc.cBuffers = 1;
    outboundBufferDesc.pBuffers = &outboundSecBuffer;
    outboundSecBuffer.BufferType = SECBUFFER_TOKEN;
    outboundSecBuffer.pvBuffer = &outboundBuffer[0];
    outboundSecBuffer.cbBuffer = (unsigned long)outboundBuffer.size();

    if (param.empty()) {
        // No response from server; we're starting a new session
        if (SecIsValidHandle(&m_creds))
            return false;

        SEC_WINNT_AUTH_IDENTITY_W id;
        id.User = (unsigned short *)m_username.c_str();
        id.UserLength = (unsigned long)m_username.size();
        id.Domain = (unsigned short *)m_domain.c_str();
        id.DomainLength = (unsigned long)m_domain.size();
        id.Password = (unsigned short *)m_password.c_str();
        id.PasswordLength = (unsigned long)m_password.size();
        id.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        status = AcquireCredentialsHandleW(NULL,
            (wchar_t *)packageW.c_str(),
            SECPKG_CRED_OUTBOUND,
            NULL,
            m_username.empty() ? NULL : &id,
            NULL,
            NULL,
            &m_creds,
            &lifetime);
        MORDOR_LOG_TRACE(g_log) << "AcquireCredentialsHandleW("
            << challenge.scheme << ", " << toUtf8(m_username) << "): ("
            << status << ")";
        if (!SUCCEEDED(status))
            MORDOR_THROW_EXCEPTION_FROM_ERROR_API(status, "AcquireCredentialsHandleW");

        status = InitializeSecurityContextW(
            &m_creds,
            NULL,
            (wchar_t *)toUtf16(uri.toString()).c_str(),
            ISC_REQ_CONFIDENTIALITY,
            0,
            SECURITY_NATIVE_DREP,
            NULL,
            0,
            &m_secCtx,
            &outboundBufferDesc,
            &contextAttributes,
            &lifetime);
        MORDOR_LOG_TRACE(g_log) << "InitializeSecurityContextW("
            << uri << ", {0}):  {" << outboundSecBuffer.cbBuffer << "} ("
            << status << ")";
    } else {
        // Prepare the response from the server
        std::string inboundBuffer = base64decode(param);
        SecBufferDesc inboundBufferDesc;
        SecBuffer inboundSecBuffer;

        inboundBufferDesc.ulVersion = 0;
        inboundBufferDesc.cBuffers = 1;
        inboundBufferDesc.pBuffers = &inboundSecBuffer;
        inboundSecBuffer.BufferType = SECBUFFER_TOKEN;
        inboundSecBuffer.pvBuffer = &inboundBuffer[0];
        inboundSecBuffer.cbBuffer = (unsigned long)inboundBuffer.size();

        status = InitializeSecurityContextW(
            &m_creds,
            &m_secCtx,
            (wchar_t *)toUtf16(uri.toString()).c_str(),
            ISC_REQ_CONFIDENTIALITY,
            0,
            SECURITY_NATIVE_DREP,
            &inboundBufferDesc,
            0,
            &m_secCtx,
            &outboundBufferDesc,
            &contextAttributes,
            &lifetime);
        MORDOR_LOG_TRACE(g_log) << "InitializeSecurityContextW("
            << uri << ", {" << inboundSecBuffer.cbBuffer << "}):  {"
            << outboundSecBuffer.cbBuffer << "} (" << status << ")";
    }

    if (status == SEC_I_COMPLETE_NEEDED ||
        status == SEC_I_COMPLETE_AND_CONTINUE) {
        status = CompleteAuthToken(&m_secCtx, &outboundBufferDesc);
        MORDOR_LOG_TRACE(g_log) << "CompleteAuthToken(): {"
            << outboundSecBuffer.cbBuffer << "} (" << status << ")";
    }

    if (!SUCCEEDED(status))
        MORDOR_THROW_EXCEPTION_FROM_ERROR(status);

    outboundBuffer.resize(outboundSecBuffer.cbBuffer);
    authorization.scheme = challenge.scheme;
    authorization.base64 = base64encode(outboundBuffer);
    authorization.parameters.clear();
    return true;
}

}}
