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
 * pkix_logger.c
 *
 * Logger Object Functions
 *
 */

#include "pkix_logger.h"

/* Global variable to keep PKIX_Logger List */
PKIX_List *pkixLoggers = NULL;

/*
 * Once the Logger has been set, for any logging related operations, we have
 * to go through the List to find a match, and if found, issue the
 * corresponding callback. The overhead to check for DEBUG and TRACE in each
 * PKIX function entering and exiting is very expensive (400X), and redundant
 * if they are not the interest of the Logger. Therefore, the PKIX_Logger List
 * pkixLoggers is separated into two lists based on its Loggers' trace level.
 *
 * Whenever the pkixLoggers List is updated by PKIX_Logger_AddLogger() or
 * PKIX_Logger_SetLoggers(), we destroy and reconstruct pkixLoggersErrors
 * and pkixLoggersDebugTrace Logger Lists. The ERROR, FATAL_ERROR and
 * WARNING goes to pkixLoggersErrors and the DEBUG and TRACE goes to
 * pkixLoggersDebugTrace.
 *
 * Currently we provide five logging levels and the default setting are by:
 *
 *     PKIX_FATAL_ERROR() macro invokes pkix_Logger_Check of FATAL_ERROR level
 *     PKIX_ERROR() macro invokes pkix_Logger_Check of ERROR level
 *     WARNING is not invoked as default
 *     PKIX_DEBUG() macro invokes pkix_Logger_Check of DEBUG level. This needs
 *         compilation -DPKIX_<component>DEBUG flag to turn on 
 *     PKIX_ENTER() and PKIX_RETURN() macros invoke pkix_Logger_Check of TRACE
 *         level. TRACE provides duplicate information of DEBUG, but needs no
 *         recompilation and cannot choose component. To allow application
 *         to use DEBUG level, TRACE is put as last.
 *
 */
PKIX_List *pkixLoggersErrors = NULL;
PKIX_List *pkixLoggersDebugTrace = NULL;

/* To ensure atomic update on pkixLoggers lists */
PKIX_PL_MonitorLock *pkixLoggerLock = NULL;

const char *
PKIX_COMPONENTNAMES[PKIX_NUMERRORS] =
{
        "Object",
        "Fatal",
        "Memory",
        "Error",
        "Mutex",
        "RWLock",
        "String",
        "OID",
        "List",
        "ByteArray",
        "BigInt",
        "HashTable",
        "Cert",
        "X500Name",
        "GeneralName",
        "PublicKey",
        "Date",
        "TrustAnchor",
        "ProcessingParams",
        "CertChain",
        "ValidateParams",
        "Validate",
        "ValidateResult",
        "CertChainChecker",
        "CertSelector",
        "ComCertSelParams",
        "TargetCertCheckerState",
        "CertBasicConstraints",
        "CertPolicyQualifier",
        "CertPolicyInfo",
        "CertPolicyNode",
        "CertPolicyCheckerState",
        "Lifecycle",
        "BasicConstraintsCheckerState",
        "ComCRLSelParams",
        "CertStore",
        "CollectionCertStoreContext",
        "DefaultCRLCheckerState",
        "CRL",
        "CRLEntry",
        "CRLSelector",
        "CertPolicyMap",
        "Build",
        "BuildResult",
        "BuildParams",
        "ForwardBuilderState",
        "SignatureCheckerState",
        "CertNameConstraints",
        "CertNameConstraintsCheckerState",
        "RevocationChecker",
        "User Defined Modules",
        "Context",
        "DefaultRevocationChecker",
        "LdapRequest",
        "LdapResponse",
        "LdapClient",
        "LdapDefaultClient",
        "Socket",
        "ResourceLimits",
        "Logger",
        "MonitorLock",
        "InfoAccess",
        "AIAMgr",
        "OcspChecker"
};

/* --Private-Functions-------------------------------------------- */

/*
 * FUNCTION: pkix_Logger_CheckErrors
 * DESCRIPTION:
 *
 *  This function goes through each PKIX_Logger at "pkixLoggersList" and
 *  checks if "maxLevel" and "logComponent" satisfies what is specified in the
 *  PKIX_Logger. If satisfies, it invokes the callback in PKIX_Logger and
 *  passes a PKIX_PL_String that is the concatenation of "message" and 
 *  "message2" to the application for processing. 
 *  Since this call is inserted into a handful PKIX macros, to avoid recursive
 *  call to itself in those macros, no macros is applied in this function.
 *  If an error occurrs, this call is aborted.
 *
 * PARAMETERS:
 *  "pkixLoggersList"
 *      A list of PKIX_Logger to be examined for invoking callback. Must be
 *      non-NULL.
 *  "message"
 *      Address of "message" to be logged. Must be non-NULL.
 *  "message2"
 *      Address of "message2" to be concatenated and logged. May be NULL.
 *  "logComponent"
 *      An PKIX_UInt32 represents the component the message is from.
 *  "maxLevel"
 *      An PKIX_UInt32 represents the level of severity of the message.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds
 *  Returns a Fatal Error if the function fails in an unrecoverable way
 */
PKIX_Error *
pkix_Logger_Check(
        PKIX_List *pkixLoggersList,
        char *message,
        char *message2,
        PKIX_UInt32 logComponent,
        PKIX_UInt32 currentLevel,
        void *plContext)
{
        PKIX_Logger *logger = NULL;
        PKIX_List *savedPkixLoggersErrors = NULL;
        PKIX_List *savedPkixLoggersDebugTrace = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *messageString = NULL;
        PKIX_PL_String *message2String = NULL;
        PKIX_PL_String *msgString = NULL;
        PKIX_PL_String *logComponentString = NULL;
        PKIX_Error *error = NULL;
        PKIX_Boolean needLogging = PKIX_FALSE;
        PKIX_UInt32 i, length;

        /*
         * We cannot use any the PKIX_ macros here, since this function
         * is called in some of these macros. It can create endless recursion.
         */

        if (pkixLoggersList == NULL || message == NULL) {
                return(NULL);;
        }

        /*
         * Disable all subsequent loggings to avoid recursion. The result is
         * if other thread is calling this function at the same time, there
         * won't be any logging because the pkixLoggersErrors and
         * pkixLoggersDebugTrace are set to null.
         * It would be nice if we provide control per thread (e.g. make
         * plContext threadable) then we can avoid the recursion by setting
         * flag at plContext. Then other thread's logging won't be affected.
         *
         * Also we need to use a reentrant Lock. Although we avoid recursion
         * for TRACE. When there is an ERROR occurrs in subsequent call, this
         * function will be called.
         */

         error = PKIX_PL_MonitorLock_Enter(pkixLoggerLock, plContext);
        if (error) { return(NULL); }

        savedPkixLoggersDebugTrace = pkixLoggersDebugTrace;
        pkixLoggersDebugTrace = NULL;
        savedPkixLoggersErrors = pkixLoggersErrors;
        pkixLoggersErrors = NULL;

        /* Convert message and message2 to String */
        error = PKIX_PL_String_Create
                    (PKIX_ESCASCII, message, 0, &messageString, plContext);
        if (error) { goto cleanup; }

        if (message2) {
                error = PKIX_PL_String_Create
                    (PKIX_ESCASCII, message2, 0, &message2String, plContext);
                if (error) { goto cleanup; }
                error = PKIX_PL_String_Create
                    (PKIX_ESCASCII, "%s %s", 0, &formatString, plContext);
                if (error) { goto cleanup; }


        } else {
                error = PKIX_PL_String_Create
                    (PKIX_ESCASCII, "%s", 0, &formatString, plContext);
                if (error) { goto cleanup; }


        }

        error = PKIX_PL_Sprintf
                    (&msgString,
                    plContext,
                    formatString,
                    messageString,
                    message2String);
        if (error) { goto cleanup; }

        /*
         * Convert component to String : see pkixt.h for reason that we have to
         * use PKXI_NUMERRORS.
         */
        if (logComponent >= PKIX_NUMERRORS) {
                goto cleanup;
        }

        error = PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    (void *)PKIX_COMPONENTNAMES[logComponent],
                    0,
                    &logComponentString,
                    plContext);
        if (error) { goto cleanup; }

        /* Go through the Logger list */

        error = PKIX_List_GetLength(pkixLoggersList, &length, plContext);
        if (error) { goto cleanup; }

        for (i = 0; i < length; i++) {

                error = PKIX_List_GetItem
                    (pkixLoggersList,
                    i,
                    (PKIX_PL_Object **) &logger,
                    plContext);
                if (error) { goto cleanup; }

                /* Intended logging level less or equal than the max */
                needLogging = currentLevel <= logger->maxLevel;

                if (needLogging && logger->callback) {

                    /*
                     * We separate Logger into two lists based on log level
                     * but log level is not modified. We need to check here to
                     * avoid logging the higher log level (lower value) twice.
                     */
                    if (pkixLoggersList == pkixLoggersErrors) {
                            needLogging = needLogging && 
                                (currentLevel <= PKIX_LOGGER_LEVEL_WARNING);
                    } else if (pkixLoggersList == pkixLoggersDebugTrace) {
                            needLogging = needLogging &&
                                (currentLevel > PKIX_LOGGER_LEVEL_WARNING);
                    }
                
                    if (needLogging && logger->logComponent) {
                        error = PKIX_PL_Object_Equals(
                            (PKIX_PL_Object *) logComponentString,
                            (PKIX_PL_Object *) logger->logComponent,
                            &needLogging,
                            plContext);
                        if (error) { goto cleanup; }
                    }

                    if (needLogging) {
                        error = logger->callback
                                (logger,
                                msgString,
                                currentLevel,
                                logComponentString,
                                plContext);
                        if (error) { goto cleanup; }
                    }

                }

                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)logger, plContext);
                logger = NULL;
                if (error) { goto cleanup; }

        }

cleanup:

        if (formatString) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)formatString, plContext);
        }

        if (messageString) {
                error = PKIX_PL_Object_DecRef
                         ((PKIX_PL_Object *)messageString, plContext);
        }

        if (message2String) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)message2String, plContext);
        }

        if (msgString) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)msgString, plContext);
        }

        if (logComponentString) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)logComponentString, plContext);
        }

        if (logger) {
                error = PKIX_PL_Object_DecRef
                        ((PKIX_PL_Object *)logger, plContext);
        }

        if (pkixLoggersErrors == NULL && savedPkixLoggersErrors != NULL) {
                pkixLoggersErrors = savedPkixLoggersErrors;
        } 

        if (pkixLoggersDebugTrace == NULL && 
           savedPkixLoggersDebugTrace != NULL) {
                pkixLoggersDebugTrace = savedPkixLoggersDebugTrace;
        }

        error = PKIX_PL_MonitorLock_Exit(pkixLoggerLock, plContext);
        if (error) { return(NULL); }

        return(NULL);
}

/*
 * FUNCTION: pkix_Logger_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Logger_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_Logger *logger = NULL;

        PKIX_ENTER(LOGGER, "pkix_Logger_Destroy");
        PKIX_NULLCHECK_ONE(object);

        /* Check that this object is a logger */
        PKIX_CHECK(pkix_CheckType(object, PKIX_LOGGER_TYPE, plContext),
                    "Object is not a Logger");

        logger = (PKIX_Logger *)object;

        /* We have a valid logger. DecRef its item and recurse on next */

        logger->callback = NULL;
        PKIX_DECREF(logger->context);
        PKIX_DECREF(logger->logComponent);

cleanup:

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: pkix_Logger_ToString
 * (see comments for PKIX_PL_ToStringCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Logger_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_Logger *logger = NULL;
        char *asciiFormat = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *contextString = NULL;
        PKIX_PL_String *loggerString = NULL;

        PKIX_ENTER(LOGGER, "pkix_Logger_ToString_Helper");
        PKIX_NULLCHECK_TWO(object, pString);

        /* Check that this object is a logger */
        PKIX_CHECK(pkix_CheckType(object, PKIX_LOGGER_TYPE, plContext),
                    "Object is not a Logger");

        logger = (PKIX_Logger *)object;

        asciiFormat =
                "[\n"
                "\tLogger: \n"
                "\tContext:          %s\n"
                "\tMaximum Level:    %d\n"
                "\tComponment Name:  %s\n"
                "]\n";

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    asciiFormat,
                    0,
                    &formatString,
                    plContext),
                    "PKIX_PL_String_Create failed");

        PKIX_TOSTRING(logger->context, &contextString, plContext,
                "PKIX_PL_Object_ToString failed");

        PKIX_CHECK(PKIX_PL_Sprintf
                (&loggerString,
                plContext,
                formatString,
                contextString,
                logger->maxLevel,
                logger->logComponent),
                "PKIX_PL_Sprintf failed");

        *pString = loggerString;


cleanup:

        PKIX_DECREF(formatString);
        PKIX_DECREF(contextString);
        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: pkix_Logger_Equals
 * (see comments for PKIX_PL_EqualsCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Logger_Equals(
        PKIX_PL_Object *first,
        PKIX_PL_Object *second,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 secondType;
        PKIX_Boolean cmpResult;
        PKIX_Logger *firstLogger = NULL;
        PKIX_Logger *secondLogger = NULL;
        PKIX_UInt32 i = 0;

        PKIX_ENTER(LOGGER, "pkix_Logger_Equals");
        PKIX_NULLCHECK_THREE(first, second, pResult);

        /* test that first is a Logger */
        PKIX_CHECK(pkix_CheckType(first, PKIX_LOGGER_TYPE, plContext),
                    "First Argument is not a Logger");

        /*
         * Since we know first is a Logger, if both references are
         * identical, they must be equal
         */
        if (first == second){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        /*
         * If second isn't a Logger, we don't throw an error.
         * We simply return a Boolean result of FALSE
         */
        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType(second, &secondType, plContext),
                    "Could not get type of second argument");
        if (secondType != PKIX_LOGGER_TYPE) goto cleanup;

        firstLogger = (PKIX_Logger *)first;
        secondLogger = (PKIX_Logger *)second;

        cmpResult = PKIX_FALSE;

        if (firstLogger->callback != secondLogger->callback) {
                goto cleanup;
        }

        PKIX_EQUALS  
                (firstLogger->context,
                secondLogger->context,
                &cmpResult,
                plContext,
                "PKIX_PL_Object_Equals failed");

        if (cmpResult == PKIX_FALSE) {
                goto cleanup;
        }

        if (firstLogger->maxLevel != secondLogger->maxLevel) {
                goto cleanup;
        }

        PKIX_EQUALS
                ((PKIX_PL_Object *)firstLogger->logComponent,
                (PKIX_PL_Object *)secondLogger->logComponent,
                &cmpResult,
                plContext,
                "PKIX_PL_Object_Equals failed");

        *pResult = cmpResult;

cleanup:

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: pkix_Logger_Hashcode
 * (see comments for PKIX_PL_HashcodeCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Logger_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_Logger *logger = NULL;
        PKIX_UInt32 hash = 0;
        PKIX_UInt32 tempHash = 0;

        PKIX_ENTER(LOGGER, "pkix_Logger_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_LOGGER_TYPE, plContext),
                    "Object is not a logger");

        logger = (PKIX_Logger *)object;

        PKIX_HASHCODE(logger->context, &tempHash, plContext,
                "PKIX_PL_Object_Hashcode failed");

        hash = (PKIX_UInt32) logger->callback + tempHash << 7 +
                logger->maxLevel;

        PKIX_HASHCODE(logger->logComponent, &tempHash, plContext,
                "PKIX_PL_Object_Hashcode failed");

        hash = hash << 7 + tempHash;

        *pHashcode = hash;

cleanup:

        PKIX_RETURN(LOGGER);
}


/*
 * FUNCTION: pkix_Logger_Duplicate
 * (see comments for PKIX_PL_DuplicateCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_Logger_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_Logger *logger = NULL;
        PKIX_Logger *dupLogger = NULL;

        PKIX_ENTER(LOGGER, "pkix_Logger_Duplicate");
        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_CHECK(pkix_CheckType
                    ((PKIX_PL_Object *)object, PKIX_LOGGER_TYPE, plContext),
                    "Object is not a Logger");

        logger = (PKIX_Logger *) object;

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LOGGER_TYPE,
                    sizeof (PKIX_Logger),
                    (PKIX_PL_Object **)&dupLogger,
                    plContext),
                    "Could not create Logger object");

        dupLogger->callback = logger->callback;
        dupLogger->maxLevel = logger->maxLevel;
        
        PKIX_DUPLICATE
                    (logger->context,
                    &dupLogger->context,
                    plContext,
                    "PKIX_PL_Object_Duplicate failed");

        PKIX_DUPLICATE
                    (logger->logComponent,
                    &dupLogger->logComponent,
                    plContext,
                    "PKIX_PL_Object_Duplicate failed");

        *pNewObject = (PKIX_PL_Object *) dupLogger;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(dupLogger);
        }

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: pkix_Logger_RegisterSelf
 * DESCRIPTION:
 *  Registers PKIX_LOGGER_TYPE and its related functions with systemClasses[]
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_Logger_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(LOGGER, "pkix_Logger_RegisterSelf");

        entry.description = "Logger";
        entry.destructor = pkix_Logger_Destroy;
        entry.equalsFunction = pkix_Logger_Equals;
        entry.hashcodeFunction = pkix_Logger_Hashcode;
        entry.toStringFunction = pkix_Logger_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_Logger_Duplicate;

        systemClasses[PKIX_LOGGER_TYPE] = entry;

        PKIX_RETURN(LOGGER);
}

/* --Public-Logger-Functions--------------------------------------------- */

/*
 * FUNCTION: PKIX_Logger_Create (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Logger_Create(
        PKIX_Logger_LogCallback callback,
        PKIX_PL_Object *loggerContext,
        PKIX_Logger **pLogger,
        void *plContext)
{
        PKIX_Logger *logger = NULL;

        PKIX_ENTER(LOGGER, "PKIX_Logger_Create");
        PKIX_NULLCHECK_ONE(pLogger);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LOGGER_TYPE,
                    sizeof (PKIX_Logger),
                    (PKIX_PL_Object **)&logger,
                    plContext),
                    "Could not create Logger object");

        logger->callback = callback;
        logger->maxLevel = 0;
        logger->logComponent = NULL;

        PKIX_INCREF(loggerContext);
        logger->context = loggerContext;

        *pLogger = logger;

cleanup:

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: PKIX_Logger_GetLogCallback (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Logger_GetLogCallback(
        PKIX_Logger *logger,
        PKIX_Logger_LogCallback *pCallback,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_GetLogCallback");
        PKIX_NULLCHECK_TWO(logger, pCallback);

        *pCallback = logger->callback;

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: PKIX_Logger_GetLoggerContext (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Logger_GetLoggerContext(
        PKIX_Logger *logger,
        PKIX_PL_Object **pLoggerContext,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_GetLoggerContex");
        PKIX_NULLCHECK_TWO(logger, pLoggerContext);

        PKIX_INCREF(logger->context);
        *pLoggerContext = logger->context;

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: PKIX_Logger_GetMaxLoggingLevel (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Logger_GetMaxLoggingLevel(
        PKIX_Logger *logger,
        PKIX_UInt32 *pLevel,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_GetMaxLoggingLevel");
        PKIX_NULLCHECK_TWO(logger, pLevel);

        *pLevel = logger->maxLevel;

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: PKIX_Logger_SetMaxLoggingLevel (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Logger_SetMaxLoggingLevel(
        PKIX_Logger *logger,
        PKIX_UInt32 level,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_SetMaxLoggingLevel");
        PKIX_NULLCHECK_ONE(logger);

        if (level > PKIX_LOGGER_LEVEL_MAX) {
                PKIX_ERROR("Logging Level exceeds Maximum");
        } else {
                logger->maxLevel = level;
        }

cleanup:

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: PKIX_Logger_GetLoggingComponent (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Logger_GetLoggingComponent(
        PKIX_Logger *logger,
        PKIX_PL_String **pComponent,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_GetLoggingComponent");
        PKIX_NULLCHECK_TWO(logger, pComponent);

        PKIX_INCREF(logger->logComponent);
        *pComponent = logger->logComponent;

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: PKIX_Logger_SetLoggingComponent (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_Logger_SetLoggingComponent(
        PKIX_Logger *logger,
        PKIX_PL_String *component,
        void *plContext)
{
        PKIX_ENTER(LOGGER, "PKIX_Logger_SetLoggingComponent");
        PKIX_NULLCHECK_ONE(logger);

        PKIX_DECREF(logger->logComponent);
        PKIX_INCREF(component);
        logger->logComponent = component;

        PKIX_RETURN(LOGGER);
}


/*
 * Following PKIX_GetLoggers(), PKIX_SetLoggers() and PKIX_AddLogger() are
 * documented as not thread-safe. However they are thread-safe now. We need
 * the lock when accessing the logger lists.
 */

/*
 * FUNCTION: PKIX_Logger__GetLoggers (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_GetLoggers(
        PKIX_List **pLoggers,  /* list of PKIX_Logger */
        void *plContext)
{
        PKIX_List *list = NULL;
        PKIX_List *savedPkixLoggersDebugTrace = NULL;
        PKIX_Logger *logger = NULL;
        PKIX_Logger *dupLogger = NULL;
        PKIX_UInt32 i, length;
        PKIX_Boolean locked = PKIX_FALSE;

        PKIX_ENTER(LOGGER, "PKIX_Logger_GetLoggers");
        PKIX_NULLCHECK_ONE(pLoggers);

        PKIX_CHECK(PKIX_PL_MonitorLock_Enter(pkixLoggerLock, plContext),
                "PKIX_PL_MonitorLock_Enter failed");
        locked = PKIX_TRUE;

        if (pkixLoggersDebugTrace != NULL) {
                /*
                 * Temporarily disable DEBUG/TRACE Logging to avoid possible
                 * deadlock:
                 * When the Logger List is being accessed, e.g. by DECREF,
                 * pkix_Logger_Check may went through the list for logging
                 * operation. This may creates deadlock situation. We can
                 * convert our Object lock to use the reentrant Monitor lock
                 * to solve this problem when pkix_Logger_Check() is not using
                 * the same mechanism (instead, plContext).
                 */
                savedPkixLoggersDebugTrace = pkixLoggersDebugTrace;
                pkixLoggersDebugTrace = NULL;
        }

        if (pkixLoggers == NULL) {
                length = 0;
        } else {
                PKIX_CHECK(PKIX_List_GetLength
                    (pkixLoggers, &length, plContext),
                    "PKIX_List_GetLength failed");
        }

        /* Create a list and copy the pkixLoggers item to the list */
        PKIX_CHECK(PKIX_List_Create(&list, plContext),
                    "PKIX_List_Create failed");

        for (i = 0; i < length; i++) {

            PKIX_CHECK(PKIX_List_GetItem
                        (pkixLoggers,
                        i,
                        (PKIX_PL_Object **) &logger,
                        plContext),
                        "PKIX_List_GetItem failed");

            PKIX_CHECK(pkix_Logger_Duplicate
                        ((PKIX_PL_Object *)logger,
                        (PKIX_PL_Object **)&dupLogger,
                        plContext),
                        "pkix_Logger_Duplicate failed");

            PKIX_CHECK(PKIX_List_AppendItem
                        (list,
                        (PKIX_PL_Object *) dupLogger,
                        plContext),
                        "PKIX_List_AppendItem failed");

            PKIX_DECREF(logger);
            PKIX_DECREF(dupLogger);
        }

        /* Set the list to be immutable */
        PKIX_CHECK(PKIX_List_SetImmutable(list, plContext),
                        "PKIX_List_SetImmutable failed");

        *pLoggers = list;

cleanup:

        PKIX_DECREF(logger);

        /* Turn pkix_Logging_Check TRACE on until the list is completed */
        if (savedPkixLoggersDebugTrace != NULL) {
                pkixLoggersDebugTrace = savedPkixLoggersDebugTrace;
        }

        if (locked) {
                PKIX_CHECK(PKIX_PL_MonitorLock_Exit(pkixLoggerLock, plContext),
                        "PKIX_PL_MonitorLock_Exit failed");
        }

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: PKIX_Logger_SetLoggers (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_SetLoggers(
        PKIX_List *loggers,  /* list of PKIX_Logger */
        void *plContext)
{
        PKIX_List *list = NULL;
        PKIX_List *savedPkixLoggersErrors = NULL;
        PKIX_List *savedPkixLoggersDebugTrace = NULL;
        PKIX_Logger *logger = NULL;
        PKIX_Logger *dupLogger = NULL;
        PKIX_Boolean locked = PKIX_FALSE;
        PKIX_UInt32 i, length;

        PKIX_ENTER(LOGGER, "PKIX_SetLoggers");

        PKIX_CHECK(PKIX_PL_MonitorLock_Enter(pkixLoggerLock, plContext),
                "PKIX_PL_MonitorLock_Enter failed");
        locked = PKIX_TRUE;

        /*
         * We should have our own copy of PKIX_Logger's on the List:
         * 1) So when Application modifies its list, we won't be affected.
         * 2) To avoid deadlock. If Application is managing this Logger
         *    object while the Looger is retrieving it from pkixLoggers
         *    for logging operation, a deadlock may occurrs.
         */

        /* disable the current one */
        PKIX_DECREF(pkixLoggers);
        PKIX_DECREF(pkixLoggersErrors);
        PKIX_DECREF(pkixLoggersDebugTrace);

        if (loggers != NULL) {

                PKIX_CHECK(PKIX_List_Create(&list, plContext),
                    "PKIX_List_Create failed");

                PKIX_CHECK(PKIX_List_GetLength(loggers, &length, plContext),
                    "PKIX_List_GetLength failed");

                for (i = 0; i < length; i++) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (loggers,
                        i,
                        (PKIX_PL_Object **) &logger,
                        plContext),
                        "PKIX_List_GetItem failed");

                    PKIX_CHECK(pkix_Logger_Duplicate
                        ((PKIX_PL_Object *)logger,
                        (PKIX_PL_Object **)&dupLogger,
                        plContext),
                        "pkix_Logger_Duplicate failed");

                    PKIX_CHECK(PKIX_List_AppendItem
                        (list,
                        (PKIX_PL_Object *) dupLogger,
                        plContext),
                        "PKIX_List_AppendItem failed");

                    /* Separate into two lists */
                    if (logger->maxLevel <= PKIX_LOGGER_LEVEL_WARNING) {

                        /* Put in pkixLoggersErrors */

                        if (savedPkixLoggersErrors == NULL) {

                            PKIX_CHECK(PKIX_List_Create
                                    (&savedPkixLoggersErrors,
                                    plContext),
                                    "PKIX_List_Create failed");
                        }
        
                        PKIX_CHECK(PKIX_List_AppendItem
                                (savedPkixLoggersErrors,
                                (PKIX_PL_Object *) dupLogger,
                                plContext),
                                "PKIX_List_AppendItem failed");
                            
                    } else {

                        /* Put in pkixLoggersDebugTrace */

                        if (savedPkixLoggersDebugTrace == NULL) {

                            PKIX_CHECK(PKIX_List_Create
                                    (&savedPkixLoggersDebugTrace,
                                    plContext),
                                    "PKIX_List_Create failed");
                        }
        
                        PKIX_CHECK(PKIX_List_AppendItem
                                (savedPkixLoggersDebugTrace,
                                (PKIX_PL_Object *) dupLogger,
                                plContext),
                                "PKIX_List_AppendItem failed");
                    }
                    PKIX_DECREF(logger);
                    PKIX_DECREF(dupLogger);

                }

                pkixLoggers = list;
        }

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(list);
                PKIX_DECREF(savedPkixLoggersErrors);
                PKIX_DECREF(savedPkixLoggersDebugTrace);
                pkixLoggers = NULL;
        }

        PKIX_DECREF(logger);

        pkixLoggersErrors = savedPkixLoggersErrors;
        pkixLoggersDebugTrace = savedPkixLoggersDebugTrace;

        if (locked) {
                PKIX_CHECK(PKIX_PL_MonitorLock_Exit(pkixLoggerLock, plContext),
                        "PKIX_PL_MonitorLock_Exit failed");
        }

        PKIX_RETURN(LOGGER);
}

/*
 * FUNCTION: PKIX_Logger_AddLogger (see comments in pkix_util.h)
 */
PKIX_Error *
PKIX_AddLogger(
        PKIX_Logger *logger,
        void *plContext)
{
        PKIX_Logger *dupLogger = NULL;
        PKIX_Logger *addLogger = NULL;
        PKIX_List *savedPkixLoggersErrors = NULL;
        PKIX_List *savedPkixLoggersDebugTrace = NULL;
        PKIX_Boolean locked = PKIX_FALSE;
        PKIX_UInt32 i, length;

        PKIX_ENTER(LOGGER, "PKIX_Logger_AddLogger");
        PKIX_NULLCHECK_ONE(logger);

        PKIX_CHECK(PKIX_PL_MonitorLock_Enter(pkixLoggerLock, plContext),
                "PKIX_PL_MonitorLock_Enter failed");
        locked = PKIX_TRUE;

        PKIX_DECREF(pkixLoggersErrors);
        PKIX_DECREF(pkixLoggersDebugTrace);

        if (pkixLoggers == NULL) {

            PKIX_CHECK(PKIX_List_Create(&pkixLoggers, plContext),
                    "PKIX_List_Create failed");
        }

        PKIX_CHECK(pkix_Logger_Duplicate
                    ((PKIX_PL_Object *)logger,
                    (PKIX_PL_Object **)&dupLogger,
                    plContext),
                    "pkix_Logger_Duplicate failed");

        PKIX_CHECK(PKIX_List_AppendItem
                    (pkixLoggers,
                    (PKIX_PL_Object *) dupLogger,
                    plContext),
                    "PKIX_List_AppendItem failed");

        PKIX_CHECK(PKIX_List_GetLength(pkixLoggers, &length, plContext),
                    "PKIX_List_GetLength failed");

        /* Reconstruct pkixLoggersErrors and pkixLoggersDebugTrace */
        for (i = 0; i < length; i++) {

                PKIX_CHECK(PKIX_List_GetItem
                        (pkixLoggers,
                        i,
                        (PKIX_PL_Object **) &addLogger,
                        plContext),
                        "PKIX_List_GetItem failed");


                /* Put in pkixLoggersErrors */

                if (savedPkixLoggersErrors == NULL) {

                        PKIX_CHECK(PKIX_List_Create
                                    (&savedPkixLoggersErrors,
                                    plContext),
                                    "PKIX_List_Create failed");
                }
        
                PKIX_CHECK(PKIX_List_AppendItem
                        (savedPkixLoggersErrors,
                        (PKIX_PL_Object *) addLogger,
                        plContext),
                        "PKIX_List_AppendItem failed");
                            
                if (addLogger->maxLevel > PKIX_LOGGER_LEVEL_WARNING) {

                        /* Put in pkixLoggersDebugTrace */

                        if (savedPkixLoggersDebugTrace == NULL) {

                            PKIX_CHECK(PKIX_List_Create
                                    (&savedPkixLoggersDebugTrace,
                                    plContext),
                                    "PKIX_List_Create failed");
                        }

                        PKIX_CHECK(PKIX_List_AppendItem
                                (savedPkixLoggersDebugTrace,
                                (PKIX_PL_Object *) addLogger,
                                plContext),
                                "PKIX_List_AppendItem failed");
                }

                PKIX_DECREF(addLogger);

        }

cleanup:

        PKIX_DECREF(dupLogger);
        PKIX_DECREF(addLogger);

        pkixLoggersErrors = savedPkixLoggersErrors;
        pkixLoggersDebugTrace = savedPkixLoggersDebugTrace;

        if (locked) {
                PKIX_CHECK(PKIX_PL_MonitorLock_Exit(pkixLoggerLock, plContext),
                        "PKIX_PL_MonitorLock_Exit failed");
        }

       PKIX_RETURN(LOGGER);
}
