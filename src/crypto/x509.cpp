// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#include "crypto/x509.h"


x509::x509()
    : m_pkey(EVP_RSA_gen(2048)),
      m_cert(X509_new())
{
}

x509::~x509()
{
    EVP_PKEY_free(m_pkey);
    X509_free(m_cert);
}

void x509::Generate()
{
    ASN1_INTEGER_set(X509_get_serialNumber(m_cert), 1);
    X509_gmtime_adj(X509_get_notBefore(m_cert), 0);
    X509_gmtime_adj(X509_get_notAfter(m_cert), 31536000L);
    X509_set_pubkey(m_cert, m_pkey);

    auto name = X509_get_subject_name(m_cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
        (unsigned char*)"Pocketnet node", -1, -1, 0);
    X509_set_issuer_name(m_cert, name);
    X509_sign(m_cert, m_pkey, EVP_sha256());
}

x509::x509(x509&& other)
{
    this->m_pkey = other.m_pkey;
    this->m_cert = other.m_cert;
    other.m_cert = nullptr;
    other.m_pkey = nullptr;
}

x509& x509::operator=(x509&& other)
{
    X509_free(this->m_cert);
    EVP_PKEY_free(this->m_pkey);
    this->m_pkey = other.m_pkey;
    this->m_cert = other.m_cert;
    other.m_pkey = nullptr;
    other.m_cert = nullptr;
    return *this;
}

EVP_PKEY* x509::GetPKey()
{
    return m_pkey;
}

X509* x509::GetCert()
{
    return m_cert;
}

SSLContext::SSLContext(x509&& cert)
    : m_ctx(SSL_CTX_new(SSLv23_server_method())),
      m_cert(std::move(cert))
{
}

SSLContext::~SSLContext()
{
    SSL_CTX_free(m_ctx);
}

bool SSLContext::Setup()
{
    if (1 != SSL_CTX_use_certificate(m_ctx, m_cert.GetCert()))
        return false;

    if (1 != SSL_CTX_use_PrivateKey(m_ctx, m_cert.GetPKey()))
        return false;

    if (1 != SSL_CTX_check_private_key(m_ctx))
        return false;

    return true;
}

SSLContext::SSLContext(SSLContext&& other)
{
    this->m_cert = std::move(other.m_cert);
    this->m_ctx = other.m_ctx;
    other.m_ctx = nullptr;
}

SSLContext& SSLContext::operator=(SSLContext&& other)
{
    SSL_CTX_free(m_ctx);
    this->m_ctx = other.m_ctx;
    this->m_cert = std::move(other.m_cert);
    other.m_ctx = nullptr;
    return *this;
}

SSL_CTX* SSLContext::GetCtx()
{
    return m_ctx;
}

