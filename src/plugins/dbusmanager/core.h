/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2012  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#pragma once

#include <memory>
#include <QObject>
#include <QDBusConnection>
#include <QStringList>
#include <interfaces/core/icoreproxy.h>
#include "notificationmanager.h"
#include "general.h"
#include "tasks.h"

namespace LeechCraft
{
struct Entity;

namespace DBusManager
{
	class Core : public QObject
	{
		Q_OBJECT

		std::unique_ptr<QDBusConnection> Connection_;
		std::unique_ptr<NotificationManager> NotificationManager_;
		std::unique_ptr<General> General_;
		std::unique_ptr<Tasks> Tasks_;

		ICoreProxy_ptr Proxy_;

		Core ();
	public:
		static Core& Instance ();
		void Release ();
		void SetProxy (ICoreProxy_ptr);
		ICoreProxy_ptr GetProxy () const;
		QString Greeter (const QString&);
		bool CouldHandle (const LeechCraft::Entity&) const;
		void Handle (const LeechCraft::Entity&);
	private:
		void DumpError ();
	private slots:
		void doDelayedInit ();
	};
}
}
