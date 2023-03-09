cmake_minimum_required(VERSION 3.15)

project(sha256sum CXX)

# target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Foundation Net NetSSL Util)
find_package(Poco REQUIRED COMPONENTS Foundation Util)

add_executable(${PROJECT_NAME} main.cxx)

target_link_libraries(${PROJECT_NAME} PRIVATE Poco::Foundation Poco::Util)
