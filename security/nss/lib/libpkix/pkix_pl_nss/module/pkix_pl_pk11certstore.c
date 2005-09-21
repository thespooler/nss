/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * pkix_pl_pk11certstore.c
 *
 * PKCS11CertStore Function Definitions
 *
 */

#include "pkix_pl_pk11certstore.h"

/* --Private-Pk11CertStore-Functions---------------------------------- */

/*
 * FUNCTION: pkix_pl_Pk11CertStore_CheckTrust
 * DESCRIPTION:
 * This function checks the trust status of this "cert" that was retrieved
 * from the CertStore "store" and returns its trust status at "pTrusted".
 *
 * PARAMETERS:
 * "store"
 *      Address of the CertStore. Must be non-NULL.
 * "cert"
 *      Address of the Cert. Must be non-NULL.
 * "pTrusted"
 *      Address of PKIX_Boolean where the "cert" trust status is returned.
 *      Must be non-NULL.
 * "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_Pk11CertStore_CheckTrust(
        PKIX_CertStore *store,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pTrusted,
        void *plContext)
{
        CERTCertTrust nssTrusted;
        SECStatus rv = SECFailure;
        PKIX_Boolean trusted = PKIX_FALSE;
        PKIX_UInt32 trustedValues = 0;

        PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_CheckTrust");
        PKIX_NULLCHECK_THREE(store, cert, pTrusted);
        PKIX_NULLCHECK_ONE(cert->nssCert);

        trustedValues = CERTDB_TRUSTED_CA | CERTDB_VALID_CA;

        PKIX_CERT_DEBUG("\t\tCalling CERT_GetCertTrust).\n");
        rv = CERT_GetCertTrust(cert->nssCert, &nssTrusted);
        if (SECSuccess == rv) {
                if (nssTrusted.sslFlags & trustedValues ||
                    nssTrusted.emailFlags & trustedValues ||
                    nssTrusted.objectSigningFlags & trustedValues) {
                        trusted = PKIX_TRUE;
                }
        }

        *pTrusted = trusted;

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_Pk11CertStore_CertQuery
 * DESCRIPTION:
 *
 *  This function obtains from the database the Certs specified by the
 *  ComCertSelParams pointed to by "params" and stores the resulting
 *  List at "pSelected". If no matching Certs are found, a NULL pointer
 *  will be stored.
 *
 *  This function uses a "smart" database query if the Subject has been set
 *  in ComCertSelParams. Otherwise, it uses a very inefficient call to
 *  retrieve all Certs in the database (and run them through the selector).
 *
 * PARAMETERS:
 * "params"
 *      Address of the ComCertSelParams. Must be non-NULL.
 * "pSelected"
 *      Address at which List will be stored. Must be non-NULL.
 * "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_Pk11CertStore_CertQuery(
        PKIX_ComCertSelParams *params,
        PKIX_List **pSelected,
        void *plContext)
{
        PRBool validOnly = PR_FALSE;
        PRTime prtime = 0;
        PKIX_PL_X500Name *subjectName = NULL;
        PKIX_PL_Date *certValid = NULL;
        PKIX_List *certList = NULL;
        PKIX_PL_Cert *cert = NULL;
        CERTCertList *pk11CertList = NULL;
        CERTCertListNode *node = NULL;
        CERTCertificate *nssCert = NULL;
        CERTCertDBHandle *dbHandle = NULL;

        PRArenaPool *arena = NULL;
        SECItem *nameItem = NULL;
        void *wincx = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_CertQuery");
        PKIX_NULLCHECK_TWO(params, pSelected);

        /* avoid multiple calls to retrieve a constant */
        PKIX_PL_NSSCALLRV(CERTSTORE, dbHandle, CERT_GetDefaultCertDB, ());

        /*
         * Any of the ComCertSelParams may be obtained and used to constrain
         * the database query, to allow the use of a "smart" query. See
         * pkix_certsel.h for a list of the PKIX_ComCertSelParams_Get*
         * calls available. No corresponding "smart" queries exist at present,
         * except for CERT_CreateSubjectCertList based on Subject. When others
         * are added, corresponding code should be added to
         * pkix_pl_Pk11CertStore_CertQuery to use them when appropriate
         * selector parameters have been set.
         */

        PKIX_CHECK(PKIX_ComCertSelParams_GetSubject
                (params, &subjectName, plContext),
                "PKIX_ComCertSelParams_GetSubject failed");

        PKIX_CHECK(PKIX_ComCertSelParams_GetCertificateValid
                (params, &certValid, plContext),
                "PKIX_ComCertSelParams_GetCertificateValid failed");

        /* If caller specified a Date, convert it to PRTime */
        if (certValid) {
                PKIX_CHECK(pkix_pl_Date_GetPRTime
                        (certValid, &prtime, plContext),
                        "pkix_pl_Date_GetPRTime failed");
                validOnly = PR_TRUE;
        }

        /*
         * If we have the subject name for the desired subject,
         * ask the database for Certs with that subject. Otherwise
         * ask the database for all Certs.
         */
        if (subjectName) {
                arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
                if (arena) {

                        PKIX_CHECK(pkix_pl_X500Name_GetSECName
                                (subjectName, arena, &nameItem, plContext),
                                "pkix_pl_X500Name_GetSECName failed");

                        if (nameItem) {

                            PKIX_PL_NSSCALLRV
                                (CERTSTORE,
                                pk11CertList,
                                CERT_CreateSubjectCertList,
                                (NULL, dbHandle, nameItem, prtime, validOnly));
                        }
                        PKIX_PL_NSSCALL
                                (CERTSTORE, PORT_FreeArena, (arena, PR_FALSE));
                }

        } else {

                PKIX_CHECK(pkix_pl_NssContext_GetWincx(plContext, &wincx),
                        "pkix_pl_NssContext_GetWincx failed");

                PKIX_PL_NSSCALLRV
                        (CERTSTORE,
                        pk11CertList,
                        PK11_ListCerts,
                        (PK11CertListAll, wincx));
        }

        if (pk11CertList) {

                PKIX_CHECK(PKIX_List_Create(&certList, plContext),
                        "PKIX_List_Create failed");

                for (node = CERT_LIST_HEAD(pk11CertList);
                    !(CERT_LIST_END(node, pk11CertList));
                    node = CERT_LIST_NEXT(node)) {

                        PKIX_PL_NSSCALLRV
                                (CERTSTORE,
                                nssCert,
                                CERT_NewTempCertificate,
                                        (dbHandle,
                                        &(node->cert->derCert),
                                        NULL, /* nickname */
                                        PR_FALSE,
                                        PR_TRUE)); /* copyDER */

                        if (!nssCert) {
                                continue; /* just skip bad certs */
                        }

                        PKIX_CHECK_ONLY_FATAL(pkix_pl_Cert_CreateWithNSSCert
                                (nssCert, &cert, plContext),
                                "pkix_pl_Cert_CreateWithNSSCert failed");

                        if (PKIX_ERROR_RECEIVED) {
                                continue; /* just skip bad certs */
                        }

                        PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                (certList, (PKIX_PL_Object *)cert, plContext),
                                "PKIX_List_AppendItem failed");

                        PKIX_DECREF(cert);

                }

                /* Don't throw away the list if one cert was bad! */
                pkixTempErrorReceived = PKIX_FALSE;
        }

        *pSelected = certList;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(certList);
                if (arena) {
                        PKIX_PL_NSSCALL
                                (CERTSTORE, PORT_FreeArena, (arena, PR_FALSE));
                }
        }

        if (pk11CertList) {
                PKIX_PL_NSSCALL
                        (CERTSTORE, CERT_DestroyCertList, (pk11CertList));
        }

        PKIX_DECREF(subjectName);
        PKIX_DECREF(certValid);
        PKIX_DECREF(cert);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_Pk11CertStore_CrlQuery
 * DESCRIPTION:
 *
 *  This function obtains from the database the CRLs specified by the
 *  ComCRLSelParams pointed to by "params" and stores the List at "pSelected".
 *  If no Crls are found matching the criteria a NULL pointer is stored.
 *
 *  This function uses a "smart" database query if IssuerNames has been set
 *  in ComCertSelParams. Otherwise, it would have to use a very inefficient
 *  call to retrieve all Crls in the database (and run them through the
 *  selector). In addition to being inefficient, this usage would cause a
 *  memory leak because we have no mechanism at present for releasing a List
 *  of Crls that occupy the same arena. Therefore this function returns a
 *  CertStore Error if the selector has not been provided with parameters
 *  that allow for a "smart" query. (Currently, only the Issuer Name meets
 *  this requirement.)
 *
 * PARAMETERS:
 * "params"
 *      Address of the ComCRLSelParams. Must be non-NULL.
 * "pSelected"
 *      Address at which List will be stored. Must be non-NULL.
 * "plContext"
 *      Platform-specific context pointer
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertStore Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_Pk11CertStore_CrlQuery(
        PKIX_ComCRLSelParams *params,
        PKIX_List **pSelected,
        void *plContext)
{
        PKIX_UInt32 nameIx = 0;
        PKIX_UInt32 numNames = 0;
        PKIX_List *issuerNames = NULL;
        PKIX_PL_X500Name *issuer = NULL;
        PRArenaPool *arena = NULL;
        SECItem *nameItem = NULL;
        SECStatus rv = SECFailure;
        PKIX_List *crlList = NULL;
        PKIX_PL_CRL *crl = NULL;
        CRLDPCache* dpcache = NULL;
        CERTSignedCrl** crls = NULL;
        PRBool writeLocked = PR_FALSE;
        PRUint16 status = 0;
        void *wincx = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_CrlQuery");
        PKIX_NULLCHECK_TWO(params, pSelected);

        PKIX_CHECK(pkix_pl_NssContext_GetWincx(plContext, &wincx),
                "pkix_pl_NssContext_GetWincx failed");

        /*
         * If we have <info> for <a smart query>,
         * ask the database for Crls meeting those constraints.
         */
        PKIX_CHECK(PKIX_ComCRLSelParams_GetIssuerNames
                (params, &issuerNames, plContext),
                "PKIX_ComCRLSelParams_GetIssuerNames failed");

        /*
         * The specification for PKIX_ComCRLSelParams_GetIssuerNames in
         * pkix_crlsel.h says that if the IssuerNames is not set we get a null
         * pointer. If the user set IssuerNames to an empty List he has
         * provided a criterion impossible to meet ("must match at least one
         * of the names in the List").
         */
        if (issuerNames) {

            PKIX_CHECK(PKIX_List_Create(&crlList, plContext),
                        "PKIX_List_Create failed");

            PKIX_CHECK(PKIX_List_GetLength
                        (issuerNames, &numNames, plContext),
                        "PKIX_List_GetLength failed");
            arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
            if (arena) {

                for (nameIx = 0; nameIx < numNames; nameIx++) {
                    PKIX_CHECK(PKIX_List_GetItem
                        (issuerNames,
                        nameIx,
                        (PKIX_PL_Object **)&issuer,
                        plContext),
                        "PKIX_List_GetItem failed");
                    PKIX_CHECK(pkix_pl_X500Name_GetSECName
                        (issuer, arena, &nameItem, plContext),
                        "pkix_pl_X500Name_GetSECName failed");
                    if (nameItem) {
                        /*
                         * Successive calls append CRLs to
                         * the end of the list. If failure,
                         * no CRLs were appended.
                         */
                        PKIX_PL_NSSCALLRV
                            (CERTSTORE, rv, AcquireDPCache,
                            (NULL,
                            nameItem,
                            NULL,
                            0,
                            wincx,
                            &dpcache,
                            &writeLocked));

                        PKIX_PL_NSSCALLRV
                            (CERTSTORE, rv, DPCache_GetAllCRLs,
                            (dpcache, arena, &crls, &status));

                        if ((status & (~CRL_CACHE_INVALID_CRLS)) != 0) {
                            PKIX_ERROR("Fetching Cached CRLfailed");
                        }

                        while (*crls != NULL) {

                            PKIX_CHECK_ONLY_FATAL
                                (pkix_pl_CRL_CreateWithSignedCRL
                                (*crls, &crl, plContext),
                                "pkix_pl_CRL_CreateWithSignedCRL failed");

                            if (PKIX_ERROR_RECEIVED) {
                                PKIX_DECREF(crl);
                                crls++;
                                continue; /* just skip bad certs */
                            }

                            PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                (crlList, (PKIX_PL_Object *)crl, plContext),
                                "PKIX_List_AppendItem failed");

                            PKIX_DECREF(crl);
                            crls++;
                        }

                        /* Don't throw away the list if one cert was bad! */
                        pkixTempErrorReceived = PKIX_FALSE;
                        
                    }
                    PKIX_DECREF(issuer);
                }

            }
        } else {
                PKIX_ERROR("Insufficient criteria for Crl Query");
        }

        *pSelected = crlList;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(crlList);
        }

        PKIX_PL_NSSCALL(CERTSTORE, ReleaseDPCache, (dpcache, writeLocked));

        if (arena) {
                PKIX_PL_NSSCALL
                        (CERTSTORE, PORT_FreeArena, (arena, PR_FALSE));
        }

        PKIX_DECREF(issuerNames);
        PKIX_DECREF(issuer);
        PKIX_DECREF(crl);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_Pk11CertStore_GetCert
 *  (see description of PKIX_CertStore_CertCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_Pk11CertStore_GetCert(
        PKIX_CertStore *store,
        PKIX_CertSelector *selector,
        PKIX_List **pCertList,
        void *plContext)
{
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_PL_Cert *candidate = NULL;
        PKIX_List *selected = NULL;
        PKIX_List *filtered = NULL;
        PKIX_CertSelector_MatchCallback callback = NULL;
        PKIX_CertStore_CheckTrustCallback trustCallback = NULL;
        PKIX_ComCertSelParams *params = NULL;
        PKIX_Boolean pass = PKIX_TRUE;
        PKIX_Boolean cacheFlag = PKIX_FALSE;

        PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_GetCert");
        PKIX_NULLCHECK_THREE(store, selector, pCertList);

        PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                (selector, &callback, plContext),
                "PKIX_CertSelector_GetMatchCallback failed");

        PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                (selector, &params, plContext),
                "PKIX_CertSelector_GetComCertSelParams failed");

        PKIX_CHECK(pkix_pl_Pk11CertStore_CertQuery
                (params, &selected, plContext),
                "pkix_pl_Pk11CertStore_CertQuery failed");

        if (selected) {
                PKIX_CHECK(PKIX_List_GetLength(selected, &numFound, plContext),
                        "PKIX_List_GetLength failed");
        }

        PKIX_CHECK(PKIX_CertStore_GetCertStoreCacheFlag
                (store, &cacheFlag, plContext),
                "PKIX_CertStore_GetCertStoreCacheFlag failed");

        PKIX_CHECK(PKIX_CertStore_GetTrustCallback
                (store, &trustCallback, plContext),
                "PKIX_CertStore_GetTrustCallback failed");

        PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                "PKIX_List_Create failed");

        for (i = 0; i < numFound; i++) {
                PKIX_CHECK_ONLY_FATAL(PKIX_List_GetItem
                        (selected,
                        i,
                        (PKIX_PL_Object **)&candidate,
                        plContext),
                        "PKIX_List_GetItem failed");

                if (PKIX_ERROR_RECEIVED) {
                        continue; /* just skip bad certs */
                }

                PKIX_CHECK_ONLY_FATAL(callback
                        (selector, candidate, &pass, plContext),
                        "certSelector failed");

                if (!(PKIX_ERROR_RECEIVED) && pass) {

                        PKIX_CHECK(PKIX_PL_Cert_SetCacheFlag
                                (candidate, cacheFlag, plContext),
                                "PKIX_PL_Cert_SetCacheFlag failed");

                        if (trustCallback) {
                                PKIX_CHECK(PKIX_PL_Cert_SetTrustCertStore
                                    (candidate, store, plContext),
                                    "PKIX_PL_Cert_SetTrustCertStore failed");
                        }

                        PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                (filtered,
                                (PKIX_PL_Object *)candidate,
                                plContext),
                                "PKIX_List_AppendItem failed");
                }

                PKIX_DECREF(candidate);
        }

        /* Don't throw away the list if one cert was bad! */
        pkixTempErrorReceived = PKIX_FALSE;

        PKIX_CHECK(PKIX_List_SetImmutable(filtered, plContext),
                "PKIX_List_SetImmutable failed");

        *pCertList = filtered;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filtered);
        }
        PKIX_DECREF(candidate);
        PKIX_DECREF(selected);
        PKIX_DECREF(params);

        PKIX_RETURN(CERTSTORE);
}

/*
 * FUNCTION: pkix_pl_Pk11CertStore_GetCRL
 *  (see description of PKIX_CertStore_CRLCallback in pkix_certstore.h)
 */
PKIX_Error *
pkix_pl_Pk11CertStore_GetCRL(
        PKIX_CertStore *store,
        PKIX_CRLSelector *selector,
        PKIX_List **pCrlList,
        void *plContext)
{
        PKIX_UInt32 i = 0;
        PKIX_UInt32 numFound = 0;
        PKIX_PL_CRL *candidate = NULL;
        PKIX_List *selected = NULL;
        PKIX_List *filtered = NULL;
        PKIX_CRLSelector_MatchCallback callback = NULL;
        PKIX_ComCRLSelParams *params = NULL;

        PKIX_ENTER(CERTSTORE, "pkix_pl_Pk11CertStore_GetCRL");
        PKIX_NULLCHECK_THREE(store, selector, pCrlList);

        PKIX_CHECK(PKIX_CRLSelector_GetMatchCallback
                (selector, &callback, plContext),
                "PKIX_CRLSelector_GetMatchCallback failed");

        PKIX_CHECK(PKIX_CRLSelector_GetCommonCRLSelectorParams
                (selector, &params, plContext),
                "PKIX_CRLSelector_GetComCertSelParams failed");

        PKIX_CHECK(pkix_pl_Pk11CertStore_CrlQuery
                (params, &selected, plContext),
                "pkix_pl_Pk11CertStore_CrlQuery failed");

        if (selected) {
                PKIX_CHECK(PKIX_List_GetLength(selected, &numFound, plContext),
                        "PKIX_List_GetLength failed");
        }

        PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                "PKIX_List_Create failed");

        for (i = 0; i < numFound; i++) {
                PKIX_CHECK_ONLY_FATAL(PKIX_List_GetItem
                        (selected,
                        i,
                        (PKIX_PL_Object **)&candidate,
                        plContext),
                        "PKIX_List_GetItem failed");

                if (PKIX_ERROR_RECEIVED) {
                        continue; /* just skip bad CRLs */
                }

                PKIX_CHECK_ONLY_FATAL(callback(selector, candidate, plContext),
                        "crlSelector failed");

                if (!(PKIX_ERROR_RECEIVED)) {
                        PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                (filtered,
                                (PKIX_PL_Object *)candidate,
                                plContext),
                                "PKIX_List_AppendItem failed");
                }

                PKIX_DECREF(candidate);
        }

        /* Don't throw away the list if one CRL was bad! */
        pkixTempErrorReceived = PKIX_FALSE;

        PKIX_CHECK(PKIX_List_SetImmutable(filtered, plContext),
                "PKIX_List_SetImmutable failed");

        *pCrlList = filtered;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(filtered);
        }
        PKIX_DECREF(candidate);
        PKIX_DECREF(selected);
        PKIX_DECREF(params);

        PKIX_RETURN(CERTSTORE);
}

/* --Public-Pk11CertStore-Functions----------------------------------- */

/*
 * FUNCTION: PKIX_PL_Pk11CertStore_Create
 * (see comments in pkix_samples_modules.h)
 */
PKIX_Error *
PKIX_PL_Pk11CertStore_Create(
        PKIX_CertStore **pCertStore,
        void *plContext)
{
        PKIX_CertStore *certStore = NULL;

        PKIX_ENTER(CERTSTORE, "PKIX_PL_Pk11CertStore_Create");
        PKIX_NULLCHECK_ONE(pCertStore);

        PKIX_CHECK(PKIX_CertStore_Create
                (pkix_pl_Pk11CertStore_GetCert,
                pkix_pl_Pk11CertStore_GetCRL,
                NULL,
                PKIX_TRUE, /* cache flag */
                pkix_pl_Pk11CertStore_CheckTrust,
                &certStore,
                plContext),
                "PKIX_CertStore_Create failed");

        *pCertStore = certStore;

cleanup:

        PKIX_RETURN(CERTSTORE);
}
