# lua library to link
LUA_LIBS = -L"/home/user/Documents/lua5_1_4_Linux26g4_64_lib/" -llua5.1

# lua header location
LUA_INCLUDEPATH = "/home/user/Documents/lua5_1_4_Linux26g4_64_lib/include"

exists(../config.pri) { include(../config.pri) }

