cmake_minimum_required(VERSION 3.13)

project(idash)

# add_executable(model4 model4.cpp)
add_executable(continuous model_conti.cpp)
add_executable(binary model_bin.cpp)

find_package(SEAL 4.0 REQUIRED)
# target_link_libraries(model4 SEAL::seal)
target_link_libraries(continuous SEAL::seal)
target_link_libraries(binary SEAL::seal)