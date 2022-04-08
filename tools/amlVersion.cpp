#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

typedef void (*amlPrintFunc) ();

static void showUsage();

int main( int argc, char** argv)
{
    int argidx;
    void *module;
    amlPrintFunc amlPrintf = 0;
    char *soFile = NULL;
    const char *printName = "version_print";

    argidx = 1;
    while ( argidx < argc )
    {
        if ( argv[argidx][0] == '-' )
        {
            int len= strlen( argv[argidx] );
            if ( (len == 6) && !strncmp( argv[argidx], "--help", len) )
            {
                showUsage();
                goto exit;
            }
            else if ( (len == 2) && !strncmp( argv[argidx], "-f", len) )
            {
                ++argidx;
                if ( argidx < argc )
                {
                    soFile = argv[argidx];
                }
            }
            else if ( (len == 2) && !strncmp( argv[argidx], "-p", len) )
            {
                ++argidx;
                if ( argidx < argc )
                {
                    printName = argv[argidx];
                }
            }
        }
        else
        {
            printf( "ignoring extra argument: %s\n", argv[argidx] );
        }
        ++argidx;
    }

    if (soFile == NULL) {
        printf("Please set so file relate path\n");
        goto exit;
    }
    printf("path:%s\n",soFile);
    printf("print:%s\n",printName);
    module= dlopen(soFile, RTLD_NOW);
    if (module) {
        amlPrintf = (amlPrintFunc)dlsym( module, printName);
        if (amlPrintf) {
            amlPrintf();
        } else {
            printf("dlsym %s fail,error:%s\n",printName,dlerror());
        }
        dlclose(module);
    } else {
        printf("dlopen %s fail,error:%s\n",soFile,dlerror());
    }
exit:
    return 0;
}

static void showUsage()
{
   printf("usage:\n");
   printf(" westeros [options]\n" );
   printf("where [options] are:\n" );
   printf("  -f <so file path> : so file path\n" );
   printf("  -p <func name> : print version function name\n" );
   printf("  --help : show usage\n" );
   printf("\n" );
}