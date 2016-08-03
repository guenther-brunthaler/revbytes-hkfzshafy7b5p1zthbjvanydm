#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#define MIN_BUFFER_SIZE 4ul * 1024
#define MAX_BUFFER_SIZE 4ul * 1024 * 1024

#define DIM(array) (sizeof (array) / sizeof *(array))

int main(int argc, char **argv) {
   if (argc >= 2) {
      int argind;
      char const *arg;
      if (strlen(arg= argv[argind= 1]) >= 2 && *arg == '-') {
         if (strcmp(arg, "--")) {
            (void)fprintf(stderr, "Unknown option %s!\n", arg);
            fail:
            return EXIT_FAILURE;
         }
         ++argind;
      }
      if (argind < argc) {
         if (argind + 1 != argc) {
            (void)fputs("Too many arguments!\n", stderr);
            goto fail;
         }
         if (strcmp(arg= argv[argind], "-") && !freopen(arg, "rb", stdin)) {
            (void)fprintf(stderr, "Could not open '%s' for reading!\n", arg);
            goto fail;
         }
      }
   }
   {
      char *buffer;
      unsigned long buffer_size;
      int seekable;
      for (buffer_size= MAX_BUFFER_SIZE; !(buffer= malloc(buffer_size)); ) {
         if (buffer_size == MIN_BUFFER_SIZE) {
            (void)fputs("Cannot allocate buffer!", stderr);
            goto fail;
         }
         assert(buffer_size > MIN_BUFFER_SIZE);
         buffer_size= MIN_BUFFER_SIZE + (buffer_size - MIN_BUFFER_SIZE >> 1);
      }
      seekable= !fseek(stdin, 0, SEEK_END);
      for (;;) {
         assert((long)buffer_size + 1 > (long)buffer_size);
         if (seekable) {
            while (fseek(stdin, -(long)buffer_size, SEEK_CUR)) {
               if (!buffer_size) {
                  seek_error_w_buf:
                  (void)fputs("Seek error!\n", stderr);
                  fail_w_buf:
                  free(buffer);
                  goto fail;
               }
               buffer_size>>= 1;
            }
         }
         {
            size_t read;
            if (
                  (
                     read= fread(
                        buffer, sizeof(char), (size_t)buffer_size, stdin
                     )
                  )
               != (size_t)buffer_size
            ) {
               assert(read < buffer_size);
               if (ferror(stdin)) {
                  read_error_w_buf:
                  (void)fputs("Read error!", stderr);
                  goto fail_w_buf;
               }
               assert(feof(stdin));
               if (seekable) goto seek_error_w_buf; /* File shrunk??? */
               assert((unsigned long)read == read);
               buffer_size= (unsigned long)read;
            }
         }
         if (!seekable && !feof(stdin)) {
            if (getchar() == EOF) {
               if (ferror(stdin)) goto read_error_w_buf;
               assert(feof(stdin));
            } else {
               (void)fputs(
                     "Non-seekable input stream is too large for internal"
                     " buffer! Please provide it as a seekable stream"
                     " instead.\n"
                  ,  stderr
               );
            }
         }
         if (!buffer_size) break;
         {
            unsigned long i= 0, j= buffer_size - 1;
            while (i < j) {
               char t= buffer[i];
               buffer[i++]= buffer[j];
               buffer[j--]= t;
            }
         }
         if (
               fwrite(buffer, sizeof(char), (size_t)buffer_size, stdout)
            != (size_t)buffer_size
         ) {
            goto output_error_w_buf;
         }
         if (seekable) {
             if (!fseek(stdin, -(long)buffer_size, SEEK_CUR)) {
                goto seek_error_w_buf;
             }
         } else {
            break;
         }
      }
      if (fflush(0)) {
         output_error_w_buf:
         (void)fputs("Write error!\n", stderr);
         goto fail_w_buf;
      }
      free(buffer);
   }
   return EXIT_SUCCESS;
}
