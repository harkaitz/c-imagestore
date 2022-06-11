#include "imagestore.h"
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/errno.h>

#define COPYRIGHT_LINE \
    "Bug reports, feature requests to gemini|https://harkadev.com/oss" "\n" \
    "Copyright (c) 2022 Harkaitz Agirre, harkaitz.aguirre@gmail.com" "\n" \
    ""

static const char help[] =
    "Usage: %s ..."                                                     "\n"
    ""                                                                  "\n"
    "Environment variables:"                                            "\n"
    ""                                                                  "\n"
    "    - IMAGESTORE_FORMAT : The format to save on (%s)."             "\n"
    "    - IMAGESTORE_PATH   : Directory to store on."                  "\n"
    "    - IMAGESTORE_URL    : URL of the directory images are stored." "\n"
    ""                                                                  "\n"
    "Command line options:"                                             "\n"
    ""                                                                  "\n"
    "    -n ID        : Identity for the file."                         "\n"
    "    -s WxH       : Image weight and height, by default `%s`."      "\n"
    "    -I [IGN].FMT : Read from standard input."                      "\n"
    "    -i FILE      : Read from file."                                "\n"
    ""                                                                  "\n"
    COPYRIGHT_LINE;

int main (int _argc, char *_argv[]) {
    char          *pname      = basename(_argv[0]);
    const char    *opts[10]   = {0};
    const char    *id         = NULL;
    const char    *input_fmt  = NULL;
    const char    *input      = NULL;
    int            input_fd   = 0;
    char           format[64] = {0};
    int            pid        = -1;
    int            retval     = 1;
    int            o;
    int            e;

    if (_argc == 1              ||
        !strcmp(_argv[1], "-h") ||
        !strcmp(_argv[1], "--help")) {
        printf(help, pname, IMAGESTORE_DEFAULT_FORMAT, IMAGESTORE_DEFAULT_SIZE);
        return 0;
    }

    openlog(pname, LOG_PERROR, LOG_USER);

    opts[0] = "imagestore_size";   opts[1] = NULL;
    opts[2] = "imagestore_format"; opts[3] = getenv("IMAGESTORE_FORMAT");
    opts[4] = "imagestore_path";   opts[5] = getenv("IMAGESTORE_PATH");
    opts[6] = "imagestore_url";    opts[7] = getenv("IMAGESTORE_URL");

    while((o = getopt (_argc, _argv, "n:s:I:i:")) != -1) {
        switch (o) {
        case 'n': id        = optarg; break;
        case 's': opts[1]   = optarg; break;
        case 'I': input_fmt = optarg; break;
        case 'i': input     = optarg; break;
        case '?':
        default:
            return 1;
        }
    }

    e = imagestore_library_init(opts);
    if (!e/*err*/) goto cleanup;
    
    if (input) {
        e = input_fd = open(input, O_RDONLY);
        if (e<0/*err*/) goto e_errno;
        if (!input_fmt) input_fmt = input;
    }

    if (!id/*err*/) goto e_missing_id;
    if (input_fmt) {
        e = filename_get_format(format, sizeof(format)-1, input_fmt);
        if (!e/*err*/) goto cleanup;
        e = imagestore_fork(&pid, id, format, input_fd);
        if (!e/*err*/) goto cleanup;
        e = imagestore_wait(pid); pid = -1;
        if (!e/*err*/) goto cleanup;
    } else {
        e = imagestore_get_url_fp(stdout, id, NULL);
        if (!e/*err*/) goto cleanup;
        fputc('\n', stdout);
    }

    retval = 0;
 cleanup:
    if (input_fd!=0) {
        close(input_fd);
    }
    if (pid!=-1) {
        kill(pid, SIGINT);
        imagestore_wait(pid);
    }
    imagestore_library_deinit();
    return retval;
 e_errno:
    syslog(LOG_ERR, "%s", strerror(errno));
    return 1;
 e_missing_id:
    syslog(LOG_ERR, "Missing ID: Specify it with -n.");
    return 1;
}
    
