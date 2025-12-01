enabled_on(i386-pc-windows-msvc)

message("Adding custom tx_user.h for windows")
# This will provide tx_user.h to rtosmon lib during windows compilation
target_include_directories(fwlib.lib.rtosmon PUBLIC ${CMAKE_CURRENT_LIST_DIR})