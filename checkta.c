/* Based on code provided by munjeni in:
* http://forum.xda-developers.com/showpost.php?p=70067912&postcount=146
*/

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>

#define TA_SIZE 0x200000
#define TOTAL_BLOCKS 16
#define BLOCK_SIZE 0x20000
#define MAGIC_BYTES 0x3BF8E9C1
#define MOD_ADLER 65521

static int file_exist(char *filename) {
    struct stat st;
    return (stat(filename, &st) == 0) ? 0 : 1;
}

static unsigned int file_size(char *filename) {
    struct stat st;
    unsigned int size;
    stat(filename, &st);
    size = st.st_size;
    return size;
}

unsigned char **allocate2D(int nrows, int ncols) {
    int i;
    unsigned char **dat2;

    dat2 = (unsigned char **)malloc(nrows * sizeof(unsigned char *));

    for (i=0; i<nrows; ++i) {
        dat2[i] = (unsigned char *)malloc(ncols * sizeof(unsigned char));
    }

    if(dat2 == NULL || dat2[i-1] == NULL) {
        printf("Error allocating 2D memory!\n");
        exit(1);
    }

    return dat2;
}

static unsigned int calculate_adler32(unsigned char *data, unsigned int len) {
    unsigned int a = 1;
    unsigned int b = 0;
    unsigned int i;

    for (i=0; i < len; ++i) {
        a = (a + data[i]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }

    return (b << 16) | a;
}

static int check_valid_ta_dump(char *filename) {
    unsigned int filesize;
    unsigned char **blocks;
    unsigned char *tmpbuf;
    FILE *fp = NULL;
    size_t blocks_read = 0;
    int total_blocks = 0;
    int i = 0;

    unsigned int partition_magic;
    unsigned int adler32;
    unsigned int calc_adler32;
    unsigned char nob;
    int data_size;

    if (file_exist(filename)) {
        printf("File \"%s\" does not exist!\n", filename);
        return 0;
    }

    filesize = file_size(filename);
    if (filesize != TA_SIZE) {
        printf("File: %s with size: 0x%x is not a valid dump!\n", filename, filesize);
        return 0;
    }

    blocks = allocate2D(TOTAL_BLOCKS, BLOCK_SIZE);

    fp = fopen(filename, "rb");
    if (NULL == fp) {
        printf("Failed to open %s\n", filename);
        free(blocks);
        return 0;
    } else {
        while (!feof(fp)) {
            blocks_read = fread(blocks[i], 1, BLOCK_SIZE, fp);
            if (blocks_read == BLOCK_SIZE)
                i += 1;
            else
                break;
        }
        fclose(fp);
        total_blocks = i;
    }

    if (total_blocks != 16) {
        printf("Error: found %d blocks in %s instead of 16!\n", total_blocks, filename);
        free(blocks);
        return 0;
    }

    if ((tmpbuf = (unsigned char *)malloc(BLOCK_SIZE * sizeof(unsigned char))) == NULL) {
        printf("Fatal error: can't malloc %d bytes for tmpbuf!\n", BLOCK_SIZE);
        free(blocks);
        return 0;
    }

    for (i=0; i<total_blocks; ++i) {
        memcpy(&partition_magic, blocks[i], 4);
        switch (partition_magic) {
        case MAGIC_BYTES:
            memcpy(&adler32, blocks[i]+4, 4);
            nob = *(unsigned char *)&blocks[i][11];

            if (nob == 0xff)
                data_size = BLOCK_SIZE - 0x8;
            else
                data_size = ((nob + 1) * 0x800) - 0x8;

            memset(tmpbuf, 0, BLOCK_SIZE);
            memcpy(tmpbuf, blocks[i]+8, data_size);
            calc_adler32 = calculate_adler32(tmpbuf, data_size);
            printf("adler32:0x%08X calc_adler32:0x%08X ", adler32, calc_adler32);
            if (adler32 == calc_adler32)
                printf("\n");
            else {
                printf("...nok!\n");
                free(blocks);
                free(tmpbuf);
                return 0;
            }
            break;

        default:
            break;
        }
    }

    free(blocks);
    free(tmpbuf);
    return 1;
}

int main(int argc, char **argv) {
    if(argv < 2) {
        fprintf(stderr, "Usage: %s <ta file.bin>\n", argv[0]);
        return -1;
    }

    if (!check_valid_ta_dump(argv[1])) {
        if (!file_exist(argv[1])) {
            remove(argv[1]);
        }
        fprintf(stderr, "Sorry trim area dump is corupted and not a safe, please try again!\n");
        return -2;
    }
    printf("Trim area dump is valid.\n");
    return 0;
}
