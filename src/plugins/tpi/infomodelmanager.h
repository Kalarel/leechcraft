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

#include <QObject>
#include <QPersistentModelIndex>
#include <interfaces/core/icoreproxy.h>

class QStandardItemModel;
class QStandardItem;
class QAbstractItemModel;

namespace LeechCraft
{
namespace TPI
{
	class InfoModelManager : public QObject
	{
		Q_OBJECT

		ICoreProxy_ptr Proxy_;
		QStandardItemModel *Model_;

		QHash<QPersistentModelIndex, QStandardItem*> PIdx2Item_;
	public:
		InfoModelManager (ICoreProxy_ptr, QObject* = 0);

		QAbstractItemModel* GetModel () const;

		void SecondInit ();
	private:
		void ManageModel (QAbstractItemModel*);
		void HandleRows (QAbstractItemModel*, int, int);
		void HandleData (QAbstractItemModel*, int, int);
	private slots:
		void handleRowsInserted (const QModelIndex&, int, int);
		void handleRowsRemoved (const QModelIndex&, int, int);
		void handleDataChanged (const QModelIndex&, const QModelIndex&);
	};
}
}
