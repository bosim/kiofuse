project(kiofuse)

find_package(ECM REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake/modules
    ${CMAKE_MODULE_PATH}
    ${ECM_MODULE_PATH}
)

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(FeatureSummary)

find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(KF5KIO REQUIRED)
find_package(KF5I18n REQUIRED)
find_package(FUSE 2.9 REQUIRED)

set(kiofuse_SRCS
    main.cpp
    kiofuseops.cpp
    kiofuseapp.cpp
    basejobhelper.cpp
    jobhelpers.cpp
    cache.cpp
    fusethread.cpp
)

add_definitions(-g -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE)
add_executable(kiofuse ${kiofuse_SRCS})

target_link_libraries(kiofuse
    Qt5::Core
    Qt5::Widgets
    KF5::KIOCore
    KF5::I18n
    ${FUSE_LIBRARIES}
)

install(
    TARGETS
    kiofuse
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)
