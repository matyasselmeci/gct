#ifndef GLOBUS_DONT_DOCUMENT_INTERNAL
/**
 * @file grid_proxy_init.h
 * Globus GSI Proxy Utils
 * @author Sam Lang, Sam Meder
 *
 * $RCSfile$
 * $Revision$
 * $Date$
 */
#endif

#include "globus_common.h"
#include "globus_error.h"
#include "globus_gsi_cert_utils.h"
#include "globus_gsi_system_config.h"
#include "globus_gsi_proxy.h"
#include "globus_gsi_credential.h"

#define GLOBUS_GSI_PROXY_GENERIC_POLICY_OID "1.3.6.1.4.1.3536.1.222"
#define GLOBUS_GSI_PROXY_GENERIC_POLICY_SN  "GENERICPOLICY"
#define GLOBUS_GSI_PROXY_GENERIC_POLICY_LN  "Generic Policy Object"

#define SHORT_USAGE_FORMAT \
"\nSyntax: %s [-help][-pwstdin][-limited][-valid H:M] ...\n"

static int quiet = 0;
static int debug = 0;

static char *  LONG_USAGE = \
"\n" \
"    Options\n" \
"    -help, -usage             Displays usage\n" \
"    -version                  Displays version\n" \
"    -debug                    Enables extra debug output\n" \
"    -q                        Quiet mode, minimal output\n" \
"    -verify                   Verifies certificate to make proxy for\n" \
"    -pwstdin                  Allows passphrase from stdin\n" \
"    -limited                  Creates a limited proxy\n" \
"    -valid H:M                Proxy is valid for H hours and M " \
                               "minutes (default:12:00)\n" \
"    -hours H                  Deprecated support of hours option\n" \
"    -bits  B                  Number of bits in key {512|1024|2048|4096}\n" \
"\n" \
"    -cert     <certfile>      Non-standard location of user certificate\n" \
"    -key      <keyfile>       Non-standard location of user key\n" \
"    -certdir  <certdir>       Non-standard location of trusted cert dir\n" \
"    -out      <proxyfile>     Non-standard location of new proxy cert\n" \
"\n" ;
/*
"    -restriction <file>       Insert a restriction extension into the\n" \
"                              generated proxy.\n" \
"    -trusted-subgroup <grp>   Insert a trusted group extension into the\n" \
"                              generated proxy.\n" \
"    -untrusted-subgroup <grp> Insert a untrusted group extension into the\n" \
"                              generated proxy.\n" \
"\n";
*/

#   define args_show_version() \
    { \
        char buf[64]; \
        sprintf( buf, \
                 "%s-%s", \
                 PACKAGE, \
                 VERSION); \
        fprintf(stderr, "%s", buf); \
        globus_module_deactivate_all(); \
        exit(0); \
    }

#   define args_show_short_help() \
    { \
        fprintf(stderr, \
                SHORT_USAGE_FORMAT \
                "\nOption -help will display usage.\n", \
                program); \
        globus_module_deactivate_all(); \
        exit(0); \
    }

#   define args_show_full_usage() \
    { \
        fprintf(stderr, SHORT_USAGE_FORMAT \
                "%s", \
                program, \
                LONG_USAGE); \
        globus_module_deactivate_all(); \
        exit(0); \
    }

#   define args_error_message(errmsg) \
    { \
        fprintf(stderr, "ERROR: %s\n", errmsg); \
        args_show_short_help(); \
        globus_module_deactivate_all(); \
        exit(1); \
    }

#   define args_error(argnum, argval, errmsg) \
    { \
        char buf[1024]; \
        sprintf(buf, "argument #%d (%s) : %s", argnum, argval, errmsg); \
        args_error_message(buf); \
    }

#   define args_verify_next(argnum, argval, errmsg) \
    { \
        if ((argnum+1 >= argc) || (argv[argnum+1][0] == '-')) \
            args_error(argnum,argval,errmsg); \
    }

void
globus_i_gsi_proxy_utils_print_error(
    globus_result_t                     result,
    int                                 debug,
    const char *                        filename,
    int                                 line);

#define GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR \
    globus_i_gsi_proxy_utils_print_error(result, debug, __FILE__, __LINE__)

static int
globus_i_gsi_proxy_utils_pwstdin_callback(
    char *                              buf, 
    int                                 num, 
    int                                 w);

static void
globus_i_gsi_proxy_utils_key_gen_callback(
    int                                 p, 
    int                                 n,
    void *                              dummy);

int 
main(
    int                                 argc,
    char **                             argv)
{
    globus_result_t                     result  = GLOBUS_SUCCESS;
    /* default proxy to 512 bits */
    int                                 key_bits    = 512;
    /* default to a 12 hour cert */
    int                                 valid       = 12*60;
    /* dont restrict the proxy */
    int                                 verify      = 0;
    int                                 arg_index;
    char *                              user_cert_filename = NULL;
    char *                              user_key_filename = NULL;
    char *                              tmp_user_cert_filename = NULL;
    char *                              tmp_user_key_filename = NULL;
    char *                              proxy_out_filename = NULL;
    char *                              ca_cert_dir = NULL;
    char *                              argp;
    char *                              program = NULL;
    globus_gsi_proxy_handle_t           proxy_handle = NULL;
    globus_gsi_proxy_handle_attrs_t     proxy_handle_attrs = NULL;
    globus_gsi_callback_data_t          callback_data = NULL;
    globus_gsi_cred_handle_attrs_t      cred_handle_attrs = NULL;
    globus_gsi_cred_handle_t            cred_handle = NULL;
    globus_gsi_cred_handle_t            proxy_cred_handle = NULL;
    globus_gsi_cert_utils_proxy_type_t  proxy_type = GLOBUS_FULL_PROXY;
    BIO *                               pem_proxy_bio = NULL;
    time_t                              goodtill;
    time_t                              lifetime;
/*
    char *                              restriction_buf = NULL;
    size_t                              restriction_buf_len = 0;
    char *                              restriction_filename = NULL;
    char *                              policy_language = NULL;
    int                                 policy_NID;
*/
    int                                 (*pw_cb)() = NULL;
/*
    char *                              trusted_subgroup = NULL;
    char *                              untrusted_subgroup = NULL;
    char *                              subgroup = NULL;
*/
    int                                 return_value = 0;
    
    if(globus_module_activate(GLOBUS_GSI_PROXY_MODULE) != (int)GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(
            stderr,
            "\n\nERROR: Couldn't load module: GLOBUS_GSI_PROXY_MODULE.\n"
            "Make sure Globus is installed correctly.\n\n");
        exit(1);
    }

    if(globus_module_activate(GLOBUS_GSI_CALLBACK_MODULE) != (int)GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(
            stderr,
            "\n\nERROR: Couldn't load module: GLOBUS_GSI_CALLBACK_MODULE.\n"
            "Make sure Globus is installed correctly.\n\n");
        globus_module_deactivate_all();
        exit(1);
    }

    /* get the program name */
    if (strrchr(argv[0], '/'))
    {
        program = strrchr(argv[0], '/') + 1;
    }
    else
    {
        program = argv[0];
    }

    /* parse the arguments */
    for(arg_index = 1; arg_index < argc; ++arg_index)
    {
        argp = argv[arg_index];

        if (strncmp(argp, "--", 2) == 0)
        {
            if (argp[2] != '\0')
            {
                args_error(arg_index, argp, 
                           "double-dashed options are not allowed");
            }
            else
            {
                /* no more parsing */
                arg_index = argc + 1;
                continue;
            }
        }
        if((strcmp(argp, "-help") == 0) ||
           (strcmp(argp, "-usage") == 0))
        {
            args_show_full_usage();
        }
        else if(strcmp(argp, "-version") == 0)
        {
            args_show_version();
        }
        else if(strcmp(argp, "-cert") == 0)
        {
            args_verify_next(arg_index, argp, "need a file name argument");
            user_cert_filename = argv[++arg_index];
        }
        else if(strcmp(argp, "-certdir") == 0)
        {
            args_verify_next(arg_index, argp, "need a file name argument");
            ca_cert_dir = strdup(argv[++arg_index]);
        }
        else if(strcmp(argp, "-out") == 0)
        {
            args_verify_next(arg_index, argp, "need a file name argument");
            proxy_out_filename = strdup(argv[++arg_index]);
        }
        else if(strcmp(argp, "-key") == 0)
        {
            args_verify_next(arg_index, argp, "need a file name argument");
            user_key_filename = argv[++arg_index];
        }
        else if(strcmp(argp, "-valid") == 0)
        {
            int                         hours;
            int                         minutes;
            args_verify_next(arg_index, argp, 
                             "valid time argument H:M missing");
            if(sscanf(argv[++arg_index], "%d:%d", &hours, &minutes) < 2)
            {
                args_error(
                    arg_index, argp, 
                    "value must be in the format: H:M");
            }
            if(hours < 0)
            {
                args_error(
                    arg_index, argp,
                    "specified hours must be a nonnegative integer");
            }
            if(minutes < 0 || minutes > 60)
            {
                args_error(
                    arg_index, argp,
                    "specified minutes must "
                    "be in the range 0-60");
            }
            valid = (hours * 60) + minutes;
        }
        else if(strcmp(argp, "-hours") == 0)
        {
            int                           hours;
            args_verify_next(arg_index, argp, "integer argument missing");
            hours = atoi(argv[arg_index + 1]);
            valid = hours * 60;
            arg_index++;
        }
        else if(strcmp(argp, "-bits") == 0)
        {
            args_verify_next(arg_index, argp, "integer argument missing");
            key_bits = atoi(argv[arg_index + 1]);
            if((key_bits != 512) && (key_bits != 1024) && 
               (key_bits != 2048) && (key_bits != 4096))
            {
                args_error(arg_index, argp, 
                           "value must be one of 512,1024,2048,4096");
            }
            arg_index++;
        }
        else if(strcmp(argp, "-debug") == 0)
        {
            debug++;
        }
        else if(strcmp(argp, "-limited") == 0)
        {
            proxy_type = GLOBUS_LIMITED_PROXY;
        }
        else if(strcmp(argp, "-verify") == 0)
        {
            verify++;
        }
        else if(strcmp(argp, "-q") == 0)
        {
            quiet++;
        }
        else if(strcmp(argp, "-pwstdin") == 0)
        {
            pw_cb = globus_i_gsi_proxy_utils_pwstdin_callback;
        }
/*
        else if(strcmp(argp, "-policy") == 0)
        {
            args_verify_next(arg_index, argp, 
                             "restriction file name missing");
            restriction_filename = argv[++arg_index];
            proxy_type = GLOBUS_RESTRICTED_PROXY;
        }
        else if(strcmp(argp, "-pl") == 0 &&
                strcmp(argp, "-policy-language") == 0)
        {
            args_verify_next(arg_index, argp, "policy language missing");
            policy_language = argv[++arg_index];
            proxy_type = GLOBUS_RESTRICTED_PROXY;
        }
        else if(strcmp(argp, "-trusted-subgroup") == 0)
        {
            args_verify_next(arg_index, argp, "subgroup name missing");
            if(untrusted_subgroup != NULL ||
               trusted_subgroup != NULL)
            {
                args_error(arg_index, argp, 
                           "You may only specify one subgroup.");
            }
            trusted_subgroup = argv[++arg_index];
            proxy_type = GLOBUS_RESTRICTED_PROXY;
        }
        else if (strcmp(argp, "-untrusted-subgroup") == 0)
        {
            args_verify_next(arg_index, argp, "subgroup name missing");
            if(untrusted_subgroup != NULL ||
               trusted_subgroup != NULL)
            {
                args_error(arg_index, argp, 
                           "You may only specify one subgroup.");
            }
            untrusted_subgroup = argv[++arg_index];
            proxy_type = GLOBUS_RESTRICTED_PROXY;
        }
*/
        else
        {
            args_error(arg_index, argp, "unrecognized option");
        }
    }

    result = globus_gsi_proxy_handle_attrs_init(
        &proxy_handle_attrs);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr, 
                            "\n\nERROR: Couldn't initialize "
                            "the proxy handle attributes.\n");
    }

    /* set the time valid in the proxy handle attributes
     * used to be hours - now the time valid needs to be set in minutes 
     */
    result = globus_gsi_proxy_handle_attrs_set_time_valid(
        proxy_handle_attrs, valid);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr,
                            "\n\nERROR: Couldn't set the validity time "
                            "of the proxy cert to %d minutes.\n", valid);
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    /* set the key bits for the proxy cert in the proxy handle
     * attributes
     */
    result = globus_gsi_proxy_handle_attrs_set_keybits(
        proxy_handle_attrs, key_bits);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr,
                            "\n\nERROR: Couldn't set the key bits for "
                            "the private key of the proxy certificate\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }
    
    result = globus_gsi_proxy_handle_attrs_set_key_gen_callback(
        proxy_handle_attrs, 
        globus_i_gsi_proxy_utils_key_gen_callback);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(
            stderr,
            "\n\nERROR: Couldn't set the key generation callback function\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    result = globus_gsi_proxy_handle_init(&proxy_handle, proxy_handle_attrs);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(
            stderr,
            "\n\nERROR: Couldn't initialize the proxy handle\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    if(!user_cert_filename || !user_key_filename)
    {
        result = GLOBUS_GSI_SYSCONFIG_GET_USER_CERT_FILENAME(
            &tmp_user_cert_filename,
            &tmp_user_key_filename);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(stderr,
                                "\n\nERROR: Couldn't find valid credentials "
                                "to generate a proxy.\n");
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }

        if(tmp_user_cert_filename == tmp_user_key_filename)
        {
            /* supposed to be a pkcs12 formated credential */
            user_cert_filename = user_key_filename
                               = tmp_user_key_filename;
        }
    }

    if(!user_cert_filename)
    {
        user_cert_filename = tmp_user_cert_filename;
    }

    if(!user_key_filename)
    {
        user_key_filename = tmp_user_key_filename;
    }
    
    if(debug)
    {
        globus_libc_fprintf(stderr,
                            "\nUser Cert File: %s\nUser Key File: %s\n",
                            user_cert_filename, user_key_filename);
    }

    if (!strncmp(user_cert_filename, "SC:", 3))
    {
        EVP_set_pw_prompt("Enter card pin:");
    }
    else
    {
        EVP_set_pw_prompt(quiet? "Enter GRID pass phrase:" :
                          "Enter GRID pass phrase for this identity:");
    }

    if(!ca_cert_dir)
    {
        result = GLOBUS_GSI_SYSCONFIG_GET_CERT_DIR(&ca_cert_dir);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(
                stderr,
                "\n\nERROR: Couldn't find a valid trusted certificate "
                "directory\n");
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }
    }

    if(debug)
    {
        globus_libc_fprintf(stderr, 
                            "\nTrusted CA Cert Dir: %s\n", 
                            ca_cert_dir);
    }

    if(!proxy_out_filename)
    {
        result = GLOBUS_GSI_SYSCONFIG_GET_PROXY_FILENAME(
            &proxy_out_filename,
            GLOBUS_PROXY_FILE_OUTPUT);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(
                stderr,
                "\n\nERROR: Couldn't find a valid location "
                "to write the proxy file\n");
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }
    }
    else
    {
        /* verify that the directory path of proxy_out_filename
         * exists and is writeable
         */
        globus_gsi_statcheck_t          file_status;
        char *                          proxy_absolute_path = NULL;
        char *                          temp_filename = NULL;
        char *                          temp_dir = NULL;

        /* first, make absolute path */
        result = GLOBUS_GSI_SYSCONFIG_MAKE_ABSOLUTE_PATH_FOR_FILENAME(
            proxy_out_filename,
            &proxy_absolute_path);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(
                stderr,
                "\n\nERROR: Can't create the absolute path "
                "of the proxy filename: %s",
                proxy_out_filename);
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }

        if(proxy_out_filename)
        {
            free(proxy_out_filename);
        }
        
        proxy_out_filename = proxy_absolute_path;

        /* then split */
        result = GLOBUS_GSI_SYSCONFIG_SPLIT_DIR_AND_FILENAME(
            proxy_absolute_path,
            &temp_dir,
            &temp_filename);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(
                stderr,
                "\n\nERROR: Can't split the full path into "
                "directory and filename. The full path is: %s", 
                proxy_absolute_path);
            if(proxy_absolute_path)
            {
                free(proxy_absolute_path);
                proxy_absolute_path = NULL;
            }
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }
                
        result = GLOBUS_GSI_SYSCONFIG_FILE_EXISTS(temp_dir, &file_status);
        if(result != GLOBUS_SUCCESS ||
           file_status != GLOBUS_FILE_DIR)
        {
            globus_libc_fprintf(
                stderr, 
                "\n\nERROR: %s is not a valid directory for writing the "
                "proxy certificate\n\n",
                temp_dir);

            if(temp_dir)
            {
                free(temp_dir);
                temp_dir = NULL;
            }
            
            if(temp_filename)
            {
                free(temp_filename);
                temp_filename = NULL;
            }

            if(result != GLOBUS_SUCCESS)
            {
                GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
            }
            else
            {
                globus_module_deactivate_all();             
                exit(1);
            }
        }

        if(temp_dir);
        {
            free(temp_dir);
            temp_dir = NULL;
        }
        
        if(temp_filename)
        {
            free(temp_filename);
            temp_filename = NULL;
        }
    }

    if(debug)
    {
        globus_libc_fprintf(stderr, "\nOutput File: %s\n", proxy_out_filename);
    }

    result = globus_gsi_cred_handle_attrs_init(&cred_handle_attrs);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr,
                            "\n\nERROR: Couldn't initialize credential "
                            "handle attributes\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    result = globus_gsi_cred_handle_attrs_set_ca_cert_dir(
        cred_handle_attrs, 
        ca_cert_dir);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(
            stderr,
            "\n\nERROR: Couldn't set the trusted CA certificate "
            "directory in the credential handle attributes\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    result = globus_gsi_cred_handle_init(&cred_handle, cred_handle_attrs);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr,
                            "\n\nERROR: Couldn't initialize credential "
                            "handle\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    result = globus_gsi_cred_handle_attrs_destroy(cred_handle_attrs);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr,
                            "\n\nERROR: Couldn't destroy credential "
                            "handle attributes.\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    if(strstr(user_cert_filename, ".p12"))
    {
        /* we have a pkcs12 credential */
        result = globus_gsi_cred_read_pkcs12(
            cred_handle,
            user_cert_filename);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(
                stderr,
                "\n\nERROR: Couldn't read in PKCS12 credential "
                "from file: %s\n", user_cert_filename);
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }

        if (!quiet)
        {
            char *                          subject = NULL;
            result = globus_gsi_cred_get_subject_name(cred_handle, &subject);
            if(result != GLOBUS_SUCCESS)
            {
                globus_libc_fprintf(
                    stderr,
                    "\n\nERROR: The subject name of the "
                    "user certificate could "
                    "not be retrieved\n");
                GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
            }
            
            printf("Your identity: %s\n", subject);
            if(subject)
            {
                free(subject);
                subject = NULL;
            }
        }
    }
    else
    {
        result = globus_gsi_cred_read_cert(
            cred_handle,
            user_cert_filename);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(
                stderr,
                "\n\nERROR: Couldn't read user certificate\n"
                "cert file location: %s\n\n", 
                user_cert_filename);
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }

        if (!quiet)
        {
            char *                          subject = NULL;
            result = globus_gsi_cred_get_subject_name(cred_handle, &subject);
            if(result != GLOBUS_SUCCESS)
            {
                globus_libc_fprintf(
                    stderr,
                    "\n\nERROR: The subject name of the "
                    "user certificate could "
                    "not be retrieved\n");
                GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
            }
            
            printf("Your identity: %s\n", subject);
            if(subject)
            {
                free(subject);
                subject = NULL;
            }
        }
        
        result = globus_gsi_cred_read_key(
            cred_handle,
            user_key_filename,
            pw_cb);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(
                stderr,
                "\n\nERROR: Couldn't read user key\n"
                "key file location: %s\n\n", 
                user_key_filename);
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }
    }

    /* add restrictions now */
/** PROXY RESTRICTIONS/GROUPS DISABLED CURRENTLY
    if(restriction_filename)
    {
        int                             restriction_buf_size = 0;
        FILE *                          restriction_fp = NULL;
        
        restriction_fp = fopen(restriction_filename, "r");
        if(!restriction_fp)
        {
            fprintf(stderr, 
                    "\n\nERROR: Unable to open restrictions "
                    " file: %s\n\n", restriction_filename);
            exit(1);
        }

        do 
        {
            restriction_buf_size += 512;
            
            * First time through this is a essentially a malloc() *
            restriction_buf = realloc(restriction_buf,
                                      restriction_buf_size);

            if (restriction_buf == NULL)
            {
                fprintf(stderr, 
                        "\nAllocation of space for "
                        "restriction buffer failed\n\n");
                exit(1);
            }

            restriction_buf_len += 
                fread(&restriction_buf[restriction_buf_len], 1, 
                      512, restriction_fp);

            *
             * If we read 512 bytes then restriction_buf_len and
             * restriction_buf_size will be equal and there is
             * probably more to read. Even if there isn't more
             * to read, no harm is done, we just allocate 512
             * bytes we don't end up using.
             *
        }
        while (restriction_buf_len == restriction_buf_size);
        
        if (restriction_buf_len > 0)
        {
            if(!policy_language)
            {
                policy_NID = 
                    OBJ_create(GLOBUS_GSI_PROXY_GENERIC_POLICY_OID,
                               GLOBUS_GSI_PROXY_GENERIC_POLICY_SN,
                               GLOBUS_GSI_PROXY_GENERIC_POLICY_LN);
            }
            else
            {
                policy_NID = 
                    OBJ_create(policy_language,
                               policy_language,
                               policy_language);
            }

            result = globus_gsi_proxy_handle_set_policy(
                proxy_handle,
                restriction_buf,
                restriction_buf_len,
                policy_NID);
            if(result != GLOBUS_SUCCESS)
            {
                globus_libc_fprintf(stderr,
                                    "\n\nERROR: Can't set the restriction "
                                    "policy in the proxy handle\n");
                GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
            }
        }   

        fclose(restriction_fp);
    }

    subgroup = trusted_subgroup ? trusted_subgroup : untrusted_subgroup;

    if(subgroup)
    {
        result = globus_gsi_proxy_handle_set_group(
            proxy_handle,
            subgroup,
            trusted_subgroup ? 1 : 0);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(stderr,
                                "\n\nERROR: Can't set the group of the proxy "
                                "credential\n");
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }
    }
** PROXY RESTRICTIONS/GROUPS currently DISABLED **/

    if (!quiet)
    {
        printf("Creating proxy ");
        fflush(stdout);
    }

    result = globus_gsi_proxy_create_signed(
        proxy_handle,
        cred_handle,
        &proxy_cred_handle);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr,
                           "\n\nERROR: Couldn't create proxy certificate\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    if (!quiet)
    {
        fprintf(stdout, " Done\n");
    }

    if(verify)
    {
        result = globus_gsi_callback_data_init(&callback_data);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(stderr,
                                "\n\nERROR: Couldn't initialize callback data "
                                "for credential verification\n");
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }

        result = globus_gsi_callback_set_cert_dir(
            callback_data,
            ca_cert_dir);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(stderr,
                                "\n\nERROR: Couldn't set the trusted "
                                "certificate directory in the callback "
                                "data\n");
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }

        result = globus_gsi_cred_verify_cert_chain(
            proxy_cred_handle,
            callback_data);
        if(result != GLOBUS_SUCCESS)
        {
            globus_libc_fprintf(
                stderr,
                "\n\nERROR: Couldn't verify the authenticity of the user's "
                "credential to generate a proxy from.\n");
            GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
        }

        globus_libc_fprintf(
            stdout,
            "Proxy Verify OK\n");
    }

    if(ca_cert_dir)
    {
        free(ca_cert_dir);
        ca_cert_dir = NULL;
    }

    result = globus_gsi_cred_write_proxy(proxy_cred_handle,
                                         proxy_out_filename);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(
            stderr,
            "\n\nERROR: The proxy credential could not be "
            "written to the output file.\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    if(proxy_out_filename)
    {
        free(proxy_out_filename);
        proxy_out_filename = NULL;
    }

    result = globus_gsi_cred_get_lifetime(
        cred_handle,
        &lifetime);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr,
                            "\n\nERROR: Can't get the lifetime of the proxy "
                            "credential.\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    result = globus_gsi_cred_get_goodtill(
        proxy_cred_handle,
        &goodtill);
    if(result != GLOBUS_SUCCESS)
    {
        globus_libc_fprintf(stderr,
                            "\n\nERROR: Can't get the expiration date of the "
                            "proxy credential.\n");
        GLOBUS_I_GSI_PROXY_UTILS_PRINT_ERROR;
    }

    if(lifetime < 0)
    {
        globus_libc_fprintf(
            stderr,
            "\n\nERROR: Your certificate has expired: %s\n\n", 
            asctime(localtime(&goodtill)));
        globus_module_deactivate_all();
        exit(2);
    }
    else if(lifetime < (valid * 60))
    {
        globus_libc_fprintf(
            stderr, 
            "\nWarning: your certificate and proxy will expire %s "
            "which is within the requested lifetime of the proxy\n",
            asctime(localtime(&goodtill)));
        return_value = 1;
    }
    else if(!quiet)
    {
        globus_libc_fprintf(
            stdout,
            "Your proxy is valid until: %s", 
            asctime(localtime(&goodtill)));
    }

    BIO_free(pem_proxy_bio);

    globus_gsi_proxy_handle_destroy(proxy_handle);
    globus_gsi_cred_handle_destroy(cred_handle);
    globus_gsi_cred_handle_destroy(proxy_cred_handle);
    globus_gsi_callback_data_destroy(callback_data);

    if(tmp_user_cert_filename)
    {
        free(tmp_user_cert_filename);
    }

    if(tmp_user_key_filename)
    {
        free(tmp_user_key_filename);
    }

    globus_module_deactivate_all();
    exit(return_value);
}

static int
globus_i_gsi_proxy_utils_pwstdin_callback(
    char *                              buf, 
    int                                 num, 
    int                                 w)
{
    int                                 i;

    if (!(fgets(buf, num, stdin))) {
        fprintf(stderr, "Failed to read pass-phrase from stdin\n");
        return -1;
    }
    i = strlen(buf);
    if (buf[i-1] == '\n') {
        buf[i-1] = '\0';
        i--;
    }
    return i;       

}

static void
globus_i_gsi_proxy_utils_key_gen_callback(int p, int n, void * dummy)
{
    char c='B';

    if (quiet) return;

    if (p == 0) c='.';
    if (p == 1) c='+';
    if (p == 2) c='*';
    if (p == 3) c='\n';
    if (!debug) c = '.';
    fputc(c, stdout);
    fflush(stdout);
}

void
globus_i_gsi_proxy_utils_print_error(
    globus_result_t                     result,
    int                                 debug,
    const char *                        filename,
    int                                 line)
{
    globus_object_t *                   error_obj;

    error_obj = globus_error_get(result);
    if(debug)
    {
        char *                          error_string = NULL;
        globus_libc_fprintf(stderr,
                            "\n%s:%d:",
                            filename, line);
        error_string = globus_error_print_chain(error_obj);
        globus_libc_fprintf(stderr, "%s\n", error_string);
        if(error_string)
        {
            globus_libc_free(error_string);
        }
    }
    else 
    {
        globus_libc_fprintf(stderr,
                            "Use -debug for further information.\n\n");
    }
    globus_object_free(error_obj);
    exit(1);
}
