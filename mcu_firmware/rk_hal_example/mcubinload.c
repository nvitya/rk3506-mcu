#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Usage: %s <binary file> <phys_addr_hex>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    unsigned long phys_addr = strtoul(argv[2], NULL, 16);

    // Get file size
    struct stat st;
    if(stat(filename, &st) != 0) {
        perror("stat");
        return 1;
    }
    size_t size = st.st_size;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd < 0) { perror("open"); return 1; }

    void *map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phys_addr);
    if(map == MAP_FAILED) { perror("mmap"); return 1; }

    FILE *f = fopen(filename, "rb");
    if(!f) { perror("fopen"); return 1; }

    if(fread(map, 1, size, f) != size) {
        perror("fread");
        fclose(f);
        munmap(map, size);
        close(fd);
        return 1;
    }

    fclose(f);
    munmap(map, size);
    close(fd);
    return 0;
}

