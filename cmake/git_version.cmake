# Get version using git describe
find_package(Git)

execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
        RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (GIT_DESCRIBE_ERROR_CODE)
    # It might happen that no tag is available. Most CI systems only checkout the git history with a certain depth
    # (ie. 50 commits). If the last tag is older than that, no tag is available and git describe will fail.
    message(FATAL_ERROR "Git describe returned an error: ${GIT_DESCRIBE_VERSION}")
endif()

# Extract version components from git tags starting with 'v'
string(REGEX MATCHALL "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)$" VERSION_MATCH ${GIT_DESCRIBE_VERSION})

set(GIT_VERSION_MAJOR ${CMAKE_MATCH_1})
set(GIT_VERSION_MINOR ${CMAKE_MATCH_2})
set(GIT_VERSION_PATCH ${CMAKE_MATCH_3})

if (${GIT_VERSION_MAJOR} STREQUAL "" OR ${GIT_VERSION_MINOR} STREQUAL "" OR ${GIT_VERSION_PATCH} STREQUAL "")
    message(FATAL_ERROR "No version info available")
endif()

message(STATUS "Git describe: ${GIT_DESCRIBE_VERSION}. Version major: ${GIT_VERSION_MAJOR}, version minor: ${GIT_VERSION_MINOR}, version patch: ${GIT_VERSION_PATCH}")
