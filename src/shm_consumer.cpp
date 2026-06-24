#include "common.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    pin_thread(1);
    
    int fd = -1;
    struct stat st;

    fmt::print("Waiting for SHM segment...\n");
    while (true) {
        fd = shm_open("/lmd_shm", O_RDWR, 0666);
        if (fd != -1) {
            fstat(fd, &st);
            if (st.st_size >= (off_t)sizeof(SPSCQueue)) break;
            close(fd);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto* q = (SPSCQueue*)mmap(NULL, sizeof(SPSCQueue), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (q == MAP_FAILED) { perror("mmap"); return 1; }

    fmt::print("SHM Consumer linked and running on CPU 1.\n");
    MarketData md;
    while (true) {
        if (q->try_pop(md)) {
            fmt::print("[SHM] [{}] {} BID={:.2f} ASK={:.2f}\n", 
                       md.timestamp_ns, md.instrument, md.bid, md.ask);
        } 
    }
    return 0;
}