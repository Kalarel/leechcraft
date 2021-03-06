CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (leechcraft_lackman)
INCLUDE (InitLCPlugin OPTIONAL)

OPTION (TESTS_LACKMAN "Enable LackMan tests" OFF)

SET (QT_USE_QTNETWORK TRUE)
SET (QT_USE_QTXML TRUE)
SET (QT_USE_QTXMLPATTERNS TRUE)
SET (QT_USE_QTSQL TRUE)
IF (TESTS_LACKMAN)
	SET (QT_USE_QTTEST TRUE)
ENDIF (TESTS_LACKMAN)
INCLUDE (${QT_USE_FILE})
INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	)
SET (SRCS
	lackman.cpp
	core.cpp
	repoinfo.cpp
	repoinfofetcher.cpp
	storage.cpp
	deptreebuilder.cpp
	packagesmodel.cpp
	packagesdelegate.cpp
	xmlparsers.cpp
	externalresourcemanager.cpp
	pendingmanager.cpp
	packageprocessor.cpp
	versioncomparator.cpp
	typefilterproxymodel.cpp
	xmlsettingsmanager.cpp
	delegatebuttongroup.cpp
	stringfiltermodel.cpp
	)
SET (HEADERS
	lackman.h
	core.h
	repoinfo.h
	repoinfofetcher.h
	storage.h
	deptreebuilder.h
	packagesmodel.h
	packagesdelegate.h
	xmlparsers.h
	externalresourcemanager.h
	pendingmanager.h
	packageprocessor.h
	versioncomparator.h
	typefilterproxymodel.h
	xmlsettingsmanager.h
	delegatebuttongroup.h
	stringfiltermodel.h
	)
CreateTrs ("lackman" "de;en;es;fr;it;ru_RU;uk_UA" COMPILED_TRANSLATIONS)
SET (RESOURCES
	lackmanresources.qrc
	)
SET (FORMS
	lackman.ui
	)

IF (NOT LC_NO_MOC)
	QT4_WRAP_CPP (MOC_SRCS ${HEADERS})
ENDIF (NOT LC_NO_MOC)
QT4_WRAP_UI (UIS_H ${FORMS})
QT4_ADD_RESOURCES (RCCS ${RESOURCES})

ADD_LIBRARY (leechcraft_lackman SHARED
	${COMPILED_TRANSLATIONS}
	${SRCS}
	${MOC_SRCS}
	${UIS_H}
	${RCCS}
	)
TARGET_LINK_LIBRARIES (leechcraft_lackman
	${QT_LIBRARIES}
	${LEECHCRAFT_LIBRARIES}
	)

IF (TESTS_LACKMAN)
	INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR}/tests)
	QT4_WRAP_CPP (VERSIONCOMPARATORTEST_MOC "tests/versioncomparatortest.h")
	ADD_EXECUTABLE (lc_lackman_versioncomparatortest WIN32
		tests/versioncomparatortest.cpp
		versioncomparator.cpp
		${VERSIONCOMPARATORTEST_MOC}
	)
	TARGET_LINK_LIBRARIES (lc_lackman_versioncomparatortest
		${QT_LIBRARIES}
		${LEECHCRAFT_LIBRARIES}
	)

	ADD_TEST (VersionComparator lc_lackman_versioncomparatortest)
ENDIF (TESTS_LACKMAN)

INSTALL (TARGETS leechcraft_lackman DESTINATION ${LC_PLUGINS_DEST})
INSTALL (FILES ${COMPILED_TRANSLATIONS} DESTINATION ${LC_TRANSLATIONS_DEST})
INSTALL (FILES lackmansettings.xml DESTINATION ${LC_SETTINGS_DEST})
