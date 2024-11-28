aux_source_directory(lib TOOLKIT_LIB_SRC)

include_directories(lib)

# 查找必要的软件包
find_package(PkgConfig REQUIRED)
pkg_check_modules(PKG_JSON REQUIRED nlohmann_json)
pkg_check_modules(PKG_GLIB REQUIRED glib-2.0)
pkg_check_modules(PKG_GIO REQUIRED gio-2.0)
pkg_check_modules(PKG_DBUS REQUIRED dbus-1)

# 包含目录和链接库
include_directories(
    ${PKG_JSON_INCLUDE_DIRS}
    ${PKG_GLIB_INCLUDE_DIRS}
    ${PKG_GIO_INCLUDE_DIRS}
    ${PKG_DBUS_INCLUDE_DIRS}
)
link_libraries(-lspdlog -ldbus-1 -lgio-2.0 -lglib-2.0 -lgobject-2.0)

# 添加库文件
add_library(ipc_toolkit SHARED ${TOOLKIT_LIB_SRC})

# 添加可执行文件
add_executable(uds tool/uds.cpp)
add_executable(dbus tool/dbus.cpp)
add_executable(u_dbus tool/u_dbus.cpp)
add_executable(uds_server test/uds_server.cpp)
add_executable(dbus_server test/dbus_server.cpp)
add_executable(glib_dbus_server test/glib_dbus_server.cpp)
add_executable(glib_dbus_client test/glib_dbus_client.cpp)
add_executable(glib_simplified_dbus_server test/glib_simplified_dbus_server.cpp)
add_executable(glib_simplified_dbus_client test/glib_simplified_dbus_client.cpp)

# 链接库到对应目标
target_link_libraries(uds_server ipc_toolkit)
target_link_libraries(dbus_server ipc_toolkit)
target_link_libraries(uds ipc_toolkit)
target_link_libraries(dbus ipc_toolkit)
target_link_libraries(u_dbus ipc_toolkit)
target_link_libraries(glib_dbus_server ipc_toolkit)
target_link_libraries(glib_dbus_client ipc_toolkit)
target_link_libraries(glib_simplified_dbus_server ipc_toolkit)
target_link_libraries(glib_simplified_dbus_client ipc_toolkit)