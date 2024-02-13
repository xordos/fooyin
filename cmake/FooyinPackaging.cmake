set(CPACK_PACKAGE_NAME "fooyin")
set(CPACK_PACKAGE_VENDOR "fooyin")
set(CPACK_PACKAGE_CONTACT "Luke Taylor <luket1@proton.me>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A customisable music player")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dist/PackageDescription.txt")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "fooyin")
set(CPACK_PACKAGE_EXECUTABLES "fooyin")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/data/icons/sc-fooyin.svg")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ludouzi/fooyin")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES ON)
set(CPACK_CREATE_DESKTOP_LINKS "fooyin")
set(CPACK_PACKAGE_VERSION_MAJOR "${FOOYIN_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${FOOYIN_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${FOOYIN_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${FOOYIN_VERSION}")

set(CPACK_SOURCE_IGNORE_FILES  "\\\\.#;/#;.*~;\\\\.o$")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.git/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/\\\\.github/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "/build/")
list(APPEND CPACK_SOURCE_IGNORE_FILES "${CMAKE_CURRENT_BINARY_DIR}/")
set(CPACK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(CPACK_DEBIAN_PACKAGE_VERSION "${FOOYIN_VERSION}")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libasound2, libtag1v5, libqt6core6, libqt6gui6, libqt6sql6, libqt6sql6-sqlite, libqt6widgets6, libqt6svg6, libqt6network6, ffmpeg")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
file(READ ${CPACK_PACKAGE_DESCRIPTION_FILE} CPACK_DEBIAN_PACKAGE_DESCRIPTION)
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION_MERGED "${CPACK_DEBIAN_PACKAGE_DESCRIPTION}")
string(PREPEND CPACK_DEBIAN_PACKAGE_DESCRIPTION_MERGED "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}" "\n")
string(REPLACE "\n\n" "\n.\n" CPACK_DEBIAN_PACKAGE_DESCRIPTION_MERGED "${CPACK_DEBIAN_PACKAGE_DESCRIPTION_MERGED}")
string(REPLACE "\n" "\n " CPACK_DEBIAN_PACKAGE_DESCRIPTION_MERGED "${CPACK_DEBIAN_PACKAGE_DESCRIPTION_MERGED}")

if (NOT CPACK_DEBIAN_PACKAGE_RELEASE)
    set(CPACK_DEBIAN_PACKAGE_RELEASE 1)
endif()

set(CPACK_DEBIAN_DISTRIBUTION_RELEASES jammy mantic noble)
set(CPACK_DEBIAN_SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(CPACK_DEBIAN_INSTALL_SCRIPT "${CMAKE_SOURCE_DIR}/dist/linux/PackageDeb.cmake")

set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/dist/PackageConfig.cmake" )

include(CPack)
