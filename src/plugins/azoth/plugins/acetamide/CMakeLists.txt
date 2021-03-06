CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (leechcraft_azoth_acetamide)
INCLUDE (InitLCPlugin OPTIONAL)

SET (QT_USE_QTNETWORK TRUE)
SET (QT_USE_QTXML TRUE)
INCLUDE (${QT_USE_FILE})
INCLUDE_DIRECTORIES (${AZOTH_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}/azoth
	)

SET (ACETAMIDE_SRCS
	acetamide.cpp
	bookmarkeditwidget.cpp
	channelclentry.cpp
	channelconfigwidget.cpp
	channelhandler.cpp
	channelparticipantentry.cpp
	channelpublicmessage.cpp
	channelsmanager.cpp
	clientconnection.cpp
	core.cpp
	entrybase.cpp
	ircaccount.cpp
	ircaccountconfigurationdialog.cpp
	ircaccountconfigurationwidget.cpp
	ircerrorhandler.cpp
	ircjoingroupchat.cpp
	ircmessage.cpp
	ircparser.cpp
	ircparticipantentry.cpp
	ircprotocol.cpp
	ircserversocket.cpp
	ircserverclentry.cpp
	ircserverhandler.cpp
	invitechannelsdialog.cpp
	localtypes.cpp
	newnickservidentifydialog.cpp
	nickservidentifywidget.cpp
	rplisupportparser.cpp
	servercommandmessage.cpp
	serverinfowidget.cpp
	serverparticipantentry.cpp
	serverresponsemanager.cpp
	sortfilterproxymodel.cpp
	sslerrorsdialog.cpp
	usercommandmanager.cpp
	xmlsettingsmanager.cpp
	vcarddialog.cpp
	)

SET (ACETAMIDE_HEADERS
	acetamide.h
	bookmarkeditwidget.h
	channelclentry.h
	channelconfigwidget.h
	channelhandler.h
	channelparticipantentry.h
	channelpublicmessage.h
	channelsmanager.h
	clientconnection.h
	core.h
	entrybase.h
	ircaccount.h
	ircaccountconfigurationdialog.h
	ircaccountconfigurationwidget.h
	ircerrorhandler.h
	ircjoingroupchat.h
	ircmessage.h
	ircparser.h
	ircparticipantentry.h
	ircprotocol.h
	ircserversocket.h
	ircserverclentry.h
	ircserverhandler.h
	invitechannelsdialog.h
	localtypes.h
	newnickservidentifydialog.h
	nickservidentifywidget.h
	rplisupportparser.h
	servercommandmessage.h
	serverinfowidget.h
	serverparticipantentry.h
	serverresponsemanager.h
	sortfilterproxymodel.h
	sslerrorsdialog.h
	usercommandmanager.h
	xmlsettingsmanager.h
	vcarddialog.h
	)

SET (ACETAMIDE_FORMS
	bookmarkeditwidget.ui
	channelconfigwidget.ui
	ircaccountconfigurationdialog.ui
	ircaccountconfigurationwidget.ui
	ircjoingroupchat.ui
	invitechannelsdialog.ui
	newnickservidentifydialog.ui
	nickservidentifywidget.ui
	serverinfowidget.ui
	sslerrorsdialog.ui
	vcarddialog.ui
	)
SET (ACETAMIDE_RESOURCES azothacetamideresources.qrc)

CreateTrs ("azoth_acetamide" "de;en;es;fr;it;ru_RU;uk_UA" ACETAMIDE_COMPILED_TRANSLATIONS)

IF (NOT LC_NO_MOC)
	QT4_WRAP_CPP (ACETAMIDE_MOC_SRCS ${ACETAMIDE_HEADERS})
ENDIF (NOT LC_NO_MOC)
QT4_WRAP_UI (ACETAMIDE_UIS_H ${ACETAMIDE_FORMS})
QT4_ADD_RESOURCES (ACETAMIDE_RCCS ${ACETAMIDE_RESOURCES})

ADD_LIBRARY (leechcraft_azoth_acetamide SHARED
	${ACETAMIDE_COMPILED_TRANSLATIONS}
	${ACETAMIDE_SRCS}
	${ACETAMIDE_MOC_SRCS}
	${ACETAMIDE_UIS_H}
	${ACETAMIDE_RCCS}
	)
TARGET_LINK_LIBRARIES (leechcraft_azoth_acetamide
	${QT_LIBRARIES}
	${LEECHCRAFT_LIBRARIES}
	)

INSTALL (TARGETS leechcraft_azoth_acetamide
		DESTINATION ${LC_PLUGINS_DEST})
INSTALL (FILES ${ACETAMIDE_COMPILED_TRANSLATIONS}
		DESTINATION ${LC_TRANSLATIONS_DEST})
INSTALL (FILES azothacetamidesettings.xml
		DESTINATION ${LC_SETTINGS_DEST})
IF (UNIX AND NOT APPLE)
	INSTALL (FILES freedesktop/leechcraft-azoth-acetamide.desktop DESTINATION share/applications)
ENDIF (UNIX AND NOT APPLE)
