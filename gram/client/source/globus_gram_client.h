/******************************************************************************
gram_client.h

Description:
    This header file contains the exported client interface of 
    the Resource Allocation Management System.

CVS Information:

    $Source$
    $Date$
    $Revision$
    $Author$
******************************************************************************/

#ifndef GLOBUS_I_GRAM_CLIENT_INCLUDE
#define GLOBUS_I_GRAM_CLIENT_INCLUDE

/******************************************************************************
                             Include header files
******************************************************************************/

#include "globus_common.h"

#ifndef EXTERN_C_BEGIN
#ifdef __cplusplus
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif
#endif
 
EXTERN_C_BEGIN

/******************************************************************************
                               Define constants
******************************************************************************/

#define GLOBUS_GRAM_CLIENT_MAX_MSG_SIZE                    64000
#define GLOBUS_GRAM_CLIENT_PARAM_SIZE                      1024
#define GLOBUS_GRAM_CLIENT_STRING_SIZE                     256

EXTERN_C_END
/* 
 *  Job State Constants
 */
#include "globus_gram_states.h"

/*
 *  Job Result Constants
 */
#include "globus_gram_error.h"
EXTERN_C_BEGIN

/*
 *  Job Parameter Constants
 */
#define GLOBUS_GRAM_CLIENT_EXECUTABLE_PARAM               "executable"
#define GLOBUS_GRAM_CLIENT_ARGUMENTS_PARAM                "arguments"
#define GLOBUS_GRAM_CLIENT_ENVIRONMENT_PARAM              "environment"
#define GLOBUS_GRAM_CLIENT_DIR_PARAM                      "directory"
#define GLOBUS_GRAM_CLIENT_COUNT_PARAM                    "count"
#define GLOBUS_GRAM_CLIENT_STDIN_PARAM                    "stdin"
#define GLOBUS_GRAM_CLIENT_STDOUT_PARAM                   "stdout"
#define GLOBUS_GRAM_CLIENT_STDERR_PARAM                   "stderr"
#define GLOBUS_GRAM_CLIENT_MAX_WALL_TIME_PARAM            "maxwalltime"
#define GLOBUS_GRAM_CLIENT_MAX_CPU_TIME_PARAM             "maxcputime"
#define GLOBUS_GRAM_CLIENT_MAX_TIME_PARAM                 "maxtime"
#define GLOBUS_GRAM_CLIENT_PARADYN_PARAM                  "paradyn"
#define GLOBUS_GRAM_CLIENT_JOB_TYPE_PARAM                 "jobtype"
#define GLOBUS_GRAM_CLIENT_MYJOB_PARAM                    "grammyjob"
#define GLOBUS_GRAM_CLIENT_QUEUE_PARAM                    "queue"
#define GLOBUS_GRAM_CLIENT_PROJECT_PARAM                  "project"
#define GLOBUS_GRAM_CLIENT_HOST_COUNT_PARAM               "hostcount"
#define GLOBUS_GRAM_CLIENT_DRY_RUN_PARAM                  "dryrun"
#define GLOBUS_GRAM_CLIENT_MIN_MEMORY_PARAM               "minmemory"
#define GLOBUS_GRAM_CLIENT_MAX_MEMORY_PARAM               "maxmemory"
#define GLOBUS_GRAM_CLIENT_START_TIME_PARAM               "starttime"
#define GLOBUS_GRAM_CLIENT_RESERVATION_HANDLE_PARAM       "reservationhandle"
#define GLOBUS_GRAM_CLIENT_STDOUT_POSITION_PARAM          "stdoutposition"
#define GLOBUS_GRAM_CLIENT_STDERR_POSITION_PARAM          "stderrposition"
#define GLOBUS_GRAM_CLIENT_SAVE_STATE_PARAM               "savestate"
#define GLOBUS_GRAM_CLIENT_RESTART_PARAM                  "restart"
#define GLOBUS_GRAM_CLIENT_TWO_PHASE_COMMIT_PARAM         "twophase"
#define GLOBUS_GRAM_CLIENT_REMOTE_IO_URL_PARAM            "remoteiourl"

/*
 *  Job Default Constants
 */
#define GLOBUS_GRAM_CLIENT_DEFAULT_STDIN                  "/dev/null"
#define GLOBUS_GRAM_CLIENT_DEFAULT_STDOUT                 "/dev/null"
#define GLOBUS_GRAM_CLIENT_DEFAULT_STDERR                 "/dev/null"
#define GLOBUS_GRAM_CLIENT_DEFAULT_MYJOB                  "collective"
#define GLOBUS_GRAM_CLIENT_DEFAULT_JOBTYPE                "multiple"
#define GLOBUS_GRAM_CLIENT_DEFAULT_DRYRUN                 "no"
#define GLOBUS_GRAM_CLIENT_DEFAULT_START_TIME             "none"


/******************************************************************************
                               Type definitions
******************************************************************************/


typedef void (* globus_gram_client_callback_func_t)(void * user_callback_arg,
						    char * job_contact,
						    int state,
						    int errorcode);
typedef struct
{
    int dumb_time;
} globus_gram_client_time_t;

typedef enum
{
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_CANCEL   = 1,
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_SUSPEND  = 2,
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_RESUME   = 3,
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_PRIORITY = 4,
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_COMMIT   = 5,
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_COMMIT_EXTEND = 6,
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_STDIO_UPDATE = 7,
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_STDIO_SIZE = 8,
    GLOBUS_GRAM_CLIENT_JOB_SIGNAL_STOP_MANAGER = 9
} globus_gram_client_job_signal_t;

/******************************************************************************
                               Global variables
******************************************************************************/


/******************************************************************************
                              Function prototypes
******************************************************************************/
extern int 
globus_gram_client_callback_allow(
                          globus_gram_client_callback_func_t callback_func,
			  void * user_callback_arg,
			  char ** callback_contact);

extern int 
globus_gram_client_job_request(char * resource_manager_contact,
			       const char * description,
			       const int job_state_mask,
			       const char * callback_contact,
			       char ** job_contact);

extern int 
globus_gram_client_job_cancel(char * job_contact);

extern int
globus_gram_client_job_status(char * job_contact,
                              int * job_status,
                              int * failure_code);

extern int
globus_gram_client_job_signal(char * job_contact,
                              globus_gram_client_job_signal_t signal,
                              char * signal_arg,
                              int * job_status,
                              int * failure_code);

extern int
globus_gram_client_job_callback_register(char * job_contact,
                                         const int job_state_mask,
                                         const char * callback_contact,
                                         int * job_status,
                                         int * failure_code);

extern int
globus_gram_client_job_callback_unregister(char * job_contact,
                                           const char * callback_contact,
                                           int * job_status,
                                           int * failure_code);

extern int 
globus_gram_client_callback_disallow(char * callback_contact);

extern int 
globus_gram_client_callback_check();

extern int 
globus_gram_client_job_contact_free(char * job_contact);

extern const char *
globus_gram_client_error_string(int error_code);

extern int
globus_gram_client_version(void);

extern int 
globus_gram_client_ping(char * resource_manager_contact);

extern void
globus_gram_client_debug(void);

/*** unimplemented ***
extern int 
globus_gram_client_job_check(char * resource_manager_contact,
			     const char * description,
			     float required_confidence,
			     globus_gram_client_time_t * estimate,
			     globus_gram_client_time_t * interval_size);

extern int 
globus_gram_client_job_start_time(char * job_contact,
				  float required_confidence,
				  globus_gram_client_time_t * estimate,
				  globus_gram_client_time_t * interval_size);
*** unimplemented ***/

/******************************************************************************
 *			       Module definition
 *****************************************************************************/

extern int
globus_i_gram_client_activate(void);

extern int
globus_i_gram_client_deactivate(void);

#define GLOBUS_GRAM_CLIENT_MODULE (&globus_gram_client_module)

extern globus_module_descriptor_t	globus_gram_client_module;

/*** internal, shouldn't be here really ***/
extern void
globus_gram_client_error_7_hack_replace_message(const char* new_message);



/******************************************************************************
 * Backward compatibility
 *****************************************************************************/

#define GRAM_JOB_STATE_PENDING \
	GLOBUS_GRAM_CLIENT_JOB_STATE_PENDING
#define GRAM_JOB_STATE_ACTIVE \
	GLOBUS_GRAM_CLIENT_JOB_STATE_ACTIVE
#define GRAM_JOB_STATE_FAILED \
	GLOBUS_GRAM_CLIENT_JOB_STATE_FAILED
#define GRAM_JOB_STATE_DONE \
	GLOBUS_GRAM_CLIENT_JOB_STATE_DONE
#define GRAM_JOB_STATE_SUSPENDED \
	GLOBUS_GRAM_CLIENT_JOB_STATE_SUSPENDED
#define GRAM_JOB_STATE_ALL \
	GLOBUS_GRAM_CLIENT_JOB_STATE_ALL

/*
 *  Job Result Constants
 */
#define GRAM_SUCCESS \
        GLOBUS_SUCCESS
#define GRAM_ERROR_PARAMETER_NOT_SUPPORTED \
	GLOBUS_GRAM_CLIENT_ERROR_PARAMETER_NOT_SUPPORTED
#define GRAM_ERROR_INVALID_REQUEST \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_REQUEST
#define GRAM_ERROR_NO_RESOURCES \
	GLOBUS_GRAM_CLIENT_ERROR_NO_RESOURCES
#define GRAM_ERROR_BAD_DIRECTORY \
	GLOBUS_GRAM_CLIENT_ERROR_BAD_DIRECTORY
#define GRAM_ERROR_BAD_EXECUTABLE \
	GLOBUS_GRAM_CLIENT_ERROR_BAD_EXECUTABLE
#define GRAM_ERROR_INSUFFICIENT_FUNDS \
	GLOBUS_GRAM_CLIENT_ERROR_INSUFFICIENT_FUNDS
#define GRAM_ERROR_AUTHORIZATION \
	GLOBUS_GRAM_CLIENT_ERROR_AUTHORIZATION
#define GRAM_ERROR_USER_CANCELLED \
	GLOBUS_GRAM_CLIENT_ERROR_USER_CANCELLED
#define GRAM_ERROR_SYSTEM_CANCELLED \
	GLOBUS_GRAM_CLIENT_ERROR_SYSTEM_CANCELLED
#define GRAM_ERROR_PROTOCOL_FAILED \
	GLOBUS_GRAM_CLIENT_ERROR_PROTOCOL_FAILED
#define GRAM_ERROR_STDIN_NOT_FOUND \
	GLOBUS_GRAM_CLIENT_ERROR_STDIN_NOT_FOUND
#define GRAM_ERROR_CONNECTION_FAILED \
	GLOBUS_GRAM_CLIENT_ERROR_CONNECTION_FAILED
#define GRAM_ERROR_INVALID_MAXTIME \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_MAXTIME
#define GRAM_ERROR_INVALID_COUNT \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_COUNT
#define GRAM_ERROR_NULL_SPECIFICATION_TREE \
	GLOBUS_GRAM_CLIENT_ERROR_NULL_SPECIFICATION_TREE
#define GRAM_ERROR_JM_FAILED_ALLOW_ATTACH \
	GLOBUS_GRAM_CLIENT_ERROR_JM_FAILED_ALLOW_ATTACH
#define GRAM_ERROR_JOB_EXECUTION_FAILED \
	GLOBUS_GRAM_CLIENT_ERROR_JOB_EXECUTION_FAILED
#define GRAM_ERROR_INVALID_PARADYN \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_PARADYN
#define GRAM_ERROR_INVALID_JOBTYPE \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_JOBTYPE
#define GRAM_ERROR_INVALID_GRAM_MYJOB \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_GRAM_MYJOB
#define GRAM_ERROR_BAD_SCRIPT_ARG_FILE \
	GLOBUS_GRAM_CLIENT_ERROR_BAD_SCRIPT_ARG_FILE
#define GRAM_ERROR_ARG_FILE_CREATION_FAILED \
	GLOBUS_GRAM_CLIENT_ERROR_ARG_FILE_CREATION_FAILED
#define GRAM_ERROR_INVALID_JOBSTATE \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_JOBSTATE
#define GRAM_ERROR_INVALID_SCRIPT_REPLY \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_SCRIPT_REPLY
#define GRAM_ERROR_INVALID_SCRIPT_STATUS \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_SCRIPT_STATUS
#define GRAM_ERROR_JOBTYPE_NOT_SUPPORTED \
	GLOBUS_GRAM_CLIENT_ERROR_JOBTYPE_NOT_SUPPORTED
#define GRAM_ERROR_UNIMPLEMENTED \
	GLOBUS_GRAM_CLIENT_ERROR_UNIMPLEMENTED
#define GRAM_ERROR_TEMP_SCRIPT_FILE_FAILED \
	GLOBUS_GRAM_CLIENT_ERROR_TEMP_SCRIPT_FILE_FAILED
#define GRAM_ERROR_PROXY_FILE_RELOCATION_FAILED \
	GLOBUS_GRAM_CLIENT_ERROR_PROXY_FILE_RELOCATION_FAILED
#define GRAM_ERROR_PROXY_FILE_OPEN_FAILED \
	GLOBUS_GRAM_CLIENT_ERROR_PROXY_FILE_OPEN_FAILED
#define GRAM_ERROR_JOB_CANCEL_FAILED \
	GLOBUS_GRAM_CLIENT_ERROR_JOB_CANCEL_FAILED
#define GRAM_ERROR_INVALID_HOST_COUNT \
	GLOBUS_GRAM_CLIENT_ERROR_INVALID_HOST_COUNT
#define GRAM_ERROR_UNSUPPORTED_PARAMETER \
	GLOBUS_GRAM_CLIENT_ERROR_UNSUPPORTED_PARAMETER
#define GRAM_ERROR_LAST \
	GLOBUS_GRAM_CLIENT_ERROR_LAST

EXTERN_C_END
#endif /* GLOBUS_I_GRAM_CLIENT_INCLUDE */

