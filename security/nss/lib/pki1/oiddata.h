/* THIS IS A GENERATED FILE */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifndef OIDDATA_H
#define OIDDATA_H

#ifdef DEBUG
static const char OIDDATA_CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$ ; @(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef NSSPKI1T_H
#include "nsspki1t.h"
#endif /* NSSPKI1T_H */

extern const NSSOID nss_builtin_oids[];
extern const PRUint32 nss_builtin_oid_count;

/*extern const nssAttributeTypeAliasTable nss_attribute_type_aliases[];*/
/*extern const PRUint32 nss_attribute_type_alias_count;*/

enum NSSOIDTagEnum {
   NSS_OID_UNKNOWN = -1,
   NSS_OID_RFC1274_UID = 11,
   NSS_OID_RFC1274_EMAIL = 12,
   NSS_OID_RFC2247_DC = 13,
   NSS_OID_ANSIX9_DSA_SIGNATURE = 43,
   NSS_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST = 44,
   NSS_OID_X942_DIFFIE_HELLMAN_KEY = 47,
   NSS_OID_PKCS1_RSA_ENCRYPTION = 52,
   NSS_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION = 53,
   NSS_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION = 54,
   NSS_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION = 55,
   NSS_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION = 56,
   NSS_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC = 58,
   NSS_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC = 59,
   NSS_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC = 60,
   NSS_OID_PKCS7 = 61,
   NSS_OID_PKCS7_DATA = 62,
   NSS_OID_PKCS7_SIGNED_DATA = 63,
   NSS_OID_PKCS7_ENVELOPED_DATA = 64,
   NSS_OID_PKCS7_SIGNED_ENVELOPED_DATA = 65,
   NSS_OID_PKCS7_DIGESTED_DATA = 66,
   NSS_OID_PKCS7_ENCRYPTED_DATA = 67,
   NSS_OID_PKCS9_EMAIL_ADDRESS = 69,
   NSS_OID_PKCS9_UNSTRUCTURED_NAME = 70,
   NSS_OID_PKCS9_CONTENT_TYPE = 71,
   NSS_OID_PKCS9_MESSAGE_DIGEST = 72,
   NSS_OID_PKCS9_SIGNING_TIME = 73,
   NSS_OID_PKCS9_COUNTER_SIGNATURE = 74,
   NSS_OID_PKCS9_CHALLENGE_PASSWORD = 75,
   NSS_OID_PKCS9_UNSTRUCTURED_ADDRESS = 76,
   NSS_OID_PKCS9_EXTENDED_CERTIFICATE_ATTRIBUTES = 77,
   NSS_OID_PKCS9_SMIME_CAPABILITIES = 78,
   NSS_OID_PKCS9_FRIENDLY_NAME = 79,
   NSS_OID_PKCS9_LOCAL_KEY_ID = 80,
   NSS_OID_PKCS9_X509_CERT = 82,
   NSS_OID_PKCS9_SDSI_CERT = 83,
   NSS_OID_PKCS9_X509_CRL = 85,
   NSS_OID_PKCS12 = 86,
   NSS_OID_PKCS12_PBE_IDS = 87,
   NSS_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4 = 88,
   NSS_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4 = 89,
   NSS_OID_PKCS12_PBE_WITH_SHA1_AND_3_KEY_TRIPLE_DES_CBC = 90,
   NSS_OID_PKCS12_PBE_WITH_SHA1_AND_2_KEY_TRIPLE_DES_CBC = 91,
   NSS_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC = 92,
   NSS_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC = 93,
   NSS_OID_PKCS12_KEY_BAG = 120,
   NSS_OID_PKCS12_PKCS8_SHROUDED_KEY_BAG = 121,
   NSS_OID_PKCS12_CERT_BAG = 122,
   NSS_OID_PKCS12_CRL_BAG = 123,
   NSS_OID_PKCS12_SECRET_BAG = 124,
   NSS_OID_PKCS12_SAFE_CONTENTS_BAG = 125,
   NSS_OID_MD2 = 127,
   NSS_OID_MD4 = 128,
   NSS_OID_MD5 = 129,
   NSS_OID_RC2_CBC = 131,
   NSS_OID_RC4 = 132,
   NSS_OID_DES_EDE3_CBC = 133,
   NSS_OID_RC5_CBC_PAD = 134,
   NSS_OID_X509_AUTH_INFO_ACCESS = 154,
   NSS_OID_PKIX_CPS_POINTER_QUALIFIER = 156,
   NSS_OID_PKIX_USER_NOTICE_QUALIFIER = 157,
   NSS_OID_EXT_KEY_USAGE_SERVER_AUTH = 159,
   NSS_OID_EXT_KEY_USAGE_CLIENT_AUTH = 160,
   NSS_OID_EXT_KEY_USAGE_CODE_SIGN = 161,
   NSS_OID_EXT_KEY_USAGE_EMAIL_PROTECTION = 162,
   NSS_OID_EXT_KEY_USAGE_IPSEC_END_SYSTEM = 163,
   NSS_OID_EXT_KEY_USAGE_IPSEC_TUNNEL = 164,
   NSS_OID_EXT_KEY_USAGE_IPSEC_USER = 165,
   NSS_OID_EXT_KEY_USAGE_TIME_STAMP = 166,
   NSS_OID_OCSP_RESPONDER = 167,
   NSS_OID_PKIX_REGCTRL_REGTOKEN = 171,
   NSS_OID_PKIX_REGCTRL_AUTHENTICATOR = 172,
   NSS_OID_PKIX_REGCTRL_PKIPUBINFO = 173,
   NSS_OID_PKIX_REGCTRL_PKI_ARCH_OPTIONS = 174,
   NSS_OID_PKIX_REGCTRL_OLD_CERT_ID = 175,
   NSS_OID_PKIX_REGCTRL_PROTOCOL_ENC_KEY = 176,
   NSS_OID_PKIX_REGINFO_UTF8_PAIRS = 178,
   NSS_OID_PKIX_REGINFO_CERT_REQUEST = 179,
   NSS_OID_OID_PKIX_OCSP = 181,
   NSS_OID_PKIX_OCSP_BASIC_RESPONSE = 182,
   NSS_OID_PKIX_OCSP_NONCE = 183,
   NSS_OID_PKIX_OCSP_RESPONSE = 184,
   NSS_OID_PKIX_OCSP_CRL = 185,
   NSS_OID_X509_OCSP_NO_CHECK = 186,
   NSS_OID_PKIX_OCSP_ARCHIVE_CUTOFF = 187,
   NSS_OID_PKIX_OCSP_SERVICE_LOCATOR = 188,
   NSS_OID_DES_ECB = 198,
   NSS_OID_DES_CBC = 199,
   NSS_OID_DES_OFB = 200,
   NSS_OID_DES_CFB = 201,
   NSS_OID_DES_MAC = 202,
   NSS_OID_ISO_SHA_WITH_RSA_SIGNATURE = 203,
   NSS_OID_DES_EDE = 204,
   NSS_OID_SHA1 = 205,
   NSS_OID_BOGUS_DSA_SIGNATURE_WITH_SHA1_DIGEST = 206,
   NSS_OID_X520_COMMON_NAME = 231,
   NSS_OID_X520_SURNAME = 232,
   NSS_OID_X520_COUNTRY_NAME = 233,
   NSS_OID_X520_LOCALITY_NAME = 234,
   NSS_OID_X520_STATE_OR_PROVINCE_NAME = 235,
   NSS_OID_X520_ORGANIZATION_NAME = 236,
   NSS_OID_X520_ORGANIZATIONAL_UNIT_NAME = 237,
   NSS_OID_X520_TITLE = 238,
   NSS_OID_X520_NAME = 239,
   NSS_OID_X520_GIVEN_NAME = 240,
   NSS_OID_X520_INITIALS = 241,
   NSS_OID_X520_GENERATION_QUALIFIER = 242,
   NSS_OID_X520_DN_QUALIFIER = 243,
   NSS_OID_X500_RSA_ENCRYPTION = 249,
   NSS_OID_X509_SUBJECT_DIRECTORY_ATTR = 268,
   NSS_OID_X509_SUBJECT_DIRECTORY_ATTRIBUTES = 269,
   NSS_OID_X509_SUBJECT_KEY_ID = 270,
   NSS_OID_X509_KEY_USAGE = 271,
   NSS_OID_X509_PRIVATE_KEY_USAGE_PERIOD = 272,
   NSS_OID_X509_SUBJECT_ALT_NAME = 273,
   NSS_OID_X509_ISSUER_ALT_NAME = 274,
   NSS_OID_X509_BASIC_CONSTRAINTS = 275,
   NSS_OID_X509_CRL_NUMBER = 276,
   NSS_OID_X509_REASON_CODE = 277,
   NSS_OID_X509_HOLD_INSTRUCTION_CODE = 278,
   NSS_OID_X509_INVALID_DATE = 279,
   NSS_OID_X509_DELTA_CRL_INDICATOR = 280,
   NSS_OID_X509_ISSUING_DISTRIBUTION_POINT = 281,
   NSS_OID_X509_CERTIFICATE_ISSUER = 282,
   NSS_OID_X509_NAME_CONSTRAINTS = 283,
   NSS_OID_X509_CRL_DIST_POINTS = 284,
   NSS_OID_X509_CERTIFICATE_POLICIES = 285,
   NSS_OID_X509_POLICY_MAPPINGS = 286,
   NSS_OID_X509_AUTH_KEY_ID = 288,
   NSS_OID_X509_POLICY_CONSTRAINTS = 289,
   NSS_OID_X509_EXT_KEY_USAGE = 290,
   NSS_OID_MISSI_DSS_OLD = 314,
   NSS_OID_FORTEZZA_SKIPJACK = 315,
   NSS_OID_MISSI_KEA = 316,
   NSS_OID_MISSI_KEA_DSS_OLD = 317,
   NSS_OID_MISSI_DSS = 318,
   NSS_OID_MISSI_KEA_DSS = 319,
   NSS_OID_MISSI_ALT_KEY = 320,
   NSS_OID_NS_CERT_EXT_CERT_TYPE = 328,
   NSS_OID_NS_CERT_EXT_BASE_URL = 329,
   NSS_OID_NS_CERT_EXT_REVOCATION_URL = 330,
   NSS_OID_NS_CERT_EXT_CA_REVOCATION_URL = 331,
   NSS_OID_NS_CERT_EXT_CA_CRL_URL = 332,
   NSS_OID_NS_CERT_EXT_CA_CERT_URL = 333,
   NSS_OID_NS_CERT_EXT_CERT_RENEWAL_URL = 334,
   NSS_OID_NS_CERT_EXT_CA_POLICY_URL = 335,
   NSS_OID_NS_CERT_EXT_HOMEPAGE_URL = 336,
   NSS_OID_NS_CERT_EXT_ENTITY_LOGO = 337,
   NSS_OID_NS_CERT_EXT_USER_PICTURE = 338,
   NSS_OID_NS_CERT_EXT_SSL_SERVER_NAME = 339,
   NSS_OID_NS_CERT_EXT_COMMENT = 340,
   NSS_OID_NS_CERT_EXT_THAYES = 341,
   NSS_OID_NS_TYPE_GIF = 343,
   NSS_OID_NS_TYPE_JPEG = 344,
   NSS_OID_NS_TYPE_URL = 345,
   NSS_OID_NS_TYPE_HTML = 346,
   NSS_OID_NS_TYPE_CERT_SEQUENCE = 347,
   NSS_OID_NS_KEY_USAGE_GOVT_APPROVED = 350,
   NSS_OID_NETSCAPE_RECOVERY_REQUEST = 353,
   NSS_OID_NETSCAPE_SMIME_KEA = 355,
   NSS_OID_NETSCAPE_NICKNAME = 357,
   NSS_OID_VERISIGN_USER_NOTICES = 362,
   NSS_OID_NS_CERT_EXT_NETSCAPE_OK = 367,
   NSS_OID_NS_CERT_EXT_ISSUER_LOGO = 368,
   NSS_OID_NS_CERT_EXT_SUBJECT_LOGO = 369
};

typedef enum NSSOIDTagEnum NSSOIDTag;

#endif /* OIDDATA_H */
