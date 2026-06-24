#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <random>
#include "common.h"
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main() {
    pin_thread(0);
    TSCClock clock;
    std::vector<const char*> symbols = {"Meta", "Google", "Tesla"};
    std::mt19937 rng(1337);

    // 1. SHM Setup
    shm_unlink("/lmd_shm");
    int shm_fd = shm_open("/lmd_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SPSCQueue));
    auto* q = (SPSCQueue*)mmap(NULL, sizeof(SPSCQueue), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    new (q) SPSCQueue();
    q->init();

    // 2. Boost.Asio TCP Setup
    boost::asio::io_context io_context;
    tcp::endpoint endpoint(tcp::v4(), 9090);
    tcp::acceptor acceptor(io_context);

    acceptor.open(endpoint.protocol());

    acceptor.set_option(tcp::acceptor::reuse_address(true));

    acceptor.bind(endpoint);
    acceptor.listen();
    fmt::print("Publisher (CPU 0). Waiting for Boost::Asio Consumer...\n");
    tcp::socket socket(io_context);
    acceptor.accept(socket);
    
    // Disable Nagle's algorithm for low latency
    socket.set_option(tcp::no_delay(true));

    char json_buf[256];
    while (true) {
        MarketData md;
        snprintf(md.instrument, sizeof(md.instrument), "%s", symbols[rng() % 3]);
        md.bid = 2500.0 + (rng() % 100);
        md.ask = md.bid + 0.50;
        md.timestamp_ns = clock.now_ns();

        // Path A: SHM
        q->push(md); 

        // Path B: TCP via Boost.Asio
        size_t len = md.to_json(json_buf, sizeof(json_buf));
        boost::system::error_code ec;
        boost::asio::write(socket, boost::asio::buffer(json_buf, len), ec);

        if (ec) {
            fmt::print("TCP Client disconnected.\n");
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}