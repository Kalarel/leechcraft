IF (NOT QT_USE_FILE)
	CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
	IF (COMMAND cmake_policy)
		cmake_policy (SET CMP0003 NEW)
	ENDIF (COMMAND cmake_policy)

	PROJECT (leechcraft_qrosp)

	IF (NOT CMAKE_MODULE_PATH)
		SET (CMAKE_MODULE_PATH "/usr/local/share/leechcraft/cmake;/usr/share/leechcraft/cmake")
	ENDIF (NOT CMAKE_MODULE_PATH)

	FIND_PACKAGE (Boost REQUIRED)
	FIND_PACKAGE (Qt4 REQUIRED)
	FIND_PACKAGE (LeechCraft REQUIRED)
ENDIF (NOT QT_USE_FILE)

SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
FIND_PACKAGE (Qross REQUIRED)
FIND_PACKAGE (QJSON REQUIRED)

SET (QT_USE_QTSCRIPT TRUE)
INCLUDE (${QT_USE_FILE})
INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	${QROSS_INCLUDE_DIR}
	${QJSON_INCLUDE_DIR}
	)
SET (SRCS
	qrosp.cpp
	pluginmanager.cpp
	wrapperobject.cpp
	typesfactory.cpp
	utilproxy.cpp
	scriptloaderinstance.cpp
	loadedscript.cpp
	wrappers/coreproxywrapper.cpp
	wrappers/hookproxywrapper.cpp
	wrappers/entitywrapper.cpp
	wrappers/shortcutproxywrapper.cpp
	wrappers/pluginsmanagerwrapper.cpp
	wrappers/tagsmanagerwrapper.cpp
	)
SET (HEADERS
	qrosp.h
	pluginmanager.h
	wrapperobject.h
	typesfactory.h
	utilproxy.h
	scriptloaderinstance.h
	loadedscript.h
	wrappers/coreproxywrapper.h
	wrappers/hookproxywrapper.h
	wrappers/entitywrapper.h
	wrappers/shortcutproxywrapper.h
	wrappers/pluginsmanagerwrapper.h
	wrappers/tagsmanagerwrapper.h
	)
IF (NOT LC_NO_MOC)
	QT4_WRAP_CPP (MOC_SRCS ${HEADERS})
ENDIF (NOT LC_NO_MOC)

IF (${QT_VERSION_MINOR} LESS 8)
	SET (SRCS ${SRCS} third-party/qmetaobjectbuilder.cpp)
	SET (HEADERS ${HEADERS} third-party/qmetaobjectbuilder.h)
ELSEIF (${QT_VERSION_MINOR} EQUAL 8)
	SET (SRCS ${SRCS} third-party/qmetaobjectbuilder_48.cpp)
	SET (HEADERS ${HEADERS} third-party/qmetaobjectbuilder_48.h)
ELSE (${QT_VERSION_MINOR} LESS 8)
	MESSAGE (ERROR "Unsupported Qt version ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}, Qrosp cannot be built")
ENDIF (${QT_VERSION_MINOR} LESS 8)

ADD_LIBRARY (leechcraft_qrosp SHARED
	${COMPILED_TRANSLATIONS}
	${SRCS}
	${MOC_SRCS}
	)
TARGET_LINK_LIBRARIES (leechcraft_qrosp
	${QT_LIBRARIES}
	${LEECHCRAFT_LIBRARIES}
	${QROSS_LIBRARIES}
	${QJSON_LIBRARIES}
	)
INSTALL (TARGETS leechcraft_qrosp DESTINATION ${LC_PLUGINS_DEST})
