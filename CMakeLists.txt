cmake_minimum_required(VERSION 3.22)

project(reliable-udp
        VERSION 0.0.1
        DESCRIPTION ""
        LANGUAGES C)

set(CMAKE_C_STANDARD 17)

# Root source directories.
set(CLIENT_DIR ${PROJECT_SOURCE_DIR}/client-src)
set(SERVER_DIR ${PROJECT_SOURCE_DIR}/server-src)
set(PROXY_DIR ${PROJECT_SOURCE_DIR}/proxy-src)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/libs)

# Client:
set(CLIENT_SRC_DIR ${CLIENT_DIR}/src)
set(CLIENT_INC_DIR ${CLIENT_DIR}/include)

set(CLIENT_SRC_LIST
        ${CLIENT_SRC_DIR}/main.c
        ${CLIENT_SRC_DIR}/client.c
        ${CLIENT_SRC_DIR}/setup.c
        )
set(CLIENT_HDR_LIST
        ${CLIENT_INC_DIR}/client.h
        ${CLIENT_INC_DIR}/setup.h
        )
# End Client.

# Server:
set(SERVER_SRC_DIR ${SERVER_DIR}/src)
set(SERVER_INC_DIR ${SERVER_DIR}/include)

set(SERVER_SRC_LIST
        ${SERVER_SRC_DIR}/main.c
        ${SERVER_SRC_DIR}/server.c
        ${SERVER_SRC_DIR}/setup.c
        )
set(SERVER_HDR_LIST
        ${SERVER_INC_DIR}/server.h
        ${SERVER_INC_DIR}/setup.h
        )
# End Server.

# Proxy:
set(PROXY_SRC_DIR ${PROXY_DIR}/src)
set(PROXY_INC_DIR ${PROXY_DIR}/include)

set(PROXY_SRC_LIST
        ${PROXY_SRC_DIR}/main.c
        ${PROXY_SRC_DIR}/setup.c
        ${PROXY_SRC_DIR}/proxy.c
        )
set(PROXY_HDR_LIST
        ${PROXY_INC_DIR}/setup.h
        ${PROXY_INC_DIR}/proxy.h
        )
# End Proxy.

# Shared:
set(LIB_SRC_DIR ${LIB_DIR}/src)
set(LIB_INC_DIR ${LIB_DIR}/include)

set(LIB_SRC_LIST
        ${LIB_SRC_DIR}/error.c
        ${LIB_SRC_DIR}/input-validation.c
        ${LIB_SRC_DIR}/manager.c
        ${LIB_SRC_DIR}/util.c
        )
set(LIB_HDR_LIST
        ${LIB_INC_DIR}/error.h
        ${LIB_INC_DIR}/input-validation.h
        ${LIB_INC_DIR}/manager.h
        ${LIB_INC_DIR}/util.h
        ${LIB_INC_DIR}/structs.h
        )
# End Shared.

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

doxygen_add_docs(doxygen-client
        ${CLIENT_HDR_LIST}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating Doxygen documentation for client")
doxygen_add_docs(doxygen-server
        ${SERVER_HDR_LIST}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating Doxygen documentation for server")
doxygen_add_docs(doxygen-proxy
        ${PROXY_HDR_LIST}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating Doxygen documentation for proxy")

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

add_executable(client ${CLIENT_SRC_LIST} ${LIB_SRC_LIST})
add_executable(server ${SERVER_SRC_LIST} ${LIB_SRC_LIST})
add_executable(proxy ${PROXY_SRC_LIST} ${LIB_SRC_LIST})
add_dependencies(client doxygen-client)
add_dependencies(server doxygen-server)
add_dependencies(proxy doxygen-proxy)
