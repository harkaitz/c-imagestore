#ifndef IMAGESTORE_H
#define IMAGESTORE_H

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

bool imagestore_library_init   (const char *_options[]);
void imagestore_library_deinit (void);

bool imagestore_fork(pid_t*, const char _id[], const char *_ifmt, int _fd0);
bool imagestore_wait(pid_t);
bool imagestore_save(const char _id[], const char *_ifmt, const void *_b, size_t _bsz);


bool imagestore_get_path   (char _b[], size_t _bsz, const char _id[]);
bool imagestore_get_url    (char _b[], size_t _bsz, const char _id[], bool *_found);
bool imagestore_get_url_fp (FILE *_fp, const char _id[], bool *_found);

bool filename_get_format(char _b[], size_t _bsz, const char _filename[]);

#define IMAGESTORE_DEFAULT_SIZE   "128"
#define IMAGESTORE_DEFAULT_FORMAT "png"

#endif
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
