/*
 * NSS utility functions
 *
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
 *
 # $Id$
 */

#include <ctype.h>
#include "seccomon.h"
#include "prinit.h"
#include "prprf.h"
#include "prmem.h"
#include "pcertt.h"
#include "lowkeyi.h"
#include "pcert.h"
#include "secrng.h"
#include "cdbhdl.h"
#include "pkcs11i.h"

static char *
pk11_certdb_name_cb(void *arg, int dbVersion)
{
    const char *configdir = (const char *)arg;
    const char *dbver;

    switch (dbVersion) {
      case 7:
	dbver = "7";
	break;
      case 6:
	dbver = "6";
	break;
      case 5:
	dbver = "5";
	break;
      case 4:
      default:
	dbver = "";
	break;
    }

    return PR_smprintf(CERT_DB_FMT, configdir, dbver);
}
    
static char *
pk11_keydb_name_cb(void *arg, int dbVersion)
{
    const char *configdir = (const char *)arg;
    const char *dbver;
    
    switch (dbVersion) {
      case 4:
	dbver = "4";
	break;
      case 3:
	dbver = "3";
	break;
      case 1:
	dbver = "1";
	break;
      case 2:
      default:
	dbver = "";
	break;
    }

    return PR_smprintf(KEY_DB_FMT, configdir, dbver);
}

/* for now... we need to define vendor specific codes here.
 */
#define CKR_CERTDB_FAILED	CKR_DEVICE_ERROR
#define CKR_KEYDB_FAILED	CKR_DEVICE_ERROR

const char *
pk11_EvaluateConfigDir(const char *configdir,char **appName)
{
    if (PORT_Strncmp(configdir, MULTIACCESS, sizeof(MULTIACCESS)-1) == 0) {
	char *cdir;

	*appName = PORT_Strdup(configdir+sizeof(MULTIACCESS)-1);
	if (*appName == NULL) {
	    return configdir;
	}
	cdir = *appName;
	while (*cdir && *cdir != ':') {
	    cdir++;
	}
	if (*cdir == ':') {
	   *cdir = 0;
	   cdir++;
	}
	configdir = cdir;
    }
    return configdir;
}

static CK_RV
pk11_OpenCertDB(const char * configdir, const char *prefix, PRBool readOnly,
    					    NSSLOWCERTCertDBHandle **certdbPtr)
{
    NSSLOWCERTCertDBHandle *certdb = NULL;
    CK_RV        crv = CKR_CERTDB_FAILED;
    SECStatus    rv;
    char * name = NULL;
    char * appName = NULL;

    if (prefix == NULL) {
	prefix = "";
    }

    configdir = pk11_EvaluateConfigDir(configdir, &appName);

    name = PR_smprintf("%s" PATH_SEPARATOR "%s",configdir,prefix);
    if (name == NULL) goto loser;

    certdb = (NSSLOWCERTCertDBHandle*)PORT_ZAlloc(sizeof(NSSLOWCERTCertDBHandle));
    if (certdb == NULL) 
    	goto loser;

/* fix when we get the DB in */
    rv = nsslowcert_OpenCertDB(certdb, readOnly, appName, prefix,
				pk11_certdb_name_cb, (void *)name, PR_FALSE);
    if (rv == SECSuccess) {
	crv = CKR_OK;
	*certdbPtr = certdb;
	certdb = NULL;
    }
loser: 
    if (certdb) PR_Free(certdb);
    if (name) PORT_Free(name);
    if (appName) PORT_Free(appName);
    return crv;
}

static CK_RV
pk11_OpenKeyDB(const char * configdir, const char *prefix, PRBool readOnly,
    						NSSLOWKEYDBHandle **keydbPtr)
{
    NSSLOWKEYDBHandle *keydb;
    char * name = NULL;
    char * appName = NULL;

    if (prefix == NULL) {
	prefix = "";
    }
    configdir = pk11_EvaluateConfigDir(configdir, &appName);

    name = PR_smprintf("%s" PATH_SEPARATOR "%s",configdir,prefix);	
    if (name == NULL) 
	return SECFailure;
    keydb = nsslowkey_OpenKeyDB(readOnly, appName, prefix, 
					pk11_keydb_name_cb, (void *)name);
    PORT_Free(name);
    if (appName) PORT_Free(appName);
    if (keydb == NULL)
	return CKR_KEYDB_FAILED;
    *keydbPtr = keydb;

    return CKR_OK;
}


/*
 * OK there are now lots of options here, lets go through them all:
 *
 * configdir - base directory where all the cert, key, and module datbases live.
 * certPrefix - prefix added to the beginning of the cert database example: "
 * 			"https-server1-"
 * keyPrefix - prefix added to the beginning of the key database example: "
 * 			"https-server1-"
 * secmodName - name of the security module database (usually "secmod.db").
 * readOnly - Boolean: true if the databases are to be openned read only.
 * nocertdb - Don't open the cert DB and key DB's, just initialize the 
 *			Volatile certdb.
 * nomoddb - Don't open the security module DB, just initialize the 
 *			PKCS #11 module.
 * forceOpen - Continue to force initializations even if the databases cannot
 * 			be opened.
 */
CK_RV
pk11_DBInit(const char *configdir, const char *certPrefix, 
	    const char *keyPrefix, PRBool readOnly, 
	    PRBool noCertDB, PRBool noKeyDB, PRBool forceOpen,
	    NSSLOWCERTCertDBHandle **certdbPtr, NSSLOWKEYDBHandle **keydbPtr)
{
    CK_RV crv = CKR_OK;


    if (!noCertDB) {
	crv = pk11_OpenCertDB(configdir, certPrefix, readOnly, certdbPtr);
	if (crv != CKR_OK) {
	    if (!forceOpen) goto loser;
	    crv = CKR_OK;
	}
    }
    if (!noKeyDB) {

	crv = pk11_OpenKeyDB(configdir, keyPrefix, readOnly, keydbPtr);
	if (crv != CKR_OK) {
	    if (!forceOpen) goto loser;
	    crv = CKR_OK;
	}
    }


loser:
    return crv;
}


void
pk11_DBShutdown(NSSLOWCERTCertDBHandle *certHandle, 
		NSSLOWKEYDBHandle *keyHandle)
{
    if (certHandle) {
    	nsslowcert_ClosePermCertDB(certHandle);
	PORT_Free(certHandle);
	certHandle= NULL;
    }

    if (keyHandle) {
    	nsslowkey_CloseKeyDB(keyHandle);
	keyHandle= NULL;
    }
}

static rdbfunc pk11_rdbfunc;
static void *pk11_tnx;

/* NOTE: SHLIB_SUFFIX is defined on the command line */
#define RDBLIB "rdb."SHLIB_SUFFIX

DB * rdbopen(const char *appName, const char *prefix, 
				const char *type, int flags)
{
    PRLibrary *lib;
    DB *db;

    if (pk11_rdbfunc) {
	db = (*pk11_rdbfunc)(appName,prefix,type,flags);
	return db;
    }

    /*
     * try to open the library.
     */
    lib = PR_LoadLibrary(RDBLIB);

    if (!lib) {
	return NULL;
    }

    /* get the entry point */
    pk11_rdbfunc = (rdbfunc) PR_FindSymbol(lib,"rdbopen");
    if (pk11_rdbfunc) {
	db = (*pk11_rdbfunc)(appName,prefix,type,flags);
	return db;
    }

    /* couldn't find the entry point, unload the library and fail */
    PR_UnloadLibrary(lib);
    return NULL;
}

struct RDBStr {
    DB	db;
    int (*xactstart)(DB *db);
    int (*xactdone)(DB *db, PRBool abort);
};

#define DB_RDB ((DBTYPE) 0xff)

int
db_BeginTransaction(DB *db)
{
    RDB *rdb = (RDB *)db;
    if (db->type != DB_RDB) {
	return 0;
    }

    return rdb->xactstart(db);
}

int
db_FinishTransaction(DB *db, PRBool abort)
{
    RDB *rdb = (RDB *)db;
    if (db->type != DB_RDB) {
	return 0;
    }

    return rdb->xactdone(db, abort);
}



SECStatus
db_Copy(DB *dest,DB *src)
{
    int ret;
    DBT key,data;
    ret = (*src->seq)(src, &key, &data, R_FIRST);
    if (ret)  {
	return SECSuccess;
    }

    do {
	(void)(*dest->put)(dest,&key,&data, R_NOOVERWRITE);
    } while ( (*src->seq)(src, &key, &data, R_NEXT) == 0);
    (void)(*dest->sync)(dest,0);

    return SECSuccess;
}
