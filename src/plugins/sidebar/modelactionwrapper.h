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
#include <QHash>

class QStandardItemModel;
class QStandardItem;
class QAction;

namespace LeechCraft
{
namespace Sidebar
{
	class ActionIconProvider;

	class ModelActionWrapper : public QObject
	{
		Q_OBJECT

		QStandardItemModel *Wrapped_;
		QHash<QAction*, QStandardItem*> ActionItems_;

		ActionIconProvider *IconProvider_;
	public:
		ModelActionWrapper (QStandardItemModel*, ActionIconProvider*);
	private:
		void SetActionData (QAction*);
	public slots:
		void addAction (QAction*);
		void removeAction (QAction*);
	private slots:
		void handleActionChanged ();
	};
}
}
