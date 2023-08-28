// Copyright (c) 2023 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCOIN_X509_H
#define POCKETCOIN_X509_H

#include <openssl/x509.h>
#include <openssl/ssl.h>

#include <memory>


class x509
{
public:
    x509();
    ~x509();
    x509(x509 const&) = delete;
    x509(x509&& other);
    x509& operator=(x509&& other);
    x509& operator=(x509 const&) = delete;
    void Generate();
    EVP_PKEY* GetPKey();
    X509* GetCert();

private:
    EVP_PKEY* m_pkey;
    X509* m_cert;
};

class SSLContext
{
public:
    SSLContext(x509&& cert);
    ~SSLContext();
    SSLContext(SSLContext const&) = delete;
    SSLContext(SSLContext&& other);
    SSLContext& operator=(SSLContext&& other);
    SSLContext& operator=(SSLContext const&) = delete;

    bool Setup();
    SSL_CTX* GetCtx();

private:
    SSL_CTX* m_ctx;
    x509 m_cert;
};

#endif // POCKETCOIN_X509_H
