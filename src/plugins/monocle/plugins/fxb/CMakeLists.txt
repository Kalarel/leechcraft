CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (leechcraft_monocle_fxb)
INCLUDE (InitLCPlugin OPTIONAL)

SET (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

SET (QT_USE_QTXML TRUE)
INCLUDE (${QT_USE_FILE})
INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	)
SET (SRCS
	fxb.cpp
	document.cpp
	documentadapter.cpp
	fb2converter.cpp
	toclink.cpp
	)

ADD_LIBRARY (leechcraft_monocle_fxb SHARED
	${SRCS}
	)
TARGET_LINK_LIBRARIES (leechcraft_monocle_fxb
	${QT_LIBRARIES}
	${LEECHCRAFT_LIBRARIES}
	)
INSTALL (TARGETS leechcraft_monocle_fxb DESTINATION ${LC_PLUGINS_DEST})
IF (UNIX AND NOT APPLE)
	INSTALL (FILES freedesktop/leechcraft-monocle-fxb.desktop DESTINATION share/applications)
ENDIF (UNIX AND NOT APPLE)
