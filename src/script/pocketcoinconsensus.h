// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POCKETCOIN_SCRIPT_POCKETCOINCONSENSUS_H
#define POCKETCOIN_SCRIPT_POCKETCOINCONSENSUS_H

#include <stdint.h>

#if defined(BUILD_POCKETCOIN_INTERNAL) && defined(HAVE_CONFIG_H)
#include <config/pocketcoin-config.h>
  #if defined(_WIN32)
    #if defined(DLL_EXPORT)
      #if defined(HAVE_FUNC_ATTRIBUTE_DLLEXPORT)
        #define EXPORT_SYMBOL __declspec(dllexport)
      #else
        #define EXPORT_SYMBOL
      #endif
    #endif
  #elif defined(HAVE_FUNC_ATTRIBUTE_VISIBILITY)
    #define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
  #endif
#elif defined(MSC_VER) && !defined(STATIC_LIBPOCKETCOINCONSENSUS)
  #define EXPORT_SYMBOL __declspec(dllimport)
#endif

#ifndef EXPORT_SYMBOL
  #define EXPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define POCKETCOINCONSENSUS_API_VER 1

typedef enum pocketcoinconsensus_error_t
{
    pocketcoinconsensus_ERR_OK = 0,
    pocketcoinconsensus_ERR_TX_INDEX,
    pocketcoinconsensus_ERR_TX_SIZE_MISMATCH,
    pocketcoinconsensus_ERR_TX_DESERIALIZE,
    pocketcoinconsensus_ERR_AMOUNT_REQUIRED,
    pocketcoinconsensus_ERR_INVALID_FLAGS,
} pocketcoinconsensus_error;

/** Script verification flags */
enum
{
    pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0), // evaluate P2SH (BIP16) subscripts
    pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2), // enforce strict DER (BIP66) compliance
    pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4), // enforce NULLDUMMY (BIP147)
    pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9), // enable CHECKLOCKTIMEVERIFY (BIP65)
    pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10), // enable CHECKSEQUENCEVERIFY (BIP112)
    pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS             = (1U << 11), // enable WITNESS (BIP141)
    pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_ALL                 = pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH | pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                               pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY | pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                                               pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY | pocketcoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS
};

/// Returns 1 if the input nIn of the serialized transaction pointed to by
/// txTo correctly spends the scriptPubKey pointed to by scriptPubKey under
/// the additional constraints specified by flags.
/// If not nullptr, err will contain an error/success code for the operation
EXPORT_SYMBOL int pocketcoinconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                                 const unsigned char *txTo        , unsigned int txToLen,
                                                 unsigned int nIn, unsigned int flags, pocketcoinconsensus_error* err);

EXPORT_SYMBOL int pocketcoinconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen, int64_t amount,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, pocketcoinconsensus_error* err);

EXPORT_SYMBOL unsigned int pocketcoinconsensus_version();

#ifdef __cplusplus
} // extern "C"
#endif

#undef EXPORT_SYMBOL

#endif // POCKETCOIN_SCRIPT_POCKETCOINCONSENSUS_H
