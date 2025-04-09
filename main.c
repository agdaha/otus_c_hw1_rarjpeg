#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


// from https://www.hanshq.net/files/hwzip/zip.c
/* End of Central Directory Record. */
typedef struct
{
    uint16_t disk_nbr;        /* Number of this disk. */
    uint16_t cd_start_disk;   /* Nbr. of disk with start of the CD. */
    uint16_t disk_cd_entries; /* Nbr. of CD entries on this disk. */
    uint16_t cd_entries;      /* Nbr. of Central Directory entries. */
    uint32_t cd_size;         /* Central Directory size in bytes. */
    uint32_t cd_offset;       /* Central Directory file offset. */
    uint16_t comment_len;     /* Archive comment length. */
    const uint8_t *comment;   /* Archive comment. */
} eocdr_t;

/* Size of the End of Central Directory Record, not including comment. */
#define EOCDR_BASE_SZ 22
#define EOCDR_SIGNATURE 0x06054b50 /* "PK\5\6" little-endian. */
#define MAX_BACK_OFFSET (1024 * 100)

/* Central File Header (Central Directory Entry) */
typedef struct
{
    uint16_t made_by_ver;    /* Version made by. */
    uint16_t extract_ver;    /* Version needed to extract. */
    uint16_t gp_flag;        /* General-purpose bit flag. */
    uint16_t method;         /* Compression method. */
    uint16_t mod_time;       /* Modification time. */
    uint16_t mod_date;       /* Modification date. */
    uint32_t crc32;          /* CRC-32 checksum. */
    uint32_t comp_size;      /* Compressed size. */
    uint32_t uncomp_size;    /* Uncompressed size. */
    uint16_t name_len;       /* Filename length. */
    uint16_t extra_len;      /* Extra data length. */
    uint16_t comment_len;    /* Comment length. */
    uint16_t disk_nbr_start; /* Disk nbr. where file begins. */
    uint16_t int_attrs;      /* Internal file attributes. */
    uint32_t ext_attrs;      /* External file attributes. */
    uint32_t lfh_offset;     /* Local File Header offset. */
    const uint8_t *name;     /* Filename. */
    const uint8_t *extra;    /* Extra data. */
    const uint8_t *comment;  /* File comment. */
} cfh_t;

/* Size of a Central File Header, not including name, extra, and comment. */
#define CFH_BASE_SZ 42
#define CFH_SIGNATURE 0x02014b50 /* "PK\1\2" little-endian. */


int find_eocdr(FILE *file, eocdr_t *eocdr)
{
    fseek(file, 0, SEEK_END);

    long file_size = ftell(file);
    size_t back_offset;
    uint32_t signature;

    for (back_offset = 0; back_offset <= MAX_BACK_OFFSET; back_offset++)
    {
        if ((long unsigned int)file_size <  EOCDR_BASE_SZ + back_offset)
        {
            break;
        }

        long search_start = file_size - EOCDR_BASE_SZ - back_offset;
        if (search_start < 0)
            search_start = 0;

        fseek(file, search_start, SEEK_SET);
        fread(&signature, 1, sizeof(signature), file);

        if (signature == EOCDR_SIGNATURE)
        {
            fread(eocdr, 1, sizeof(*eocdr), file);
            eocdr->cd_offset = search_start - eocdr->cd_size;
            return 1;
        }
    }
    return 0;
}

void print_cfhs(FILE *file, uint16_t entries){
    uint32_t signature;
    printf("Cодержимое архива: \n");
    for (int i = 0; i < entries; i++) {
        cfh_t file_header;

        fread(&signature, sizeof(signature), 1, file);

        if (signature !=CFH_SIGNATURE ) {
            printf("Ошибка: неверная сигнатура файла!\n");
            break;
        }

        fread(&file_header, CFH_BASE_SZ, 1, file);
          
        char filename[256];
        fread(filename, 1, file_header.name_len, file);
        filename[file_header.name_len] = '\0';

        printf("%d) (%s) %s (%u байт)\n", 
            i + 1, 
            filename[file_header.name_len-1] == '/' ? "папка" : "файл",
            filename, 
            file_header.uncomp_size
        );

        fseek(file, file_header.extra_len + file_header.comment_len, SEEK_CUR);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("USAGE: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file)
    {
        printf("Failed to open %s for reading!\n", argv[1]);
        return EXIT_FAILURE;
    }

    eocdr_t eocdr;
    if (!find_eocdr(file, &eocdr))
    {
        printf("Не содержит ахив!\n");
    }
    else
    {
        if (eocdr.cd_entries==0) {
            printf("Пустой архив\n");
            return EXIT_SUCCESS;
        }

        fseek(file, eocdr.cd_offset, SEEK_SET);
        print_cfhs(file, eocdr.cd_entries);

    }

    fclose(file);
    return EXIT_SUCCESS;
}
