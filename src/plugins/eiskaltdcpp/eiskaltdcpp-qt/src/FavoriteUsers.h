/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef FAVORITEUSERS_H
#define FAVORITEUSERS_H

#include <QObject>
#include <QWidget>
#include <QCloseEvent>
#include <QHash>
#include <QTreeWidgetItem>
#include <QMetaType>

#include "dcpp/stdinc.h"
#include "dcpp/DCPlusPlus.h"
#include "dcpp/Singleton.h"
#include "dcpp/FavoriteManager.h"

#include "ArenaWidget.h"
#include "WulforUtil.h"

#include "ui_UIFavoriteUsers.h"

class FavoriteUsersModel;

class FavoriteUsers :
        public QWidget,
        public dcpp::Singleton<FavoriteUsers>,
        public dcpp::FavoriteManagerListener,
        public ArenaWidget,
        private Ui::UIFavoriteUsers
{
Q_OBJECT
Q_INTERFACES(ArenaWidget ITabWidget)

friend class dcpp::Singleton<FavoriteUsers>;
typedef QMap<QString, QVariant> VarMap;

public:

    virtual QWidget *getWidget() { return this; }
    virtual QString getArenaTitle() const { return tr("Favourite users"); }
    virtual QString getArenaShortTitle() const { return getArenaTitle(); }
    virtual QMenu *getMenu() { return NULL; }
    const QPixmap &getPixmap() const{ return WICON(WulforUtil::eiFAVUSERS); }
    ArenaWidget::Role role() const { return ArenaWidget::FavoriteUsers; }

Q_SIGNALS:
    void coreUserAdded(VarMap);
    void coreUserRemoved(QString);
    void coreStatusChanged(QString,QString);

protected:
    virtual void closeEvent(QCloseEvent *);
    virtual bool eventFilter(QObject *, QEvent *);

    virtual void on(UserAdded, const dcpp::FavoriteUser& aUser) throw();
    virtual void on(UserRemoved, const dcpp::FavoriteUser& aUser) throw();
    virtual void on(StatusChanged, const dcpp::UserPtr& aUser) throw();

public Q_SLOTS:
    bool addUserToFav(const QString &id);
    bool remUserFromFav(const QString &id);
    QStringList getUsers() const;

private Q_SLOTS:
    void slotContextMenu();
    void slotHeaderMenu();
    void slotAutoGrant(bool b){ WBSET(WB_FAVUSERS_AUTOGRANT, b); }
    void slotSettingsChanged(const QString &key, const QString &);

    void addUser(const VarMap &);
    void updateUser(const QString &, const QString &);
    void remUser(const QString &);

private:
    FavoriteUsers(QWidget *parent = NULL);
    virtual ~FavoriteUsers();

    void handleRemove(const QString &);
    void handleDesc(const QString &);
    void handleGrant(const QString &);

    void getParams(VarMap &map, const dcpp::FavoriteUser &);

    FavoriteUsersModel *model;
};

Q_DECLARE_METATYPE (FavoriteUsers*)

#endif // FAVORITEUSERS_H
