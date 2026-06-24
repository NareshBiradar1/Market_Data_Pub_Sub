
cmake_minimum_required(VERSION 3.10)
project(LowLatencyMarketData CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -pthread")

find_package(fmt REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json REQUIRED)


include_directories(include)
add_executable(publisher src/publisher.cpp)
add_executable(shm_consumer src/shm_consumer.cpp)
add_executable(tcp_consumer src/tcp_consumer.cpp)

foreach(target publisher shm_consumer tcp_consumer)
    target_link_libraries(${target}
        PRIVATE
            fmt::fmt
            Boost::system
            nlohmann_json::nlohmann_json
            rt
    )
endforeach()