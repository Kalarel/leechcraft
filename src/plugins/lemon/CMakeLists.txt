CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (leechcraft_lemon)
INCLUDE (InitLCPlugin OPTIONAL)

SET (QT_USE_QTNETWORK TRUE)
INCLUDE (${QT_USE_FILE})
INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	)
SET (SRCS
	lemon.cpp
	actionsmanager.cpp
	core.cpp
	)

ADD_LIBRARY (leechcraft_lemon SHARED
	${COMPILED_TRANSLATIONS}
	${SRCS}
	)
TARGET_LINK_LIBRARIES (leechcraft_lemon
	${QT_LIBRARIES}
	${LEECHCRAFT_LIBRARIES}
	)
INSTALL (TARGETS leechcraft_lemon DESTINATION ${LC_PLUGINS_DEST})
