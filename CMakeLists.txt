cmake_minimum_required ( VERSION 2.8 )

project ( lua-llthreads2 C )
include ( cmake/dist.cmake )
include ( lua )

set ( CMAKE_THREAD_PREFER_PTHREAD TRUE )
include ( FindThreads )

include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}/llthreads2/src )

set ( LUA_LLTHREADS_SRC llthreads2/src/l52util.c llthreads2/src/llthread.c )

install_lua_module ( llthreads2 ${LUA_LLTHREADS_SRC} LINK ${CMAKE_THREAD_LIBS_INIT} )

set_target_properties ( llthreads2 PROPERTIES COMPILE_DEFINITIONS "LLTHREAD_MODULE_NAME=llthreads2" )

install_data ( llthreads2/COPYRIGHT.llthreads llthreads2/README.md llthreads2/LICENSE )
install_test ( llthreads2/test/ )
