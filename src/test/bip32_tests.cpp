// Copyright (c) 2013-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <key.h>
#include <key_io.h>
#include <uint256.h>
#include <util.h>
#include <utilstrencodings.h>
#include <test/test_pocketcoin.h>

#include <string>
#include <vector>

struct TestDerivation {
    std::string pub;
    std::string prv;
    unsigned int nChild;
};

struct TestVector {
    std::string strHexMaster;
    std::vector<TestDerivation> vDerive;

    explicit TestVector(std::string strHexMasterIn) : strHexMaster(strHexMasterIn) {}

    TestVector& operator()(std::string pub, std::string prv, unsigned int nChild) {
        vDerive.push_back(TestDerivation());
        TestDerivation &der = vDerive.back();
        der.pub = pub;
        der.prv = prv;
        der.nChild = nChild;
        return *this;
    }
};

TestVector test1 =
  TestVector("000102030405060708090a0b0c0d0e0f")
    ("7UxezJrmM5XabLZLohjjdUioDcJK9j2RXBZbrtmYLocd69SMwXNs9uPj1JduyjpeXbU5DiGc3cnGk4vLReLBL6Kc4M1B2RWStHDkFNujKCwMbRGd",
     "7UxcKNdmzg23hWBnWVFfANhGDFANRAzb2n6t1XYck1EDUb6pxeaY1MrQkkqbVtZ1E5af9nji77qFbRcz7qozBo5mQiyAwdoif7hEeco8aAiRCwCX",
     0x80000000)
    ("7UxezJu2m5Et4RpTnrLsLgUfrVLRG3qysZe5saNyEmi1D7dZDtfALKdG6RnGPX6DELmykiUtcV1pAoZQWcj9U3MLSQtdkd9F9CXeTtqrsWc1ayYL",
     "7UxcKNg3QfjMAbSuVdrnsaT8r8CUXVp9PABN2DA3dyKbbZJ2F1rqBn5wqsywufpMZVJ37VnboV8aToypzZa1uSg3bX3FThgPoMewYKomRbqaKfsn",
     1)
    ("7UxezJwCtH2RwpXLrgnunQjZj65Jv7DxyZfpAWhe5Myn24GMS8CfU3RgQPMAcwAfokih6gCWfGSGGv9tzD2tHWgWL6YTgvM5zgAejXaPeobY3XnU",
     "7UxcKNiDXsWu3z9nZUJqKJi2iiwNBZC8VAD6K9UiUZbNQVvpTFQLKVtN9qYr95sYd6Dp9wsMVHxAttgnQhWY8a1qaJCjuDTMXRQiPzc5o8jgBoc4",
     0x80000002)
    ("7UxezJypAKZFoXQCGYxhsfKy6T5M4LNqVqDQ9rEskcK5BE8isY3rarBWqHniYVBreCb8R2icPspEtJ6TxfN2aDn3s9zcXWF48m7iRFGJntr4voX6",
     "7UxcKNkpov3iuh2dyLUdQZJS65wQKnM11RkgJV1x9ovfZfoBtfFXSJeCajzQ4dumF4TKeTLNstR7pK6cJviGtA2kmRJBZa4v5aLxLbsq1mQBzyzs",
     2)
    ("7UxezK23Z9zNkhssFdSBbMbBBLaWNRC8EToiMHim3sGbeYaHUu9UwuxDCVNBUAWr2cPH9W2ja1piqLqN5DnNaK5V4sb5nR2cKbJcFFxDe3VRHpet",
     "7UxcKNo4CkUqrsWJxQx78FZeAySZdsAHk4LzVvVqT4tC2zEkW2M9oNQtwwZrzKEAhX7j5X8WaWLMPDv4UPuBLMTJ7cgmbyK1B8Rn3krJBGrF6VGw",
     1000000000)
    ("7UxezK3mKdfyzq1FT9my3U8jueHWNDRhYHJXP5y9c7YynkefycGtLSNTrkULXoLkyTeMryj27XAedt34fpCmbQvyuirEuHa2QPnBt4uqZHaQna8w",
     "7UxcKNpmyEAT6zdh9wHtaN7CuH9ZdfPs3sqoXikE1KAaBCK8zjUZBtq9cCg23x5wuS2rA8XuqHhLL4xbbQ5BhtXJqjeR9JomQFLXm3BrY1wf2THF",
     0);

TestVector test2 =
  TestVector("fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542")
    ("7UxezJrmM5XabLZLohMExassW4W2uyDZwsBrgaML56ZMv7pUopMtM5YVwjY5a5vjWJiSBcpqqXSvuTzqgN7uZDWXDUvAyHSeM7iEuhHQRUwG5dnv",
     "7UxcKNdmzg23hWBnWUsAVUrLVhN6BRBjTTj8qD8QUJAxJZUwpwZZCY1BhBjm6EcnHG6DF2nTXs7i3hj2JwLiHqWA7LLVi6DnuG39PC3koXcX8X5a",
     0)
    ("7UxezJv36MFGYpaAtDcqFifFP8JSSLBKCaALy8X5SGNPNCELmWLLKeor1tNepfPhQ6oScq9L1Pfves5geXHLF6fDdpWqFZVP224PHpmX5v5Ybchr",
     "7UxcKNh3jwjjezCcb18kncdiNmAVhn9UiAhd7mJ9qTyykdtondY1B7GXmLaLLp845uA45SShMm87n8tSSHrM7PnfAGZf76QCxeXK8KTfMtwvuZXD",
     0xFFFFFFFF)
    ("7UxezJwC9brJ5CULzPV3S97F2be7zivY77eWfDg9yBmJ8qy79xdZnGhGL6SSRGmwWrY3icfFZ3WnGAJ7FBxCasZhkYfHt6a15NneJh9ngksWd1to",
     "7UxcKNiCoCLmBN6nhAzxy35i2EWBGAthciBnorTENPNtXHdaB5qEdj9x5Ye7wRUY4dQnZLg2FY9KRBtwYj65t8AGHqs6fX8SJR33P7EztfLFB7jz",
     1)
    ("7UxezJz181sH1N7eBV6iaCmPbZb4E4TQcDqRvY7eq2pxU9rTbdTRwaCo2zLpo7a8eeQk1Cqi6t9sVoRR5GkTbnU8hozkngrCvbL5WcvTe1Q3Rjnz",
     "7UxcKNm1mcMk7Xk5tGce76jrbCT7VWRa7pNi5AtjEESYrbWvckf6o2fUnSYWKGGjgf7zFedNxHSPobScm3ns3B3nfrLuxety7G91mb3Rd8wF7ndQ",
     0xFFFFFFFE)
    ("7UxezK1B9vqDMyWbtn3xAwFD8w6kQyBsyGp4RZpNiZYeszcir5oksfsj3WsBtkEG9qnGTAHMjgewCr2jvvPczUMFDXMTExkXR9dAvV9YDYL3KJsk",
     "7UxcKNnBoXKgU993bZZshqDg8ZxogRA3UsMLaCbT7mAFGSHBsD1Rj8LQny4sQtyU5HYNYCCfebGAnypCLruxQgzhAYBb8UqWRQM3PvdS9CmvwLRW",
     2)
    ("7UxezK2YBtGRsVgv8XK94fKLpur8PNFR5X8yWDyzbyL1NdQaSA16SqWeohS8tpdLRyMd9gLZxxaScjjkvQpgVJSw7qcJpbvMW5zTfASjQD134XeM",
     "7UxcKNoYqUktyfKMqJq4bZHopYiBepDab7gFerm51Awbm553THCmJHyLZ9dpQyP95SZTcEWXNt1ocRv32uTqvRK4PbAKrHh1vTmB7czmhHMPaptL",
     0);

TestVector test3 =
  TestVector("4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be")
    ("7UxezJrmM5XabLZLogQh87GNJGVHwqru3d41xHh6dVrPUPWZbnc7cjgjugoK6ZvGV8K1pnubhMSourffXoRYU4DZeuFq7QgYZrGgkUzv81PG6gVf",
     "7UxcKNdmzg23hWBnWTvcf1EqHuMMDHq4ZDbJ6vUB2hTyrqB2cuonUC9Rf8zzcicWTD7Sa7V8HS3Q9c9m2HzKc4wJY7ozxcWMYTUVMUmRABWseR8W",
      0x80000000)
    ("7UxezJu8YpVQj1vi9swnXLQhoKk3cYDWRmuRrhBLaK65KKpUy3dF5KBWmmG6ose6zrfcZzH6Y8QZehcs2RtUozTVqUW745nVTQ8BYyBH2fpHpPKa",
     "7UxcKNg9CQysqBZ9rfTi4EPAnxc6szBfwNSi1KxQyWhfhmUwzApuvmeCXDTnL2NroZ4mVZdmhSzC2FtPUwXxke9tHsaCb3eGbCdmAbnNp8zecgp7",
      0);

static void RunTest(const TestVector &test) {
    std::vector<unsigned char> seed = ParseHex(test.strHexMaster);
    CExtKey key;
    CExtPubKey pubkey;
    key.SetSeed(seed.data(), seed.size());
    pubkey = key.Neuter();
    for (const TestDerivation &derive : test.vDerive) {
        unsigned char data[74];
        key.Encode(data);
        pubkey.Encode(data);

        // Test private key
        BOOST_CHECK(EncodeExtKey(key) == derive.prv);
        BOOST_CHECK(DecodeExtKey(derive.prv) == key); //ensure a base58 decoded key also matches

        // Test public key
        BOOST_CHECK(EncodeExtPubKey(pubkey) == derive.pub);
        BOOST_CHECK(DecodeExtPubKey(derive.pub) == pubkey); //ensure a base58 decoded pubkey also matches

        // Derive new keys
        CExtKey keyNew;
        BOOST_CHECK(key.Derive(keyNew, derive.nChild));
        CExtPubKey pubkeyNew = keyNew.Neuter();
        if (!(derive.nChild & 0x80000000)) {
            // Compare with public derivation
            CExtPubKey pubkeyNew2;
            BOOST_CHECK(pubkey.Derive(pubkeyNew2, derive.nChild));
            BOOST_CHECK(pubkeyNew == pubkeyNew2);
        }
        key = keyNew;
        pubkey = pubkeyNew;

        CDataStream ssPub(SER_DISK, CLIENT_VERSION);
        ssPub << pubkeyNew;
        BOOST_CHECK(ssPub.size() == 75);

        CDataStream ssPriv(SER_DISK, CLIENT_VERSION);
        ssPriv << keyNew;
        BOOST_CHECK(ssPriv.size() == 75);

        CExtPubKey pubCheck;
        CExtKey privCheck;
        ssPub >> pubCheck;
        ssPriv >> privCheck;

        BOOST_CHECK(pubCheck == pubkeyNew);
        BOOST_CHECK(privCheck == keyNew);
    }
}

BOOST_FIXTURE_TEST_SUITE(bip32_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(bip32_test1) {
    RunTest(test1);
}

BOOST_AUTO_TEST_CASE(bip32_test2) {
    RunTest(test2);
}

BOOST_AUTO_TEST_CASE(bip32_test3) {
    RunTest(test3);
}


BOOST_AUTO_TEST_SUITE_END()
