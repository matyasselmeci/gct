/*
 * Copyright 1999-2006 University of Chicago
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "globus_common.h"
#include "globus_gram_client.h"
#include "globus_preload.h"

typedef struct
{
    int					state;
    int					errorcode;
    globus_mutex_t			mutex;
    globus_cond_t			cond;
    globus_bool_t			refresh_called;
    globus_gram_protocol_error_t	refresh_result;
}
monitor_t;

static
void
gram_state_callback(
    void *				arg,
    char *				job_contact,
    int					state,
    int					errorcode);

static
void
nonblocking_callback(
    void *				arg,
    globus_gram_protocol_error_t	error_code,
    const char *			job_contact,
    globus_gram_protocol_job_state_t	state,
    globus_gram_protocol_error_t	job_failure_code);

int main(int argc, char *argv[])
{
    char *				rm_contact;
    char *				callback_contact;
    char *				job_contact;
    monitor_t				monitor;
    int					rc = 0;

    LTDL_SET_PRELOADED_SYMBOLS();
    printf("1..1\n");
    rm_contact = getenv("CONTACT_STRING");
    if (argc == 2)
    {
        rm_contact = argv[1];
    }
    if (rm_contact == NULL)
    {
	fprintf(stderr, "Usage: %s resource-manager-contact\n", argv[0]);
	exit(1);
    }
    rc = globus_module_activate(GLOBUS_COMMON_MODULE);
    if(rc)
    {
	goto end;
    }
    rc = globus_module_activate(GLOBUS_GRAM_CLIENT_MODULE);
    if(rc)
    {
	goto disable_modules;
    }

    globus_mutex_init(&monitor.mutex ,GLOBUS_NULL);
    globus_cond_init(&monitor.cond, GLOBUS_NULL);
    monitor.state = GLOBUS_GRAM_PROTOCOL_JOB_STATE_PENDING;
    monitor.refresh_called = GLOBUS_FALSE;
    monitor.refresh_result = 0;

    rc = globus_gram_client_callback_allow(gram_state_callback,
	                                   &monitor,
					   &callback_contact);
    if(rc != GLOBUS_SUCCESS)
    {
	fprintf(stderr,
		"Error creating callback contact %s.\n",
		globus_gram_client_error_string(rc));

	goto error_exit;
    }

    globus_mutex_lock(&monitor.mutex);
    rc = globus_gram_client_job_request(
	    rm_contact,
	    "&(executable=/bin/sleep)(arguments=10)",
	    GLOBUS_GRAM_PROTOCOL_JOB_STATE_ALL,
	    callback_contact,
	    &job_contact);

    if(rc != GLOBUS_SUCCESS)
    {
	fprintf(stderr,
		"Error submitting job request %s.\n",
		globus_gram_client_error_string(rc));

	goto destroy_callback_contact;
    }

    while(monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_ACTIVE &&
	  monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED &&
	  monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE)
    {
	globus_cond_wait(&monitor.cond, &monitor.mutex);
    }

    if(monitor.state == GLOBUS_GRAM_PROTOCOL_JOB_STATE_ACTIVE)
    {
	rc = globus_gram_client_register_job_refresh_credentials(
		job_contact,
		GSS_C_NO_CREDENTIAL,
		GLOBUS_GRAM_CLIENT_NO_ATTR,
		nonblocking_callback,
		&monitor);

	while(!monitor.refresh_called)
	{
	    globus_cond_wait(&monitor.cond, &monitor.mutex);
	}

	if(monitor.refresh_result != GLOBUS_SUCCESS)
	{
	    fprintf(stderr,
		    "Error refrehsing credentials: %s.\n",
		    globus_gram_client_error_string(rc));

	    goto destroy_callback_contact;
	}
    }

    while(monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE &&
	  monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED)
    {
	globus_cond_wait(&monitor.cond, &monitor.mutex);
    }
    rc = monitor.errorcode;
destroy_callback_contact:
    globus_mutex_unlock(&monitor.mutex);
    globus_gram_client_callback_disallow(callback_contact);
    globus_libc_free(callback_contact);
    globus_libc_free(job_contact);
error_exit:
    globus_mutex_destroy(&monitor.mutex);
    globus_cond_destroy(&monitor.cond);
disable_modules:
    globus_module_deactivate_all();
end:
    printf("%s 1 register-refresh-credentials-test\n", rc == 0 ? "ok" : "not ok");
    return rc;
}
/* main() */

static
void
gram_state_callback(
    void *				arg,
    char *				job_contact,
    int					state,
    int					errorcode)
{
    monitor_t *				monitor;

    monitor = arg;

    globus_mutex_lock(&monitor->mutex);
    monitor->state = state;
    monitor->errorcode = errorcode;
    globus_cond_signal(&monitor->cond);
    globus_mutex_unlock(&monitor->mutex);
}
/* gram_state_callback() */

static
void
nonblocking_callback(
    void *				arg,
    globus_gram_protocol_error_t	error_code,
    const char *			job_contact,
    globus_gram_protocol_job_state_t	state,
    globus_gram_protocol_error_t	job_failure_code)
{
    monitor_t *				monitor;

    monitor = arg;

    globus_mutex_lock(&monitor->mutex);
    monitor->refresh_called = GLOBUS_TRUE;
    monitor->refresh_result = error_code;
    globus_cond_signal(&monitor->cond);
    globus_mutex_unlock(&monitor->mutex);
}
