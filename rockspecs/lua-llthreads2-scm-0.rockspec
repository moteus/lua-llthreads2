package = "lua-llthreads2"
version = "scm-0"
source = {
  url = "https://github.com/moteus/lua-llthreads2/archive/master.zip",
  dir = "lua-llthreads2-master",
}
description = {
  summary = "Low-Level threads for Lua",
  homepage = "http://github.com/moteus/lua-llthreads2",
  license = "MIT/X11",
}
dependencies = {
  "lua >= 5.1, < 5.3",
}
build = {
  type = "builtin",
  platforms = {
    unix = {
      modules = {
        llthreads = {
          libraries = {"pthread"},
        }
      }
    },
    windows = {
      modules = {
        llthreads = {
          libraries = {"kernel32"},
        }
      }
    }
  },
  modules = {
    llthreads = {
      sources = { "src/l52util.c", "src/llthread.c" },
    }
  }
}