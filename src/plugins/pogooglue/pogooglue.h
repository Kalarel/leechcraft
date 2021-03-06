/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#include <QObject>
#include <interfaces/iinfo.h>
#include <interfaces/ientityhandler.h>
#include <interfaces/idatafilter.h>

namespace LeechCraft
{
namespace Pogooglue
{
	class Plugin : public QObject
				 , public IInfo
				 , public IEntityHandler
				 , public IDataFilter
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IEntityHandler IDataFilter)
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		void Release ();
		QByteArray GetUniqueID () const;
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		EntityTestHandleResult CouldHandle (const Entity& entity) const;
		void Handle (Entity entity);

		QString GetFilterVerb () const;
		QList<FilterVariant> GetFilterVariants () const;
	private:
		void GoogleIt (QString);
	signals:
		void gotEntity (const LeechCraft::Entity&);
	};
}
}
