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
 * pkix_basicconstraintschecker.c
 *
 * Functions for basic constraints validation
 *
 */

#include "pkix_basicconstraintschecker.h"

/* --Private-BasicConstraintsCheckerState-Functions------------------------- */

/*
 * FUNCTION: pkix_BasicConstraintsCheckerState_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_BasicConstraintsCheckerState_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        pkix_BasicConstraintsCheckerState *state = NULL;

        PKIX_ENTER(BASICCONSTRAINTSCHECKERSTATE,
                    "pkix_BasicConstraintsCheckerState_Destroy");

        PKIX_NULLCHECK_ONE(object);

        /* Check that this object is a basic constraints checker state */
        PKIX_CHECK(pkix_CheckType
                (object, PKIX_BASICCONSTRAINTSCHECKERSTATE_TYPE, plContext),
                "Object is not a basic constraints checker state");

        state = (pkix_BasicConstraintsCheckerState *)object;

        PKIX_DECREF(state->basicConstraintsOID);

cleanup:

        PKIX_RETURN(BASICCONSTRAINTSCHECKERSTATE);
}

/*
 * FUNCTION: pkix_BasicConstraintsCheckerState_RegisterSelf
 * DESCRIPTION:
 *  Registers PKIX_CERT_TYPE and its related functions with systemClasses[]
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_BasicConstraintsCheckerState_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(BASICCONSTRAINTSCHECKERSTATE,
                "pkix_BasicConstraintsCheckerState_RegisterSelf");

        entry.description = "BasicConstraintsCheckerState";
        entry.destructor = pkix_BasicConstraintsCheckerState_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_BASICCONSTRAINTSCHECKERSTATE_TYPE] = entry;

cleanup:

        PKIX_RETURN(BASICCONSTRAINTSCHECKERSTATE);
}

/*
 * FUNCTION: pkix_BasicConstraintsCheckerState_Create
 * DESCRIPTION:
 *
 *  Creates a new BasicConstraintsCheckerState using the number of certs in
 *  the chain represented by "certsRemaining" and stores it at "pState".
 *
 * PARAMETERS:
 *  "certsRemaining"
 *      Number of certificates in the chain.
 *  "pState"
 *      Address where object pointer will be stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a BasicConstraintsCheckerState Error if the function fails in a
 *      non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_BasicConstraintsCheckerState_Create(
        PKIX_UInt32 certsRemaining,
        pkix_BasicConstraintsCheckerState **pState,
        void *plContext)
{
        pkix_BasicConstraintsCheckerState *state = NULL;

        PKIX_ENTER(BASICCONSTRAINTSCHECKERSTATE,
                    "pkix_BasicConstraintsCheckerState_Create");

        PKIX_NULLCHECK_ONE(pState);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_BASICCONSTRAINTSCHECKERSTATE_TYPE,
                    sizeof (pkix_BasicConstraintsCheckerState),
                    (PKIX_PL_Object **)&state,
                    plContext),
                    "Could not create basic constraints state object");

        /* initialize fields */
        state->certsRemaining = certsRemaining;
        state->maxPathLength = PKIX_UNLIMITED_PATH_CONSTRAINT;

        PKIX_CHECK(PKIX_PL_OID_Create
                    (PKIX_BASICCONSTRAINTS_OID,
                    &state->basicConstraintsOID,
                    plContext),
                    "PKIX_PL_OID_Create failed");

        *pState = state;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(state);
        }

        PKIX_RETURN(BASICCONSTRAINTSCHECKERSTATE);
}

/* --Private-BasicConstraintsChecker-Functions------------------------------ */

/*
 * FUNCTION: pkix_BasicConstraintsChecker_Check
 * (see comments for PKIX_CertChainChecker_CheckCallback in pkix_checker.h)
 */
PKIX_Error *
pkix_BasicConstraintsChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,  /* list of PKIX_PL_OID */
        void *plContext)
{
        PKIX_PL_CertBasicConstraints *basicConstraints = NULL;
        pkix_BasicConstraintsCheckerState *state = NULL;
        PKIX_Boolean caFlag = PKIX_FALSE;
        PKIX_Int32 pathLength = 0;
        PKIX_Int32 maxPathLength_now;
        PKIX_PL_OID *theOID = NULL;
        PKIX_Boolean isSelfIssued = PKIX_FALSE;
        PKIX_Boolean isLastCert = PKIX_FALSE;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_BasicConstraintsChecker_Check");
        PKIX_NULLCHECK_TWO(checker, cert);

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&state, plContext),
                    "PKIX_CertChainChecker_GetCertChainCheckerState failed");

        state->certsRemaining--;

        if (state->certsRemaining != 0) {

                PKIX_CHECK(PKIX_PL_Cert_GetBasicConstraints
                    (cert, &basicConstraints, plContext),
                    "PKIX_PL_Cert_GetBasicConstraints failed");

                /* get CA Flag and path length */
                if (basicConstraints != NULL) {
                        PKIX_CHECK(PKIX_PL_BasicConstraints_GetCAFlag
                            (basicConstraints,
                            &caFlag,
                            plContext),
                            "PKIX_PL_BasicConstraints_GetCAFlag failed");

                if (caFlag == PKIX_TRUE) {
                        PKIX_CHECK(PKIX_PL_BasicConstraints_GetPathLenConstraint
                            (basicConstraints,
                            &pathLength,
                            plContext),
                            "PKIX_PL_BasicConstraints_GetPathLenConstraint "
                            "failed");
                }

                }else{
                        caFlag = PKIX_FALSE;
                        pathLength = PKIX_UNLIMITED_PATH_CONSTRAINT;
                }

                PKIX_CHECK(pkix_IsCertSelfIssued
                        (cert,
                        &isSelfIssued,
                        plContext),
                        "pkix_IsCertSelfIssued failed");

                maxPathLength_now = state->maxPathLength;

                if (isSelfIssued != PKIX_TRUE) {

                    /* Not last CA Cert, but maxPathLength is down to zero */
                    if (maxPathLength_now == 0) {
                        PKIX_ERROR("PKIX_BasicConstraints validation failed: "
                                "maximum length mismatch");
                    }

                    if (caFlag == PKIX_FALSE) {
                        PKIX_ERROR("PKIX_BaseConstraints validation failed: "
                                "CA Flag not set");
                    }

                    if (maxPathLength_now > 0) { /* can be unlimited (-1) */
                        maxPathLength_now--;
                    }

                }

                if (caFlag == PKIX_TRUE) {
                    if (maxPathLength_now == PKIX_UNLIMITED_PATH_CONSTRAINT){
                            maxPathLength_now = pathLength;
                    } else {
                            /* If pathLength is not specified, don't set */
                        if (pathLength != PKIX_UNLIMITED_PATH_CONSTRAINT) {
                            maxPathLength_now =
                                    (maxPathLength_now > pathLength)?
                                    pathLength:maxPathLength_now;
                        }
                    }
                }

                state->maxPathLength = maxPathLength_now;
        }

        /* Remove Basic Constraints Extension OID from list */
        if (unresolvedCriticalExtensions != NULL) {

                PKIX_CHECK(pkix_List_Remove
                            (unresolvedCriticalExtensions,
                            (PKIX_PL_Object *) state->basicConstraintsOID,
                            plContext),
                            "PKIX_List_Remove failed");
        }


        PKIX_CHECK(PKIX_CertChainChecker_SetCertChainCheckerState
                    (checker, (PKIX_PL_Object *)state, plContext),
                    "PKIX_CertChainChecker_SetCertChainCheckerState failed");


cleanup:
        PKIX_DECREF(state);
        PKIX_DECREF(basicConstraints);
        PKIX_RETURN(CERTCHAINCHECKER);

}

/*
 * FUNCTION: pkix_BasicConstraintsChecker_Initialize
 * DESCRIPTION:
 *  Registers PKIX_CERT_TYPE and its related functions with systemClasses[]
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_BasicConstraintsChecker_Initialize(
        PKIX_UInt32 certsRemaining,
        PKIX_CertChainChecker **pChecker,
        void *plContext)
{
        pkix_BasicConstraintsCheckerState *state = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_BasicConstraintsChecker_Initialize");
        PKIX_NULLCHECK_ONE(pChecker);

        PKIX_CHECK(pkix_BasicConstraintsCheckerState_Create
                    (certsRemaining, &state, plContext),
                    "PKIX_BasicConstraintsCheckerState_Create failed");

        PKIX_CHECK(PKIX_CertChainChecker_Create
                    (pkix_BasicConstraintsChecker_Check,
                    PKIX_FALSE,
                    PKIX_FALSE,
                    NULL,
                    (PKIX_PL_Object *)state,
                    pChecker,
                    plContext),
                    "PKIX_CertChainChecker_Check failed");

cleanup:
        PKIX_DECREF(state);

        PKIX_RETURN(CERTCHAINCHECKER);
}
