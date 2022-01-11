#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>

static long long getCurrentTimeMillis(void);

static int g_activeLevel= 6;
static FILE * g_fd = stderr;

void Logger_set_level(int setLevel)
{
    if (setLevel <=0) {
        g_activeLevel = 0;
    } else if (setLevel > 6) {
        g_activeLevel = 6;
    } else {
        g_activeLevel = setLevel;
    }
}

int Logger_get_level()
{
    return g_activeLevel;
}

void Logger_set_file(char *filepath)
{
    FILE * logFd;
    if (!filepath) {
        if (g_fd != stderr) {
            fclose(g_fd);
            g_fd = stderr;
        }
        return;
    } else { //close pre logfile
        if (g_fd != stderr) {
            fclose(g_fd);
            g_fd = stderr;
        }
    }

    logFd = fopen(filepath, "w");
    if (logFd == NULL) {
        return;
    }
    g_fd = logFd;
}

void logPrint( int level, const char *fmt, ... )
{
   if ( level <= g_activeLevel )
   {
      va_list argptr;
      fprintf( g_fd, "%lld: ", getCurrentTimeMillis());
      va_start( argptr, fmt );
      vfprintf( g_fd, fmt, argptr );
      va_end( argptr );
   }
}

static long long getCurrentTimeMillis(void)
{
#if 0
   struct timeval tv;
   long long utcCurrentTimeMillis;

   gettimeofday(&tv,0);
   utcCurrentTimeMillis= tv.tv_sec*1000LL+(tv.tv_usec/1000LL);

   return utcCurrentTimeMillis;
#else
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID,
            CLOCK_BOOTTIME
    };
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[1], &t);
    int64_t mono_ns = int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
    return mono_ns/1000LL;
#endif
}