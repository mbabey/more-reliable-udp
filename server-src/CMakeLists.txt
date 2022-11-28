cmake_minimum_required(VERSION 3.22)

project(reliable-udp
        VERSION 0.0.1
        DESCRIPTION ""
        LANGUAGES C)

set(CMAKE_C_STANDARD 17)

set(SERVER_SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(SERVER_INC_DIR ${PROJECT_SOURCE_DIR}/include)

set(SERVER_SRC_LIST
        ${SERVER_SRC_DIR}/main.c
        ${SERVER_SRC_DIR}/manager.c
        ${SERVER_SRC_DIR}/server.c
        ${SERVER_SRC_DIR}/server-util.c
        ${SERVER_SRC_DIR}/setup.c
        ${SERVER_SRC_DIR}/Game.c # By Prabh Sokhey
        )
set(SERVER_HDR_LIST
        ${SERVER_INC_DIR}/manager.h
        ${SERVER_INC_DIR}/server.h
        ${SERVER_INC_DIR}/server-util.h
        ${SERVER_INC_DIR}/setup.h
        ${SERVER_INC_DIR}/Game.h # By Prabh Sokhey
        )

set(SANITIZE TRUE)

add_compile_definitions(_POSIX_C_SOURCE=200809L)
add_compile_definitions(_XOPEN_SOURCE=700)

if (APPLE)
    add_definitions(-D_DARWIN_C_SOURCE)
endif ()

include_directories(${INCLUDE_DIR})
add_compile_options("-Wall"
        "-Wextra"
        "-Wpedantic"
        "-Wshadow"
        "-Wstrict-overflow=4"
        "-Wswitch-default"
        "-Wswitch-enum"
        "-Wunused"
        "-Wunused-macros"
        "-Wdate-time"
        "-Wimplicit-fallthrough"
        "-Winvalid-pch"
        "-Wmissing-declarations"
        "-Wmissing-include-dirs"
        "-Wmissing-prototypes"
        "-Wstrict-prototypes"
        "-Wundef"
        "-Wnull-dereference"
        "-Wstack-protector"
        "-Wdouble-promotion"
        "-Wvla"
        "-Walloca"
        "-Woverlength-strings"
        "-Wdisabled-optimization"
        "-Winline"
        "-Wcast-qual"
        "-Wfloat-equal"
        "-Wformat=2"
        "-Wfree-nonheap-object"
        "-Wshift-overflow"
        "-Wwrite-strings")

if (${SANITIZE})
    add_compile_options("-fsanitize=address")
    add_compile_options("-fsanitize=undefined")
    add_compile_options("-fsanitize-address-use-after-scope")
    add_compile_options("-fstack-protector-all")
    add_compile_options("-fdelete-null-pointer-checks")
    add_compile_options("-fno-omit-frame-pointer")

    if (NOT APPLE)
        add_compile_options("-fsanitize=leak")
    endif ()

    add_link_options("-fsanitize=address")
    add_link_options("-fsanitize=bounds")
endif ()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    #    add_compile_options("-O2")
    add_compile_options("-Wcast-align"
            "-Wunsuffixed-float-constants"
            "-Warith-conversion"
            "-Wcast-align=strict"
            "-Wunsafe-loop-optimizations"
            "-Wvector-operation-performance"
            "-Walloc-zero"
            "-Wtrampolines"
            "-Wtsan"
            "-Wformat-overflow=2"
            "-Wformat-signedness"
            "-Wjump-misses-init"
            "-Wformat-truncation=2")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
endif ()

find_package(Doxygen
        REQUIRED
        REQUIRED dot
        OPTIONAL_COMPONENTS mscgen dia)

set(DOXYGEN_ALWAYS_DETAILED_SEC YES)
set(DOXYGEN_REPEAT_BRIEF YES)
set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_WARNINGS YES)
set(DOXYGEN_QUIET YES)

doxygen_add_docs(doxygen-server
        ${SERVER_HDR_LIST}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating Doxygen documentation for server")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CLANG_TIDY_CHECKS "*")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-llvmlibc-restrict-system-libc-headers")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-misc-unused-parameters")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-diagnostic-unused-parameter")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-diagnostic-unused-variable")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-cppcoreguidelines-init-variables")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-readability-identifier-length")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-diagnostic-unused-but-set-variable")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-analyzer-deadcode.DeadStores")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-altera-id-dependent-backward-branch")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-cert-dcl03-c")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-hicpp-static-assert")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-misc-static-assert")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-altera-unroll-loops")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-altera-struct-pack-align")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-analyzer-security.insecureAPI.strcpy")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-bugprone-easily-swappable-parameters")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-android-cloexec-open")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling")
set(CLANG_TIDY_CHECKS "${CLANG_TIDY_CHECKS},-android-cloexec-accept")
set(CMAKE_C_CLANG_TIDY clang-tidy -checks=${CLANG_TIDY_CHECKS};--quiet)

add_executable(server ${SERVER_SRC_LIST})
add_dependencies(server doxygen-server)
