#include "imagestore.h"
#include <sys/wait.h>
#include <sys/pathsearch.h>
#include <str/str2num.h>
#include <str/sizes.h>
#include <io/slog.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

struct {
    char   *convert_m;
    str64   size; /* <w>[x<h>]*/
    str64   format;
    strpath path;
    strurl  url;
} g_imagestore = {0};

bool imagestore_library_init(const char *_options[]) {
    int          e;
    const char  *var,*val;
    const char  *path;
    /* Read options. */
    for (const char **o = _options; _options && *o; o+=2) {
        var = *o;      
        val = *(o+1); if (!val) continue;
        switch(str2num(var, strcasecmp,
                       "convert"          , 1,
                       "imagestore_size"  , 2,
                       "imagestore_format", 3,
                       "imagestore_path"  , 4,
                       "imagestore_url"   , 5,
                       NULL)) {
        case 1: /* convert */
            free(g_imagestore.convert_m);
            g_imagestore.convert_m = strdup(val);
            if (!g_imagestore.convert_m/*err*/) goto e_errno;
            break;
        case 2: /* imagestore_size */
            strncpy(g_imagestore.size, val, sizeof(g_imagestore.size)-1);
            break;
        case 3: /* imagestore_format */
            strncpy(g_imagestore.format, val, sizeof(g_imagestore.format)-1);
            break;
        case 4: /* imagestore_path */
            strncpy(g_imagestore.path, val, sizeof(g_imagestore.path)-1);
            break;
        case 5: /* imagestore_url */
            strncpy(g_imagestore.url, val, sizeof(g_imagestore.url)-1);
            break;
        default:
            break;
        }
    }
    /* Search convert binary in the path. */
    if (!g_imagestore.convert_m) {
        path = getenv("PATH");
        if (!path/*err*/) goto e_missing_path_env;
        e = pathsearch(path, PATH_SEP, "convert", &g_imagestore.convert_m);
        if (!e/*err*/) goto e_cleanup;
    }
    /* Set default size and format. */
    if (g_imagestore.size[0]=='\0') {
        strcpy(g_imagestore.size, IMAGESTORE_DEFAULT_SIZE);
    }
    if (g_imagestore.format[0]=='\0') {
        strcpy(g_imagestore.format, IMAGESTORE_DEFAULT_FORMAT);
    }
    if (g_imagestore.path[0]=='\0' && g_imagestore.url[0]=='\0') {
        strcpy(g_imagestore.path, "/tmp");
        strcpy(g_imagestore.url, "file:///tmp");
    }
    /* Require paths. */
    if (g_imagestore.path[0]=='\0'/*err*/) goto e_missing_option_path;
    if (g_imagestore.url[0]=='\0'/*err*/)  goto e_missing_option_url;
    /* Success. */
    return true;
 e_errno:               error("%s", strerror(errno));                goto e_cleanup;
 e_missing_path_env:    error("Missing environment variable: PATH"); goto e_cleanup;
 e_missing_option_path: error("Missing option: imagestore_path");    goto e_cleanup;
 e_missing_option_url:  error("Missing option: imagestore_url");     goto e_cleanup;
 e_cleanup:             imagestore_library_deinit();                 return false;
}

void imagestore_library_deinit(void) {
    free(g_imagestore.convert_m);
    memset(&g_imagestore, 0, sizeof(g_imagestore));
}

bool imagestore_get_path(char _b[], size_t _bsz, const char _id[]) {
    int e = snprintf(_b, _bsz, "%s/%s.%s",
                     g_imagestore.path,
                     _id,
                     g_imagestore.format);
    if (e>=_bsz/*err*/) { error("Identifier too long"); return false; }
    return true;
}

bool imagestore_fork(pid_t *_pid, const char _id[], const char _ifmt[], int _fd0) {
    pid_t          pid         = -1;
    str64          path_i      = {0};
    strpath        path_o      = {0};
    int            e;
    snprintf(path_i, sizeof(path_i)-1, "%s:-", _ifmt);
    e = imagestore_get_path(path_o, sizeof(path_o)-1, _id);
    if (!e/*err*/) { return false; } 
    pid = fork();
    if (pid==-1/*err*/) { error("%s", strerror(errno)); return false; }
    if (pid==0) {
        if (_fd0!=0) {
            dup2(_fd0, 0);
            close(_fd0);
        }
        execl(g_imagestore.convert_m,
              g_imagestore.convert_m,
              path_i,
              "-resize", g_imagestore.size,
              "-strip" ,
              path_o,
              NULL);
        error("Can't execute %s: %s", g_imagestore.convert_m, strerror(errno));
        exit(1);
    }
    *_pid = pid;
    return true;
}

bool imagestore_wait(pid_t _pid) {
    int            status;
    int            e;
    if (_pid!=-1) {
        e = waitpid(_pid, &status, 0);
        if (e==-1/*err*/) goto e_errno;
        if (!WIFEXITED(status)/*err*/)  goto e_interrupted;
        if (WEXITSTATUS(status)/*err*/) goto e_convert_failed;
    }
    return true;
 e_errno:          error("imagestore: wait %i: %s", _pid, strerror(errno)); return false;
 e_interrupted:    error("imagestore: Interrupted.");        return false;
 e_convert_failed: error("imagestore: Convert failed.");     return false;
}

bool imagestore_save(const char _id[], const char *_ifmt, const void *_b, size_t _bsz) {
    pid_t          pid  = -1;
    int            p[2] = {-1,-1};
    int            e;
    bool           r = false;
    e = pipe(p)!=-1 &&
        fcntl(p[0], F_SETFD, FD_CLOEXEC)!=-1 &&
        fcntl(p[1], F_SETFD, FD_CLOEXEC)!=-1;
    if (!e/*err*/) goto e_errno;
    e = imagestore_fork(&pid, _id, _ifmt, p[0]);
    if (!e/*err*/) goto cleanup;
    close(p[0]); p[0] = -1;
    write(p[1], _b, _bsz);
    close(p[1]); p[1] = -1;
    e = imagestore_wait(pid);
    if (!e/*err*/) goto cleanup;
    r = true;
 cleanup:
    if (p[0]!=-1) close(p[0]);
    if (p[1]!=-1) close(p[1]);
    return r;
 e_errno:
    error("%s", strerror(errno)); 
    goto cleanup;
}

bool imagestore_get_url(char _b[], size_t _bsz, const char _id[], bool *_found) {
    strpath        path_i      = {0};
    struct stat    s;
    int            e;
    e = snprintf(_b, _bsz, "%s/%s.%s",
                 g_imagestore.url,
                 _id,
                 g_imagestore.format);
    if (e>=_bsz/*err*/) return false;
    e = imagestore_get_path(path_i, sizeof(path_i), _id);
    if (!e/*err*/) return false;
    if (stat(path_i, &s)!=-1) {
        if (_found) {
            *_found = true;
        }
        return true;
    } else {
        if (_found) {
            *_found = false;
            return true;
        } else {
            error("%s: %s", path_i, strerror(errno));
            return false;
        }
    }
}

bool imagestore_get_url_fp(FILE *_fp, const char _id[], bool *_found) {
    strpath b = {0}; int e;
    e = imagestore_get_url(b, sizeof(b), _id, _found);
    if (!e/*err*/) return false;
    if ((!_found || *_found) && b[0]) {
        fputs(b, _fp);
        fprintf(_fp, "?i=%li", random());
    }
    return true;
}

bool filename_get_format(char _b[], size_t _bsz, const char _filename[]) {
    const char *last_dot = NULL; size_t i; const char *c;
    for (const char *c = _filename; *c; c++) {
        if (*c=='.') last_dot = c;
    }
    if (!last_dot) {
        error("%s: Can't get the file format.", _filename);
        return false;
    }
    for (i=0,c=last_dot+1; i<_bsz && *c; i++, c++) {
        if(*c>=65 && *c<=90) {
            _b[i]=*c+32;
        } else {
            _b[i]=*c;
        }
    }
    if (i>32 || i>=_bsz) {
        error("%s: Format too long: %li", _filename, i);
        return false;
    }
    _b[i] = '\0';
    return true;
}

/**l*
 * 
 * MIT License
 * 
 * Bug reports, feature requests to gemini|https://harkadev.com/oss
 * Copyright (c) 2022 Harkaitz Agirre, harkaitz.aguirre@gmail.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **l*/
