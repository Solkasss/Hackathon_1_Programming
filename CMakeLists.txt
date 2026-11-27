cmake_minimum_required(VERSION 3.26)
project(QueueDesk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    add_compile_options(-Wall -Wextra)
endif()

add_library(queuedesk_core STATIC
    src/DeskState.cpp
    src/LinkedQueue.cpp
    src/Operations.cpp
    src/Stack.cpp
    src/Validation.cpp
    src/Visitor.cpp
)
target_include_directories(queuedesk_core PUBLIC src)

add_executable(queuedesk src/main.cpp)
target_link_libraries(queuedesk PRIVATE queuedesk_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(visitor_test tests/visitor_test.cpp)
    target_link_libraries(visitor_test PRIVATE queuedesk_core)
    add_test(NAME visitor_output COMMAND visitor_test)
    set_tests_properties(visitor_output PROPERTIES LABELS core)
endif()
