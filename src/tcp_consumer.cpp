#include "common.h"
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using boost::asio::ip::tcp;

int main() {
    pin_thread(2);
    
    boost::asio::io_context io_context;
    tcp::socket socket(io_context);
    tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 9090);

    fmt::print("TCP Consumer connecting to publisher...\n");
    
    // Retry logic
    while (true) {
        boost::system::error_code ec;
        socket.connect(endpoint, ec);
        if (!ec) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    socket.set_option(tcp::no_delay(true));
    boost::asio::streambuf buffer;

    while (true) {
        boost::system::error_code ec;
        // Efficiently read until newline
        size_t n = boost::asio::read_until(socket, buffer, '\n', ec);
        
        if (!ec) {
            std::string s{
                boost::asio::buffers_begin(buffer.data()),
                boost::asio::buffers_begin(buffer.data()) + n
            };
            auto md = json::parse(s);
            fmt::print("[TCP] [{}] {} BID={:.2f} ASK={:.2f}\n", 
                       md["timestamp_ns"].get<uint64_t>(), md["instrument"], md["bid"].get<double>(), md["ask"].get<double>());
            buffer.consume(n); // Clear the buffer for next read
        } else {
            fmt::print("Connection lost.\n");
            break;
        }
    }
    return 0;
}