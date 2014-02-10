cmake_minimum_required ( VERSION 2.8 )

project ( lua-llthreads2 C )
include ( cmake/dist.cmake )
include ( lua )

set ( CMAKE_THREAD_PREFER_PTHREAD TRUE )
include ( FindThreads )

include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}/src )

set ( LUA_LLTHREADS_SRC src/l52util.c src/llthread.c )

install_lua_module ( llthreads2 ${LUA_LLTHREADS_SRC} LINK ${CMAKE_THREAD_LIBS_INIT} )

install_data ( COPYRIGHT.llthreads README.md LICENSE )
install_test ( test/ )
