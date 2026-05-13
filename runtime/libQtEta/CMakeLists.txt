cmake_minimum_required(VERSION 3.25)
# based on kdelibs/kjs/CMakeLists.txt
project(qteta)

include(CheckCXXCompilerFlag)

IF(WIN64)
    MESSAGE(WARNING " Windows build. Tricky workarounds enabled, plus this is untested since the 32 -> 64-bit migration.")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-cygwin")
    SET(QT_QMAKE_EXECUTABLE ./qmake-wrap.sh)
    SET(QT_MOC_EXECUTABLE ./moc-wrap.sh)
    EXECUTE_PROCESS(COMMAND cygpath -m ${CMAKE_CURRENT_SOURCE_DIR}/ OUTPUT_VARIABLE QIDL_DIR)
ELSE(WIN64)
  SET(QIDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/)
ENDIF(WIN64)

# handy default option from the FAQ...
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Qt5 requires C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5 components
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

find_library(Eta NAMES eta PATHS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} NO_DEFAULT_PATH)

if(Eta)
    set(Eta_FOUND TRUE)
    MESSAGE("Found libeta in ${Eta}, will build examples")
else(Eta)
    MESSAGE(WARNING " libeta not found. Won't build examples.")
    MESSAGE(WARNING " copy it here, and remove CMakeCache.txt to try again.")
endif(Eta)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})


########### makebindings ###############
set(makebindings_SRCS
    generator/types.cpp
    generator/codeprinter.cpp
    generator/driver.cpp
    generator/lexer.cpp
    generator/parser.cpp
    generator/bindgen.cpp
   )
ADD_EXECUTABLE(makebindings ${makebindings_SRCS})

# and the custom command
ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bindqt.h ${CMAKE_CURRENT_BINARY_DIR}/bindqt.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/qt.eti
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/makebindings ${QIDL_DIR}
  DEPENDS makebindings ${CMAKE_CURRENT_SOURCE_DIR}/qt.idl
          ${CMAKE_CURRENT_SOURCE_DIR}/bindqt.h.in
          ${CMAKE_CURRENT_SOURCE_DIR}/bindqt.cpp.in
          ${CMAKE_CURRENT_SOURCE_DIR}/qt.eti.in
)

########### next target ###############

set (QtEta_MOC_HEADERS etaqslot.h)
qt5_wrap_cpp (QtEta_MOC_OUTFILES ${QtEta_MOC_HEADERS})
set(QtEta_LIB_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/bindqt.cpp
    gcpin.cpp
    etaobj.cpp
    etaqobjectholder.cpp
    etaqwrapperfactory.cpp
    etaqproxywidgets.cpp
    etaqslot.cpp
    )


add_library(QtEta STATIC ${QtEta_LIB_SRCS} ${QtEta_MOC_OUTFILES})

# Link Qt5 libraries
target_link_libraries(QtEta Qt5::Core Qt5::Gui Qt5::Widgets)
