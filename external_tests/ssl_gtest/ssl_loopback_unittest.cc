/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ssl.h"
#include "sslerr.h"
#include "sslproto.h"

#include <memory>

#include "tls_parser.h"
#include "tls_filter.h"
#include "tls_connect.h"
#include "gtest_utils.h"

namespace nss_test {

class TlsServerKeyExchangeEcdhe {
 public:
  bool Parse(const DataBuffer& buffer) {
    TlsParser parser(buffer);

    uint8_t curve_type;
    if (!parser.Read(&curve_type)) {
      return false;
    }

    if (curve_type != 3) {  // named_curve
      return false;
    }

    uint32_t named_curve;
    if (!parser.Read(&named_curve, 2)) {
      return false;
    }

    return parser.ReadVariable(&public_key_, 1);
  }

  DataBuffer public_key_;
};

TEST_P(TlsConnectGeneric, SetupOnly) {}

TEST_P(TlsConnectGeneric, Connect) {
  SetExpectedVersion(std::get<1>(GetParam()));
  Connect();
  client_->CheckAuthType(ssl_auth_rsa);
}

TEST_P(TlsConnectGeneric, ConnectEcdsa) {
  SetExpectedVersion(std::get<1>(GetParam()));
  ResetEcdsa();
  Connect();
  client_->CheckAuthType(ssl_auth_ecdsa);
}

TEST_P(TlsConnectGeneric, ConnectFalseStart) {
  client_->EnableFalseStart();
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectResumed) {
  ConfigureSessionCache(RESUME_SESSIONID, RESUME_SESSIONID);
  Connect();

  ResetRsa();
  ExpectResumption(RESUME_SESSIONID);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectClientCacheDisabled) {
  ConfigureSessionCache(RESUME_NONE, RESUME_SESSIONID);
  Connect();
  ResetRsa();
  ExpectResumption(RESUME_NONE);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectServerCacheDisabled) {
  ConfigureSessionCache(RESUME_SESSIONID, RESUME_NONE);
  Connect();
  ResetRsa();
  ExpectResumption(RESUME_NONE);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectSessionCacheDisabled) {
  ConfigureSessionCache(RESUME_NONE, RESUME_NONE);
  Connect();
  ResetRsa();
  ExpectResumption(RESUME_NONE);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectResumeSupportBoth) {
  // This prefers tickets.
  ConfigureSessionCache(RESUME_BOTH, RESUME_BOTH);
  Connect();

  ResetRsa();
  ConfigureSessionCache(RESUME_BOTH, RESUME_BOTH);
  ExpectResumption(RESUME_TICKET);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectResumeClientTicketServerBoth) {
  // This causes no resumption because the client needs the
  // session cache to resume even with tickets.
  ConfigureSessionCache(RESUME_TICKET, RESUME_BOTH);
  Connect();

  ResetRsa();
  ConfigureSessionCache(RESUME_TICKET, RESUME_BOTH);
  ExpectResumption(RESUME_NONE);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectResumeClientBothTicketServerTicket) {
  // This causes a ticket resumption.
  ConfigureSessionCache(RESUME_BOTH, RESUME_TICKET);
  Connect();

  ResetRsa();
  ConfigureSessionCache(RESUME_BOTH, RESUME_TICKET);
  ExpectResumption(RESUME_TICKET);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectClientServerTicketOnly) {
  // This causes no resumption because the client needs the
  // session cache to resume even with tickets.
  ConfigureSessionCache(RESUME_TICKET, RESUME_TICKET);
  Connect();

  ResetRsa();
  ConfigureSessionCache(RESUME_TICKET, RESUME_TICKET);
  ExpectResumption(RESUME_NONE);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectClientBothServerNone) {
  ConfigureSessionCache(RESUME_BOTH, RESUME_NONE);
  Connect();

  ResetRsa();
  ConfigureSessionCache(RESUME_BOTH, RESUME_NONE);
  ExpectResumption(RESUME_NONE);
  Connect();
}

TEST_P(TlsConnectGeneric, ConnectClientNoneServerBoth) {
  ConfigureSessionCache(RESUME_NONE, RESUME_BOTH);
  Connect();

  ResetRsa();
  ConfigureSessionCache(RESUME_NONE, RESUME_BOTH);
  ExpectResumption(RESUME_NONE);
  Connect();
}

TEST_P(TlsConnectGeneric, ResumeWithHigherVersion) {
  EnsureTlsSetup();
  SetExpectedVersion(SSL_LIBRARY_VERSION_TLS_1_1);
  ConfigureSessionCache(RESUME_SESSIONID, RESUME_SESSIONID);
  client_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_1,
                           SSL_LIBRARY_VERSION_TLS_1_1);
  server_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_1,
                           SSL_LIBRARY_VERSION_TLS_1_1);
  Connect();

  ResetRsa();
  EnsureTlsSetup();
  SetExpectedVersion(SSL_LIBRARY_VERSION_TLS_1_2);
  client_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_1,
                           SSL_LIBRARY_VERSION_TLS_1_2);
  server_->SetVersionRange(SSL_LIBRARY_VERSION_TLS_1_1,
                           SSL_LIBRARY_VERSION_TLS_1_2);
  ExpectResumption(RESUME_NONE);
  Connect();
}

TEST_P(TlsConnectGeneric, ClientAuth) {
  client_->SetupClientAuth();
  server_->RequestClientAuth(true);
  Connect();
  server_->CheckAuthType(ssl_auth_rsa);
}

TEST_P(TlsConnectGeneric, ClientAuthEcdsa) {
  ResetEcdsa();
  client_->SetupClientAuth();
  server_->RequestClientAuth(true);
  Connect();
  server_->CheckAuthType(ssl_auth_ecdsa);
}

static const SSLSignatureAndHashAlg SignatureEcdsaSha384[] = {
  {ssl_hash_sha384, ssl_sign_ecdsa}
};
static const SSLSignatureAndHashAlg SignatureEcdsaSha256[] = {
  {ssl_hash_sha256, ssl_sign_ecdsa}
};
static const SSLSignatureAndHashAlg SignatureRsaSha384[] = {
  {ssl_hash_sha384, ssl_sign_rsa}
};
static const SSLSignatureAndHashAlg SignatureRsaSha256[] = {
  {ssl_hash_sha256, ssl_sign_rsa}
};

// When signature algorithms match up, this should connect successfully; even
// for TLS 1.1 and 1.0, where they should be ignored.
TEST_P(TlsConnectGeneric, SignatureAlgorithmServerAuth) {
  client_->SetSignatureAlgorithms(SignatureEcdsaSha384,
                                  PR_ARRAY_SIZE(SignatureEcdsaSha384));
  server_->SetSignatureAlgorithms(SignatureEcdsaSha384,
                                  PR_ARRAY_SIZE(SignatureEcdsaSha384));
  ResetEcdsa();
  Connect();
}

// Here the client picks a single option, which should work in all versions.
// Defaults on the server include the first option.
TEST_P(TlsConnectGeneric, SignatureAlgorithmClientOnly) {
  const SSLSignatureAndHashAlg clientAlgorithms[] = {
    {ssl_hash_sha384, ssl_sign_ecdsa},
    {ssl_hash_sha384, ssl_sign_rsa}, // supported but unusable
    {ssl_hash_md5, ssl_sign_ecdsa} // unsupported and ignored
  };
  client_->SetSignatureAlgorithms(clientAlgorithms,
                                  PR_ARRAY_SIZE(clientAlgorithms));
  ResetEcdsa();
  Connect();
}

// Here the server picks a single option, which should work in all versions.
// Defaults on the client include the provided option.
TEST_P(TlsConnectGeneric, SignatureAlgorithmServerOnly) {
  server_->SetSignatureAlgorithms(SignatureEcdsaSha384,
                                  PR_ARRAY_SIZE(SignatureEcdsaSha384));
  ResetEcdsa();
  Connect();
}

// There is no need for overlap on signatures; since we don't actually use the
// signatures for static RSA, this should still connect successfully.
// This should also work in TLS 1.0 and 1.1 where the algorithms aren't used.
TEST_P(TlsConnectGeneric, SignatureAlgorithmNoOverlapStaticRsa) {
  client_->SetSignatureAlgorithms(SignatureRsaSha384,
                                  PR_ARRAY_SIZE(SignatureRsaSha384));
  server_->SetSignatureAlgorithms(SignatureRsaSha256,
                                  PR_ARRAY_SIZE(SignatureRsaSha256));
  DisableDheCiphers();
  Connect();
  client_->CheckKEAType(ssl_kea_rsa);
  client_->CheckAuthType(ssl_auth_rsa);
}

// Signature algorithms governs both verification and generation of signatures.
// With ECDSA, we need to at least have a common signature algorithm configured.
TEST_P(TlsConnectTls12, SignatureAlgorithmNoOverlapEcdsa) {
  ResetEcdsa();
  client_->SetSignatureAlgorithms(SignatureEcdsaSha384,
                                  PR_ARRAY_SIZE(SignatureEcdsaSha384));
  server_->SetSignatureAlgorithms(SignatureEcdsaSha256,
                                  PR_ARRAY_SIZE(SignatureEcdsaSha256));
  ConnectExpectFail();
}

// Pre 1.2, a mismatch on signature algorithms shouldn't affect anything.
TEST_P(TlsConnectPre12, SignatureAlgorithmNoOverlapEcdsa) {
  ResetEcdsa();
  client_->SetSignatureAlgorithms(SignatureEcdsaSha384,
                                  PR_ARRAY_SIZE(SignatureEcdsaSha384));
  server_->SetSignatureAlgorithms(SignatureEcdsaSha256,
                                  PR_ARRAY_SIZE(SignatureEcdsaSha256));
  Connect();
}

// The server requests client auth but doesn't offer a SHA-256 option.
// This fails because NSS only uses SHA-256 for handshake transcript hashes.
TEST_P(TlsConnectTls12, RequestClientAuthWithoutSha256) {
  server_->SetSignatureAlgorithms(SignatureRsaSha384,
                                  PR_ARRAY_SIZE(SignatureRsaSha384));
  server_->RequestClientAuth(false);
  ConnectExpectFail();
}

TEST_P(TlsConnectGeneric, ConnectAlpn) {
  EnableAlpn();
  Connect();
  client_->CheckAlpn(SSL_NEXT_PROTO_SELECTED, "a");
  server_->CheckAlpn(SSL_NEXT_PROTO_NEGOTIATED, "a");
}

TEST_P(TlsConnectDatagram, ConnectSrtp) {
  EnableSrtp();
  Connect();
  CheckSrtp();
}

TEST_P(TlsConnectStream, ConnectAndClientRenegotiate) {
  Connect();
  server_->PrepareForRenegotiate();
  client_->StartRenegotiate();
  Handshake();
  CheckConnected();
}

TEST_P(TlsConnectStream, ConnectAndServerRenegotiate) {
  Connect();
  client_->PrepareForRenegotiate();
  server_->StartRenegotiate();
  Handshake();
  CheckConnected();
}

TEST_P(TlsConnectStream, ConnectEcdhe) {
  EnableSomeEcdheCiphers();
  Connect();
  client_->CheckKEAType(ssl_kea_ecdh);
}

TEST_P(TlsConnectStream, ConnectEcdheTwiceReuseKey) {
  EnableSomeEcdheCiphers();
  TlsInspectorRecordHandshakeMessage* i1 =
      new TlsInspectorRecordHandshakeMessage(kTlsHandshakeServerKeyExchange);
  server_->SetPacketFilter(i1);
  Connect();
  client_->CheckKEAType(ssl_kea_ecdh);
  TlsServerKeyExchangeEcdhe dhe1;
  EXPECT_TRUE(dhe1.Parse(i1->buffer()));

  // Restart
  ResetRsa();
  TlsInspectorRecordHandshakeMessage* i2 =
      new TlsInspectorRecordHandshakeMessage(kTlsHandshakeServerKeyExchange);
  server_->SetPacketFilter(i2);
  EnableSomeEcdheCiphers();
  ConfigureSessionCache(RESUME_NONE, RESUME_NONE);
  Connect();
  client_->CheckKEAType(ssl_kea_ecdh);

  TlsServerKeyExchangeEcdhe dhe2;
  EXPECT_TRUE(dhe2.Parse(i2->buffer()));

  // Make sure they are the same.
  EXPECT_EQ(dhe1.public_key_.len(), dhe2.public_key_.len());
  EXPECT_TRUE(!memcmp(dhe1.public_key_.data(), dhe2.public_key_.data(),
                      dhe1.public_key_.len()));
}

TEST_P(TlsConnectStream, ConnectEcdheTwiceNewKey) {
  EnableSomeEcdheCiphers();
  SECStatus rv =
      SSL_OptionSet(server_->ssl_fd(), SSL_REUSE_SERVER_ECDHE_KEY, PR_FALSE);
  EXPECT_EQ(SECSuccess, rv);
  TlsInspectorRecordHandshakeMessage* i1 =
      new TlsInspectorRecordHandshakeMessage(kTlsHandshakeServerKeyExchange);
  server_->SetPacketFilter(i1);
  Connect();
  client_->CheckKEAType(ssl_kea_ecdh);
  TlsServerKeyExchangeEcdhe dhe1;
  EXPECT_TRUE(dhe1.Parse(i1->buffer()));

  // Restart
  ResetRsa();
  EnableSomeEcdheCiphers();
  rv = SSL_OptionSet(server_->ssl_fd(), SSL_REUSE_SERVER_ECDHE_KEY, PR_FALSE);
  EXPECT_EQ(SECSuccess, rv);
  TlsInspectorRecordHandshakeMessage* i2 =
      new TlsInspectorRecordHandshakeMessage(kTlsHandshakeServerKeyExchange);
  server_->SetPacketFilter(i2);
  ConfigureSessionCache(RESUME_NONE, RESUME_NONE);
  Connect();
  client_->CheckKEAType(ssl_kea_ecdh);

  TlsServerKeyExchangeEcdhe dhe2;
  EXPECT_TRUE(dhe2.Parse(i2->buffer()));

  // Make sure they are different.
  EXPECT_FALSE((dhe1.public_key_.len() == dhe2.public_key_.len()) &&
               (!memcmp(dhe1.public_key_.data(), dhe2.public_key_.data(),
                        dhe1.public_key_.len())));
}

TEST_P(TlsConnectGeneric, ConnectSendReceive) {
  Connect();
  SendReceive();
}

// The next two tests takes advantage of the fact that we
// automatically read the first 1024 bytes, so if
// we provide 1200 bytes, they overrun the read buffer
// provided by the calling test.

// DTLS should return an error.
TEST_P(TlsConnectDatagram, ShortRead) {
  Connect();
  client_->SetExpectedReadError(true);
  server_->SendData(1200, 1200);
  WAIT_(client_->error_code() == SSL_ERROR_RX_SHORT_DTLS_READ, 2000);
  // Don't call CheckErrorCode() because it requires us to being
  // in state ERROR.
  ASSERT_EQ(SSL_ERROR_RX_SHORT_DTLS_READ, client_->error_code());

  // Now send and receive another packet.
  client_->SetExpectedReadError(false);
  server_->ResetSentBytes(); // Reset the counter.
  SendReceive();
}

// TLS should get the write in two chunks.
TEST_P(TlsConnectStream, ShortRead) {
  // This test behaves oddly with TLS 1.0 because of 1/n+1 splitting,
  // so skip in that case.
  if (version_ < SSL_LIBRARY_VERSION_TLS_1_1)
    return;

  Connect();
  server_->SendData(1200, 1200);
  // Read the first tranche.
  WAIT_(client_->received_bytes() == 1024, 2000);
  ASSERT_EQ(1024U, client_->received_bytes());
  // The second tranche should now immediately be available.
  client_->ReadBytes();
  ASSERT_EQ(1200U, client_->received_bytes());
}

INSTANTIATE_TEST_CASE_P(VariantsStream10, TlsConnectGeneric,
                        ::testing::Combine(
                          TlsConnectTestBase::kTlsModesStream,
                          TlsConnectTestBase::kTlsV10));
INSTANTIATE_TEST_CASE_P(VariantsAll, TlsConnectGeneric,
                        ::testing::Combine(
                          TlsConnectTestBase::kTlsModesAll,
                          TlsConnectTestBase::kTlsV11V12));
INSTANTIATE_TEST_CASE_P(VersionsDatagram, TlsConnectDatagram,
                        TlsConnectTestBase::kTlsV11V12);
INSTANTIATE_TEST_CASE_P(Variants12, TlsConnectTls12,
                        TlsConnectTestBase::kTlsModesAll);
INSTANTIATE_TEST_CASE_P(Pre12Stream, TlsConnectPre12,
                        ::testing::Combine(
                          TlsConnectTestBase::kTlsModesStream,
                          TlsConnectTestBase::kTlsV10));
INSTANTIATE_TEST_CASE_P(Pre12All, TlsConnectPre12,
                        ::testing::Combine(
                          TlsConnectTestBase::kTlsModesAll,
                          TlsConnectTestBase::kTlsV11));
INSTANTIATE_TEST_CASE_P(VersionsStream10, TlsConnectStream,
                        TlsConnectTestBase::kTlsV10);
INSTANTIATE_TEST_CASE_P(VersionsStream, TlsConnectStream,
                        TlsConnectTestBase::kTlsV11V12);

}  // namespace nspr_test
