#if !defined(_WIN32) && !defined(USE_PTHREAD)
#  define USE_PTHREAD
#endif

#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <errno.h>

#ifndef USE_PTHREAD
#  include <windows.h>
#  include <stdio.h>
#  include <process.h>
#else
#  include <pthread.h>
#  include <stdio.h>
#endif

/*export*/
#ifdef _WIN32
#  define LLTHREADS_EXPORT_API __declspec(dllexport)
#else
#  define LLTHREADS_EXPORT_API LUALIB_API
#endif

/* wrap strerror_s(). */
#ifdef _WIN32
#  ifdef __GNUC__
#    ifndef strerror_r
#      define strerror_r(errno, buf, buflen) do { \
         strncpy((buf), strerror(errno), (buflen)-1); \
         (buf)[(buflen)-1] = '\0'; \
       } while(0)
#    endif
#  else
#    ifndef strerror_r
#      define strerror_r(errno, buf, buflen) strerror_s((buf), (buflen), (errno))
#    endif
#  endif
#endif

#ifndef USE_PTHREAD
#  define OS_THREAD_RETURT      unsigned int __stdcall
#  define INVALID_THREAD        INVALID_HANDLE_VALUE
#  define INFINITE_JOIN_TIMEOUT INFINITE
#  define JOIN_OK               0
#  define JOIN_ETIMEDOUT        1
typedef DWORD  join_timeout_t;
typedef HANDLE os_thread_t;
#else
#  define OS_THREAD_RETURT      void *
#  define INVALID_THREAD        0
#  define INFINITE_JOIN_TIMEOUT -1
#  define JOIN_OK               0
#  define JOIN_ETIMEDOUT        ETIMEDOUT
typedef int       join_timeout_t;
typedef pthread_t os_thread_t;
#endif

#include "l52util.h"
#include <lualib.h>

LLTHREADS_EXPORT_API int luaopen_llthreads(lua_State *L);

//{ traceback

#define ERROR_LEN 1024

/******************************************************************************
* traceback() function from Lua 5.1/5.2 source.
* Copyright (C) 1994-2008 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
#if !defined(LUA_VERSION_NUM) || (LUA_VERSION_NUM == 501)
/* from Lua 5.1 */
static int traceback (lua_State *L) {
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  lua_getglobal(L, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}
#else
/* from Lua 5.2 */
static int traceback (lua_State *L) {
  const char *msg = lua_tostring(L, 1);
  if (msg)
    luaL_traceback(L, L, msg, 1);
  else if (!lua_isnoneornil(L, 1)) {  /* is there an error object? */
    if (!luaL_callmeta(L, 1, "__tostring"))  /* try its 'tostring' metamethod */
      lua_pushliteral(L, "(no error message)");
  }
  return 1;
}
#endif

//}

//{ copy values

/* maximum recursive depth of table copies. */
#define MAX_COPY_DEPTH 30

typedef struct {
  lua_State *from_L;
  lua_State *to_L;
  int has_cache;
  int cache_idx;
  int is_arg;
} llthread_copy_state;

static int llthread_copy_table_from_cache(llthread_copy_state *state, int idx) {
  void *ptr;

  /* convert table to pointer for lookup in cache. */
  ptr = (void *)lua_topointer(state->from_L, idx);
  if(ptr == NULL) return 0; /* can't convert to pointer. */

  /* check if we need to create the cache. */
  if(!state->has_cache) {
    lua_newtable(state->to_L);
    lua_replace(state->to_L, state->cache_idx);
    state->has_cache = 1;
  }

  lua_pushlightuserdata(state->to_L, ptr);
  lua_rawget(state->to_L, state->cache_idx);
  if(lua_isnil(state->to_L, -1)) {
    /* not in cache. */
    lua_pop(state->to_L, 1);
    /* create new table and add to cache. */
    lua_newtable(state->to_L);
    lua_pushlightuserdata(state->to_L, ptr);
    lua_pushvalue(state->to_L, -2);
    lua_rawset(state->to_L, state->cache_idx);
    return 0;
  }
  /* found table in cache. */
  return 1;
}

static int llthread_copy_value(llthread_copy_state *state, int depth, int idx) {
  const char *str;
  size_t str_len;
  int kv_pos;

  /* Maximum recursive depth */
  if(++depth > MAX_COPY_DEPTH) {
    return luaL_error(state->from_L, "Hit maximum copy depth (%d > %d).", depth, MAX_COPY_DEPTH);
  }

  /* only support string/number/boolean/nil/table/lightuserdata. */
  switch(lua_type(state->from_L, idx)) {
  case LUA_TNIL:
    lua_pushnil(state->to_L);
    break;
  case LUA_TNUMBER:
    lua_pushnumber(state->to_L, lua_tonumber(state->from_L, idx));
    break;
  case LUA_TBOOLEAN:
    lua_pushboolean(state->to_L, lua_toboolean(state->from_L, idx));
    break;
  case LUA_TSTRING:
    str = lua_tolstring(state->from_L, idx, &(str_len));
    lua_pushlstring(state->to_L, str, str_len);
    break;
  case LUA_TLIGHTUSERDATA:
    lua_pushlightuserdata(state->to_L, lua_touserdata(state->from_L, idx));
    break;
  case LUA_TTABLE:
    /* make sure there is room on the new state for 3 values (table,key,value) */
    if(!lua_checkstack(state->to_L, 3)) {
      return luaL_error(state->from_L, "To stack overflow!");
    }
    /* make room on from stack for key/value pairs. */
    luaL_checkstack(state->from_L, 2, "From stack overflow!");

    /* check cache for table. */
    if(llthread_copy_table_from_cache(state, idx)) {
      /* found in cache don't need to copy table. */
      break;
    }
    lua_pushnil(state->from_L);
    while (lua_next(state->from_L, idx) != 0) {
      /* key is at (top - 1), value at (top), but we need to normalize these
       * to positive indices */
      kv_pos = lua_gettop(state->from_L);
      /* copy key */
      llthread_copy_value(state, depth, kv_pos - 1);
      /* copy value */
      llthread_copy_value(state, depth, kv_pos);
      /* Copied key and value are now at -2 and -1 in state->to_L. */
      lua_settable(state->to_L, -3);
      /* Pop value for next iteration */
      lua_pop(state->from_L, 1);
    }
    break;
  case LUA_TFUNCTION:
  case LUA_TUSERDATA:
  case LUA_TTHREAD:
  default:
    if (state->is_arg) {
      return luaL_argerror(state->from_L, idx, "function/userdata/thread types un-supported.");
    } else {
      /* convert un-supported types to an error string. */
      lua_pushfstring(state->to_L, "Un-supported value: %s: %p",
        lua_typename(state->from_L, lua_type(state->from_L, idx)), lua_topointer(state->from_L, idx));
    }
  }

  return 1;
}

static int llthread_copy_values(lua_State *from_L, lua_State *to_L, int idx, int top, int is_arg) {
  llthread_copy_state state;
  int nvalues = 0;
  int n;

  nvalues = (top - idx) + 1;
  /* make sure there is room on the new state for the values. */
  if(!lua_checkstack(to_L, nvalues + 1)) {
    return luaL_error(from_L, "To stack overflow!");
  }

  /* setup copy state. */
  state.from_L = from_L;
  state.to_L = to_L;
  state.is_arg = is_arg;
  state.has_cache = 0; /* don't create cache table unless it is needed. */
  lua_pushnil(to_L);
  state.cache_idx = lua_gettop(to_L);

  nvalues = 0;
  for(n = idx; n <= top; n++) {
    llthread_copy_value(&state, 0, n);
    ++nvalues;
  }

  /* remove cache table. */
  lua_remove(to_L, state.cache_idx);

  return nvalues;
}

//}

static int fail(lua_State *L, const char *msg){
  lua_pushnil(L);
  lua_pushstring(L, msg);
  return 2;
}

#define flags_t unsigned char

#define TSTATE_NONE     (flags_t)0
#define TSTATE_STARTED  (flags_t)1<<0
#define TSTATE_DETACHED (flags_t)1<<1
#define TSTATE_JOINED   (flags_t)1<<2

/*At leas one flag*/
#define FLAG_IS_SET(O, F) (O->flags & (flags_t)(F))
/*All flags*/
#define FLAGS_IS_SET(O, F) ((F) == FLAG_IS_SET(O, F))
#define FLAG_SET(O, F) O->flags |= (flags_t)(F)
#define FLAG_UNSET(O, F) O->flags &= ~((flags_t)(F))

#define ALLOC_STRUCT(S) (S*)calloc(1, sizeof(S))
#define FREE_STRUCT(O) free(O)

typedef struct llthread_child_t {
  lua_State  *L;
  int        status;
  flags_t    flags;
} llthread_child_t;

typedef struct llthread_t {
  llthread_child_t *child;
  os_thread_t       thread;
  flags_t           flags;
} llthread_t;

//{ llthread_child

static void open_thread_libs(lua_State *L){
#ifdef LLTHREAD_REGISTER_STD_LIBRARY
#  define L_REGLIB(L, name, G) lua_pushcfunction(L, luaopen_##name); lua_setfield(L, -2, #name)
#else
#  define L_REGLIB(L, name, G) lutil_require(L, #name, luaopen_##name, G)
#endif

  int top = lua_gettop(L);
  lutil_require(L, "_G",      luaopen_base,    1);
  lutil_require(L, "package", luaopen_package, 1);
  lua_settop(L, top);

  /* get package.preload */
  lua_getglobal(L, "package"); lua_getfield(L, -1, "preload"); lua_remove(L, -2);

  /*always only register*/
  lua_pushcfunction(L, luaopen_llthreads); lua_setfield(L, -2, "llthreads");

  L_REGLIB(L, io,        1);
  L_REGLIB(L, os,        1);
  L_REGLIB(L, math,      1);
  L_REGLIB(L, table,     1);
  L_REGLIB(L, debug,     1);
  L_REGLIB(L, string,    1);

  lua_settop(L, top);
#undef L_REGLIB
}

static llthread_child_t *llthread_child_new() {
  llthread_child_t *this = ALLOC_STRUCT(llthread_child_t);
  if(!this) return NULL;

  memset(this, 0, sizeof(llthread_child_t));

  /* create new lua_State for the thread.             */
  /* open standard libraries.                         */
  /* push traceback function as first value on stack. */
  this->L = luaL_newstate();
  open_thread_libs(this->L);
  lua_pushcfunction(this->L, traceback); 

  return this;
}

static void llthread_child_destroy(llthread_child_t *this) {
  lua_close(this->L);
  FREE_STRUCT(this);
}

static OS_THREAD_RETURT llthread_child_thread_run(void *arg) {
  llthread_child_t *this = (llthread_child_t *)arg;
  lua_State *L = this->L;
  int nargs = lua_gettop(L) - 2;

  this->status = lua_pcall(L, nargs, LUA_MULTRET, 1);

  /* alwasy print errors here, helps with debugging bad code. */
  if(this->status != 0) {
    const char *err_msg = lua_tostring(L, -1);
    fprintf(stderr, "Error from thread: %s\n", err_msg);
    fflush(stderr);
  }

  /* if thread is detached, then destroy the child state. */
  if(FLAG_IS_SET(this, TSTATE_DETACHED)) {
    /* thread is detached, so it must clean-up the child state. */
    llthread_child_destroy(this);
    this = NULL;
  }

#ifndef USE_PTHREAD
  if(this) {
    /* attached thread, don't close thread handle. */
    _endthreadex(0);
  } else {
    /* detached thread, close thread handle. */
    _endthread();
  }
  return 0;
#else
  return this;
#endif
}

//}

//{ llthread

static llthread_t *llthread_new() {
  llthread_t *this = ALLOC_STRUCT(llthread_t);
  if(!this) return NULL;

  this->flags  = TSTATE_NONE;
#ifndef USE_PTHREAD
  this->thread = INVALID_THREAD;
#endif
  this->child  = llthread_child_new();
  if(!this->child){
    FREE_STRUCT(this);
    return NULL;
  }

  return this;
}

static void llthread_cleanup_child(llthread_t *this) {
  if(this->child) {
    llthread_child_destroy(this->child);
    this->child = NULL;
  }
}

static void llthread_destroy(llthread_t *this) {
  /* We still own the child thread object iff the thread was not started or
   * we have joined the thread.
   */
  if(FLAG_IS_SET(this, TSTATE_JOINED)||(this->flags == TSTATE_NONE))  {
    llthread_cleanup_child(this);
  }
  FREE_STRUCT(this);
}

static int llthread_push_args(lua_State *L, llthread_child_t *child, int idx, int top) {
  return llthread_copy_values(L, child->L, idx, top, 1 /* is_arg */);
}

static int llthread_push_results(lua_State *L, llthread_child_t *child, int idx, int top) {
  return llthread_copy_values(child->L, L, idx, top, 0 /* is_arg */);
}

static int llthread_start(llthread_t *this, int start_detached) {
  llthread_child_t *child = this->child;
  int rc = 0;

  if(start_detached){
    FLAG_SET(child, TSTATE_DETACHED);
  }

#ifndef USE_PTHREAD
  this->thread = (HANDLE)_beginthreadex(NULL, 0, llthread_child_thread_run, child, 0, NULL);
  if(INVALID_THREAD == this->thread){
    rc = -1;
  }
#else
  rc = pthread_create(&(this->thread), NULL, llthread_child_thread_run, child);
#endif

  if(rc == 0) {
    FLAG_SET(this, TSTATE_STARTED);
    if(start_detached) {
      FLAG_SET(this, TSTATE_DETACHED);
      this->child = NULL;
#ifdef USE_PTHREAD
      rc = pthread_detach(this->thread);
#endif
    }
  }

  return rc;
}

static int llthread_join(llthread_t *this, join_timeout_t timeout) {
#ifndef USE_PTHREAD
  DWORD ret = 0;
  if(INVALID_THREAD == this->thread) return 0;
  ret = WaitForSingleObject( this->thread, timeout );
  if( ret == WAIT_OBJECT_0){ /* Destroy the thread object. */
    CloseHandle( this->thread );
    this->thread = INVALID_THREAD;
    FLAG_SET(this, TSTATE_JOINED);
    return 0;
  }
  else if( ret == WAIT_TIMEOUT ){
    return 1;
  }
  return 2;
#else
  int rc;
  if(timeout == 0){
    rc = pthread_kill(this->thread, 0);
    if(rc == 0){ /* still alive */
      rc = ETIMEDOUT;
    }
    if(rc == ESRCH){ /*thread dead*/
      FLAG_SET(this, TSTATE_JOINED);
      rc = 0;
    }
    return rc;
  }

  // @todo use pthread_tryjoin_np to support timeout

  /* then join the thread. */
  rc = pthread_join(this->thread, NULL);
  if((rc == 0) || (rc == ESRCH)) {
    FLAG_SET(this, TSTATE_JOINED);
  }
  return rc;
#endif
}

static llthread_t *llthread_create(lua_State *L, const char *code, size_t code_len) {
  llthread_t *this        = llthread_new();
  llthread_child_t *child = this->child;

  /* load Lua code into child state. */
  int rc = luaL_loadbuffer(child->L, code, code_len, code);
  if(rc != 0) {
    /* copy error message to parent state. */
    size_t len; const char *str = lua_tolstring(child->L, -1, &len);
    if(str != NULL) {
      lua_pushlstring(L, str, len);
    } else {
      /* non-string error message. */
      lua_pushfstring(L, "luaL_loadbuffer() failed to load Lua code: rc=%d", rc);
    }
    llthread_destroy(this);
    lua_error(L);
    return NULL;
  }

  /* copy extra args from main state to child state. */
  /* Push all args after the Lua code. */
  llthread_push_args(L, child, 3, lua_gettop(L));

  return this;
}

//}

//{ Lua interface to llthread

#define LLTHREAD_T_NAME "LLThread"
static const char *LLTHREAD_T = LLTHREAD_T_NAME;

static llthread_t *l_llthread_at (lua_State *L, int i) {
  llthread_t **this = (llthread_t **)lutil_checkudatap (L, i, LLTHREAD_T);
  luaL_argcheck (L,  this != NULL, i, "thread expected");
  luaL_argcheck (L, *this != NULL, i, "thread expected");
  // luaL_argcheck (L, !(counter->flags & FLAG_DESTROYED), 1, "PDH Counter is destroyed");
  return *this;
}

static int l_llthread_delete(lua_State *L) {
  llthread_t **pthis = (llthread_t **)lutil_checkudatap (L, 1, LLTHREAD_T);
  llthread_t *this;
  luaL_argcheck (L, pthis != NULL, 1, "thread expected");
  this = *pthis;

  /*already exists*/
  if(this == NULL) return 0;

  /* if the thread has been started and has not been detached/joined. */
  if( FLAG_IS_SET(this, TSTATE_STARTED) && 
     !FLAG_IS_SET(this, (TSTATE_DETACHED|TSTATE_JOINED))
  ){
    /* then join the thread. */
    llthread_child_t *child = this->child;
    llthread_join(this, INFINITE_JOIN_TIMEOUT);
    if(child && child->status != 0) {
      const char *err_msg = lua_tostring(child->L, -1);
      fprintf(stderr, "Error from non-joined thread: %s\n", err_msg);
      fflush(stderr);
    }
  }

  llthread_destroy(this);
  *pthis = NULL;

  return 0;
}

static int l_llthread_start(lua_State *L) {
  llthread_t *this = l_llthread_at(L, 1);
  int start_detached = lua_toboolean(L, 2);
  int rc;

  if(this->flags != TSTATE_NONE) {
    return fail(L, "Thread already started.");
  }

  rc = llthread_start(this, start_detached);
  if(rc != 0) {
    char buf[ERROR_LEN];
    strerror_r(errno, buf, ERROR_LEN);
    return fail(L, buf);
  }

  lua_settop(L, 1); // return this
  return 1;
}

static int l_llthread_join(lua_State *L) {
  llthread_t *this = l_llthread_at(L, 1);
  llthread_child_t *child = this->child;
  int rc;

  if(!FLAG_IS_SET(this, TSTATE_STARTED )) {
    return fail(L, "Can't join a thread that hasn't be started.");
  }
  if( FLAG_IS_SET(this, TSTATE_DETACHED)) {
    return fail(L, "Can't join a thread that has been detached.");
  }
  if( FLAG_IS_SET(this, TSTATE_JOINED  )) {
    return fail(L, "Can't join a thread that has already been joined.");
  }

  /* join the thread. */
  rc = llthread_join(this, luaL_optint(L, 2, INFINITE_JOIN_TIMEOUT));

  /* Push all results after the Lua code. */
  if(child && FLAG_IS_SET(this, TSTATE_JOINED)) {
    int top;
    if(child->status != 0) {
      const char *err_msg = lua_tostring(child->L, -1);
      lua_pushboolean(L, 0);
      lua_pushfstring(L, "Error from child thread: %s", err_msg);
      top = 2;
    } else {
      lua_pushboolean(L, 1);
      top = lua_gettop(child->L);
      /* return results to parent thread. */
      llthread_push_results(L, child, 2, top);
    }
    llthread_cleanup_child(this);
    return top;
  }

  if( rc == JOIN_ETIMEDOUT ){
    lua_pushboolean(L, 0);
    lua_pushstring(L, "timeout");
    return 2;
  } 

  {
    char buf[ERROR_LEN];
    strerror_r(errno, buf, ERROR_LEN);

    llthread_cleanup_child(this);

    lua_pushboolean(L, 0);
    lua_pushstring(L, buf);
    return 2;
  }

}

static int l_llthread_new(lua_State *L) {
  size_t lua_code_len; const char *lua_code = luaL_checklstring(L, 1, &lua_code_len);
  llthread_t **this = lutil_newudatap(L, llthread_t*, LLTHREAD_T);
  lua_insert(L, 2); /*move self prior args*/
  *this = llthread_create(L, lua_code, lua_code_len);

  lua_settop(L, 2);
  return 1;
}

static const struct luaL_Reg l_llthread_meth[] = {
  {"start",         l_llthread_start         },
  {"join",          l_llthread_join          },
  {"__gc",          l_llthread_delete        },

  {NULL, NULL}
};

//}

static const struct luaL_Reg l_llthreads_lib[] = {
  {"new",           l_llthread_new           },

  {NULL, NULL}
};

LLTHREADS_EXPORT_API int luaopen_llthreads(lua_State *L) {
  int top = lua_gettop(L);
  lutil_createmetap(L, LLTHREAD_T,   l_llthread_meth,   0);
  lua_settop(L, top);

  lua_newtable(L);
  luaL_setfuncs(L, l_llthreads_lib, 0);
  return 1;
}
