include(CheckTypeSize)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckLibraryExists)

cmake_minimum_required(VERSION 2.6)

project(minikonoha)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "3")
set(VERSION_PATCH "0")

SET(CFLAGS "-Wsign-compare -Wformat")
SET(CFLAGS "${CFLAGS} -Wall -Wcomment -Wunused-macros")
SET(CMAKE_C_FLAGS   "${CFLAGS} -Wno-long-long")
SET(CMAKE_CXX_FLAGS "${CFLAGS}")
if(MSVC)
	SET(CMAKE_C_FLAGS_RELEASE      "/D NDEBUG /W3 /Wall /TP /MT /Ox /Ob2 /GL")
	SET(CMAKE_C_FLAGS_DEBUG        "/D_DEBUG /W3 /Wall /TP /MTd /Zi /Ob0 /Od /RTC1")
	SET(CMAKE_C_FLAGS_COMPRESSED   "/D NDEBUG /W3 /Wall /TP /MT /Ox /GL")
else(MSVC)
	SET(CMAKE_C_FLAGS_RELEASE      "${CMAKE_C_FLAGS} -O3 -g")
	SET(CMAKE_C_FLAGS_DEBUG        "${CMAKE_C_FLAGS} -O0 -g3")
	SET(CMAKE_C_FLAGS_COMPRESSED   "${CMAKE_C_FLAGS} -Os -g0")
endif(MSVC)
SET(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS} -O3 -g")
SET(CMAKE_CXX_FLAGS_DEBUG      "${CMAKE_CXX_FLAGS} -O0 -g3")
SET(CMAKE_CXX_FLAGS_COMPRESSED "${CMAKE_CXX_FLAGS} -Os -g0")

## Source Code settings
set(RUNTIME
	src/konoha/minikonoha.c
)

set(PARSER
	src/parser/parser.c
)

set(PACKAGE_NAME    ${PROJECT_NAME})
set(PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(PACKAGE_STRING  ${CMAKE_PROJECT_NAME}-${PACKAGE_VERSION})
set(SO_VERSION      ${VERSION_MAJOR}.${VERSION_MINOR})
set(PACKAGE_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(K_CONFIG_ARCH ${CMAKE_SYSTEM})
set(K_CONFIG_OS   ${CMAKE_SYSTEM_NAME})
set(K_CONFIG_CPU  ${CMAKE_SYSTEM_PROCESSOR})
set(K_PREFIX ${CMAKE_INSTALL_PREFIX})

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR})
if(${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
	set(INCLUDE_DIRS ${INCLUDE_DIRS} /usr/pkg/include)
endif(${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
include_directories(${INCLUDE_DIRS})

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} uppercase_CMAKE_BUILD_TYPE)

if(${CMAKE_BUILD_TYPE} STREQUAL "Compressed")
	add_definitions(-DNDEBUG)
	add_definitions(-DUSE_SMALLBUILD)
endif()

if(UNIX OR CYGWIN)
	set(K_OSDLLEXT ".so")
endif()

if(APPLE)
	set(K_OSDLLEXT ".dylib")
endif(APPLE)

if(MSVC OR MINGW)
	set(K_OSDLLEXT ".dll")
endif()

find_library(HAVE_LIBDL    NAMES dl)
find_library(HAVE_LIBICONV NAMES iconv)
find_library(HAVE_PTHREAD  NAMES pthread)
find_library(HAVE_LIBDB    NAMES db)

check_type_size("void *" SIZEOF_VOIDP)
check_type_size(long     SIZEOF_LONG)
check_type_size(int      SIZEOF_INT)
check_type_size(float    SIZEOF_FLOAT)
check_type_size(double   SIZEOF_DOUBLE)

SET(CMAKE_EXTRA_INCLUDE_FILES pthread.h)
CHECK_TYPE_SIZE(pthread_mutex_t SIZEOF_PTHREAD_MUTEX_T)
SET(CMAKE_EXTRA_INCLUDE_FILES)

check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(iconv.h HAVE_ICONV_H)
check_include_files(syslog.h HAVE_SYSLOG_H)
check_include_files(stdbool.h HAVE_STDBOOL_H)
check_include_files(db.h      HAVE_DB_H)

set(_HEADERS stdio.h stdlib.h string.h ctype.h)
if(UNIX)
	set(_HEADERS ${_HEADERS} sys/mman.h unistd.h)
endif(UNIX)

if(APPLE)
	set(_HEADERS ${_HEADERS} mach-o/dyld.h)
endif(APPLE)

check_symbol_exists(posix_memalign "${_HEADERS}" HAVE_POSIX_MEMALIGN)
check_symbol_exists(memalign       "${_HEADERS}" HAVE_MEMALIGN)
check_symbol_exists(__builtin_ctzl "${_HEADERS}" HAVE_BUILTIN_CTZL)
check_symbol_exists(bzero "${_HEADERS}" HAVE_BZERO)

FIND_FILE(HAS_DOTGIT .git PATHS "${CMAKE_CURRENT_SOURCE_DIR}")
if("${HAS_DOTGIT}" MATCHES ".git")
	find_package(Git)
	if(GIT_EXECUTABLE)
		execute_process(
			COMMAND ${GIT_EXECUTABLE} rev-parse -q --short=8 HEAD
			OUTPUT_VARIABLE GIT_SHA1
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	set(K_REVISION "${GIT_SHA1}")
	set(K_REVISION_PREFIX "git:")
	endif(GIT_EXECUTABLE)
else()
	set(K_REVISION "${PACKAGE_VERSION}")
	set(K_REVISION_PREFIX "release:")
endif()

add_definitions(-DHAVE_CONFIG_H)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
		${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(ExtraLibs)
set(ExtraLibs ${ExtraLibs} ${HAVE_PTHREAD})
if(HAVE_LIBDL)
	set(ExtraLibs ${ExtraLibs} ${HAVE_LIBDL})
endif(HAVE_LIBDL)

if(HAVE_LIBICONV)
	set(ExtraLibs ${ExtraLibs} ${HAVE_LIBICONV})
endif(HAVE_LIBICONV)


macro(POP _STR _DELIM _RESULT)
	string(REPLACE ${_DELIM} ";" _tmp ${_STR})
	list(REVERSE _tmp)
	list(GET _tmp 0 ${_RESULT})
endmacro()

# for jenkins coverage test
option(TEST_COVERAGE "Coverage Test" OFF)
if(TEST_COVERAGE)
	include(CTest)
	message(STATUS "TEST_COVERAGE = ${TEST_COVERAGE}")
	add_definitions(-fprofile-arcs -ftest-coverage)
	set(ExtraLibs ${ExtraLibs} -fprofile-arcs -ftest-coverage)
endif(TEST_COVERAGE)

# for .k script test
enable_testing()
set(TEST_BIN ${CMAKE_CURRENT_BINARY_DIR}/minikonoha)
set(PARENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PARENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

macro(add_test_with_property test_name)
	add_test(${test_name} ${TEST_BIN} ${ARGN})
	set_tests_properties(${test_name} PROPERTIES TIMEOUT 5)
	set_tests_properties(${test_name} PROPERTIES ENVIRONMENT
		"KONOHA_HOME=${PARENT_BINARY_DIR}")
endmacro(add_test_with_property test_name)

option(TEST_STRESS "Use Stress Test" OFF)
macro(add_konoha_test test_name test_file)
	if(test_name MATCHES "Stress")
		if(TEST_STRESS)
			add_test_with_property(${test_name} -MOutputTest ${test_file})
			set_tests_properties(${test_name} PROPERTIES TIMEOUT 10)
		endif(TEST_STRESS)
	else(test_name MATCHES "Stress")
		if(NOT test_name MATCHES "^Shell")
			add_test_with_property(${test_name} -MOutputTest -DBUFSIZ=1024 -DPASSWD=XXX ${test_file})
		endif(NOT test_name MATCHES "^Shell")
	endif(test_name MATCHES "Stress")
endmacro(add_konoha_test test_name test_file)

file(GLOB_RECURSE test_files ${CMAKE_CURRENT_SOURCE_DIR}/test/*.k)
foreach(f ${test_files})
	string(REPLACE // / f ${f})
	string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/test/ "" test_name ${f})
	add_konoha_test(${test_name} ${f})
endforeach(f ${test_files})

macro(SET_CXX_PROPERTY_IF_MSVC TARGET)
	if(MSVC)
		foreach(arg ${ARGN})
			set_source_files_properties(${arg} PROPERTIES LANGUAGE CXX )
		endforeach(arg ${ARGN})
		set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX )
	endif(MSVC)
endmacro(SET_CXX_PROPERTY_IF_MSVC TARGET)

add_library(minikonoha SHARED ${RUNTIME} ${PARSER} ${VM})
SET_CXX_PROPERTY_IF_MSVC(minikonoha ${RUNTIME} ${PARSER} ${VM})
target_link_libraries(minikonoha ${ExtraLibs})
set(ExtraLibs ${ExtraLibs} minikonoha)

add_executable(minikonoha_bin src/exec/command/command.c)
SET_CXX_PROPERTY_IF_MSVC(minikonoha_bin src/exec/command/command.c)
target_link_libraries(minikonoha_bin ${ExtraLibs})
set_target_properties(minikonoha_bin PROPERTIES OUTPUT_NAME minikonoha)

macro(ENABLE_COVERAGE_INFO_IF_ENABLE VARNAME)
	if(TEST_COVERAGE)
		add_definitions(-fprofile-arcs -ftest-coverage)
		set(${VARNAME} ${${VARNAME}} -fprofile-arcs -ftest-coverage)
	endif(TEST_COVERAGE)
endmacro(ENABLE_COVERAGE_INFO_IF_ENABLE VARNAME)

macro(create_dir DIR_NAME)
	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${DIR_NAME}
		COMMAND ${CMAKE_COMMAND} -E make_directory
		${CMAKE_CURRENT_BINARY_DIR}/${DIR_NAME})
	add_custom_target(make_${DIR_NAME}_dir ALL DEPENDS
		${CMAKE_CURRENT_BINARY_DIR}/${DIR_NAME})
endmacro(create_dir DIR_NAME)

create_dir(module)
create_dir(package)

add_subdirectory(src/module)
add_subdirectory(src/module-devel)

add_subdirectory(src/package)
add_subdirectory(src/package-devel)

macro(add_clib_test test_name)
	add_executable(${test_name} ${ARGN})
	SET_CXX_PROPERTY_IF_MSVC(${test_name} ${ARGN})
	target_link_libraries(${test_name} ${ExtraLibs})
	add_test(Clib/Release/${test_name} ${test_name})
	set_tests_properties(Clib/Release/${test_name} PROPERTIES TIMEOUT 5)
endmacro(add_clib_test sources)

add_clib_test(test_sizeof  ./test/Clib/test_sizeof.c)
add_clib_test(test_Kmalloc ./test/Clib/test_Kmalloc.c)
add_clib_test(test_Karray  ./test/Clib/test_Karray.c)
add_clib_test(test_gc      ./test/Clib/test_gc.c)
add_clib_test(test_Kwb     ./test/Clib/test_Kwb.c)
add_clib_test(test_map     ./test/Clib/test_map.c)
add_clib_test(test_string  ./test/Clib/test_string.c)
add_clib_test(test_cpp     ./test/Clib/test_cpp.cpp)
add_clib_test(test_proto   ./test/Clib/test_proto.c)
add_clib_test(MultiContext   ./test/Clib/MultiContext.c)

macro(set_package_path test_name)
set_tests_properties(${test_name} PROPERTIES ENVIRONMENT
	"KONOHA_HOME=${PARENT_BINARY_DIR}")
endmacro(set_package_path test_name)

add_test(Shell/Release/Oneline
	${CMAKE_CURRENT_SOURCE_DIR}/test/test-oneline.sh ${TEST_BIN}
	"System.p(\"hi\");")
set_package_path(Shell/Release/Oneline)

add_test(Shell/Release/Multiline
	${CMAKE_CURRENT_SOURCE_DIR}/test/test-multiline.sh ${TEST_BIN}
	${CMAKE_CURRENT_SOURCE_DIR}/test/Release/Fibonacci.k)
set_package_path(Shell/Release/Multiline)

file(GLOB_RECURSE test_files ${CMAKE_CURRENT_SOURCE_DIR}/test/Shell/*.sh)
foreach(f ${test_files})
	string(REPLACE // / f ${f})
	string(REPLACE .sh .k TEST_SCRIPT ${f})
	string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/test/ "" test_name ${TEST_SCRIPT})
	if(EXISTS ${TEST_SCRIPT})
		add_test(${test_name} sh +x ${f} ${TEST_BIN} ${TEST_SCRIPT})
	else(EXISTS ${TEST_SCRIPT})
		add_test(${test_name} sh +x ${f} ${TEST_BIN})
	endif(EXISTS ${TEST_SCRIPT})
	set_tests_properties(${test_name} PROPERTIES TIMEOUT 5)
	set_tests_properties(${test_name} PROPERTIES ENVIRONMENT
		"KONOHA_HOME=${PARENT_BINARY_DIR}")
endforeach(f ${test_files})

add_subdirectory(src/exec/apache)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack.cmake)

# for konoha command
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/exec/command/konoha.sh.cmake
		${CMAKE_CURRENT_BINARY_DIR}/konoha.sh)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/konoha.sh
	DESTINATION bin
	RENAME konoha
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(TARGETS minikonoha minikonoha_bin
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib${LIB_SUFFIX}
		)

install(DIRECTORY include/ DESTINATION include 
		FILES_MATCHING
		PATTERN "*.h"
		PATTERN ".git" EXCLUDE
		)

MESSAGE(STATUS)
MESSAGE(STATUS "---------------------------------------------------------------------------" )
MESSAGE(STATUS "CMAKE_BUILD_TYPE = ${uppercase_CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
MESSAGE(STATUS "CMAKE_C_FLAGS   = ${CMAKE_C_FLAGS_${uppercase_CMAKE_BUILD_TYPE}}")
MESSAGE(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS_${uppercase_CMAKE_BUILD_TYPE}}")
MESSAGE(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "Change a value with: cmake -D<Variable>=<Value>" )
MESSAGE(STATUS "---------------------------------------------------------------------------" )
MESSAGE(STATUS)
IF(WARNINGS)
	SET(MSG_STATUS STATUS)
	FOREACH(WARN ${WARNINGS})
		MESSAGE(${MSG_STATUS} "[WARNING] " ${WARN})
	ENDFOREACH(WARN ${WARNINGS})
	MESSAGE(STATUS)
ENDIF(WARNINGS)
