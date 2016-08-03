static char const *const version_info=
   "$APPLICATION_NAME Version 2016.216\n"
   "\n"
   "Copyright (c) 2016 Guenther Brunthaler. All rights reserved.\n"
   "\n"
   "This program is free software.\n"
   "Distribution is permitted under the terms of the GPLv3.\n"
;

static char const *const help=
   "Usage: $APPLICATION_NAME [ <options> ... [--] ] [ <input_file> ]\n"
   "\n"
   "$APPLICATION_NAME is a filter which reads binary input data from\n"
   "<input_file> (or from its standard input stream if <input_file> is\n"
   "\"-\" or has been omitted as an argument), reverses the order of the\n"
   "bytes read, and writes the reversed output to its standard output\n"
   "stream.\n"
   "\n"
   "$APPLICATION_NAME is similar to the \"rev\"-utility, only that it\n"
   "operates byte-wise and not line-oriented.\n"
   "\n"
   "When provided with a seekable stream as input, $APPLICATION_NAME has no\n"
   "restrictions regarding the maximum stream size and processes the input\n"
   "internally as a series of chunks with fixed maximum size. This means it\n"
   "will not load the complete file into memory unless it is small enough\n"
   "to be processed as a single chunk.\n"
   "\n"
   "When provided with a non-seekable input stream, however,\n"
   "$APPLICATION_NAME will process the whole input stream as a single chunk\n"
   "and load it into memory completely. In this case, the size of the input\n"
   "stream is restricted to the internal buffer size and larger streams\n"
   "will result in an error message. The internal buffer size is 4 MiB by\n"
   "default, but can become as small as 4 KiB if not enough memory is\n"
   "available. Nevertheless, under normal circumstances it will be large\n"
   "enough for processing most typical application document sizes. If in\n"
   "doubt, make sure the input to $APPLICATION_NAME is a seekable file or\n"
   "block device rather than coming from a pipe or fifo.\n"
   "\n"
   "Supported options:\n"
   "\n"
   "-h: Display this help.\n"
   "\n"
   "-V: Display only the copyright and version information.\n"
   "\n"
   "\n"
;


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#ifdef MALLOC_TRACE
   #ifdef NDEBUG
      #undef MALLOC_TRACE
   #else
      #include <mcheck.h>
      #include <malloc.h>
   #endif
#endif


#define MIN_BUFFER_SIZE 4ul * 1024
#define MAX_BUFFER_SIZE 4ul * 1024 * 1024


static char *buffer;


static void cleanup(void) {
   free(buffer); /* ANSI C allows <buffer> to be NULL. */
}

static void die(char const *msg, ...) {
   va_list args;
   va_start(args, msg);
   (void)vfprintf(stderr, msg, args);
   va_end(args);
   (void)fputc('\n', stderr);
   cleanup();
   exit(EXIT_FAILURE);
}

static void stdout_error(void) {
   die("Error writing to standard output stream!");
}

static void ck_write(char const *buf, size_t bytes) {
   if (fwrite(buf, sizeof(char), bytes, stdout) != bytes) stdout_error();
}

static void ck_puts(char const *s) {
   if (fputs(s, stdout) < 0) stdout_error();
}

static void appinfo(const char *text, const char *app) {
   static char const marker[]= "$APPLICATION_NAME";
   int const mlen= (int)(sizeof marker - sizeof(char));
   char const *end;
   while (end= strstr(text, marker)) {
      ck_write(text, (size_t)(end - text));
      ck_puts(app);
      text= end + mlen;
   }
   if (text) ck_puts(text);
}


int main(int argc, char **argv) {
   #ifdef MALLOC_TRACE
       assert(mcheck_pedantic(0) == 0);
       (void)mallopt(M_PERTURB, (int)0xaaaaaaaa);
       mcheck_check_all();
       mtrace();
   #endif
   if (argc > 1) {
      int optind= 1, argpos;
      char *arg;
      process_arg:
      arg= argv[optind];
      for (argpos= 0;; ) {
         int c;
         switch (c= arg[argpos]) {
            case '-':
               if (argpos) goto bad_option;
               switch (arg[++argpos]) {
                  case '-':
                     if (c= arg[argpos + 1]) {
                        die("Unsupported long option %s!", arg);
                     }
                     ++optind; /* "--". */
                     goto end_of_options;
                  case '\0': goto end_of_options; /* "-". */
               }
               /* At first option switch character now. */
               continue;
            case '\0': {
               next_arg:
               if (++optind == argc) goto end_of_options;
               goto process_arg;
            }
         }
         if (!argpos) goto end_of_options;
         switch (c) {
            case 'h': appinfo(help, argv[0]); /* Fall through. */
            case 'V': appinfo(version_info, argv[0]); goto done;
            default: bad_option: die("Unsupported option -%c!", c);
         }
         ++argpos;
      }
      end_of_options:
      if (optind < argc) {
         char const *fname= argv[optind++];
         if (strcmp(fname, "-") && !freopen(fname, "rb", stdin)) {
            die("Could not open file \"%s\" for reading!", fname);
         }
      }
      if (optind != argc) die("Too many arguments!");
   }
   {
      unsigned long buffer_size;
      int seekable;
      for (buffer_size= MAX_BUFFER_SIZE; !(buffer= malloc(buffer_size)); ) {
         if (buffer_size == MIN_BUFFER_SIZE) die("Cannot allocate buffer!");
         assert(buffer_size > MIN_BUFFER_SIZE);
         buffer_size= MIN_BUFFER_SIZE + (buffer_size - MIN_BUFFER_SIZE >> 1);
      }
      seekable= !fseek(stdin, 0, SEEK_END);
      for (;;) {
         assert((long)buffer_size + 1 > (long)buffer_size);
         if (seekable) {
            while (fseek(stdin, -(long)buffer_size, SEEK_CUR)) {
               if (!buffer_size) seek_error: die("Seek error!");
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
               if (ferror(stdin)) read_error: die("Read error!");
               assert(feof(stdin));
               if (seekable) goto seek_error; /* File shrunk??? */
               assert((unsigned long)read == read);
               buffer_size= (unsigned long)read;
            }
         }
         if (!seekable && !feof(stdin)) {
            if (getchar() == EOF) {
               if (ferror(stdin)) goto read_error;
               assert(feof(stdin));
            } else {
               die(
                  "Non-seekable input stream is too large for internal"
                  " buffer! Please provide it as a seekable stream"
                  " instead."
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
         ck_write(buffer, (size_t)buffer_size);
         if (seekable) {
             if (fseek(stdin, -(long)buffer_size, SEEK_CUR)) {
                goto seek_error;
             }
         } else {
            break;
         }
      }
      if (fflush(0)) stdout_error();
   }
   done:
   cleanup();
   return EXIT_SUCCESS;
}
