/*
 * Copyright 1999-2009 University of Chicago
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
#include "globus_gram_client.h"
#include "globus_gram_protocol.h"

#define test_assert(assertion, message) \
    if (!(assertion)) \
    { \
        printf("# %s:%d ", __FILE__, __LINE__); \
        printf message; \
        printf("\n"); \
        return 1; \
    }

#define TEST_CASE(x) { #x, x }
#define ARRAY_LEN(x) ((int) (sizeof(x)/sizeof(x[0])))

const char * rm_contact;
typedef struct
{
    char * name;
    int (*test_function)(void);
}
test_case;

static
int
version_test(void)
{
    int                                 rc;
    globus_gram_protocol_extension_t *  extension;
    globus_hashtable_t                  extensions;

    rc = globus_gram_client_get_jobmanager_version(
            rm_contact,
            &extensions);
    test_assert(
            rc == GLOBUS_SUCCESS,
            ("globus_gram_client_get_jobmanager_version() failed: %d (%s)",
            rc,
            globus_gram_protocol_error_string(rc)));
    extension = globus_hashtable_lookup(&extensions, "toolkit-version");
    test_assert(
            extension != NULL,
            ("toolkit-version attribute not present in response"));
    extension = globus_hashtable_lookup(&extensions, "version");
    test_assert(
            extension != NULL,
            ("version attribute not present in response"));
    globus_gram_protocol_hash_destroy(&extensions);

    return rc;
}
/* version_test() */

int main(int argc, char *argv[])
{
    test_case                           tests[] =
    {
        TEST_CASE(version_test)
    };
    int                                 i;
    int                                 rc;
    int                                 not_ok = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s RM-CONTACT\n", argv[0]);
        return 1;
    }
    rm_contact = argv[1];

    printf("1..%d\n", ARRAY_LEN(tests));

    globus_module_activate(GLOBUS_GRAM_CLIENT_MODULE);
    for (i = 0; i < ARRAY_LEN(tests); i++)
    {
        rc = tests[i].test_function();

        if (rc != 0)
        {
            not_ok++;
            printf("not ok # %s\n", tests[i].name);
        }
        else
        {
            printf("ok\n");
        }
    }
    globus_module_deactivate(GLOBUS_GRAM_CLIENT_MODULE);

    return not_ok;
}
