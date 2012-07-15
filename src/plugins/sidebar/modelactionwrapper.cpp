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

#include "modelactionwrapper.h"
#include <QStandardItemModel>
#include <QAction>
#include <QtDebug>
#include "sidebarview.h"
#include "actioniconprovider.h"

namespace LeechCraft
{
namespace Sidebar
{
	ModelActionWrapper::ModelActionWrapper (QStandardItemModel *model, ActionIconProvider *prov)
	: QObject (model)
	, Wrapped_ (model)
	, IconProvider_ (prov)
	{
	}

	void ModelActionWrapper::SetActionData (QAction *action)
	{
		auto item = ActionItems_ [action];
		item->setText (action->text ());
		item->setIcon (action->icon ());

		const auto& iconId = QString::number (action->icon ().cacheKey ());
		item->setData ("image://actionIcons/" + iconId, ViewRoles::ActionIconID);

		IconProvider_->UpdateAction (action, iconId);
	}

	void ModelActionWrapper::addAction (QAction *action)
	{
		auto item = new QStandardItem;
		ActionItems_ [action] = item;
		SetActionData (action);
		Wrapped_->appendRow (item);

		connect (action,
				SIGNAL (changed ()),
				this,
				SLOT (handleActionChanged ()),
				Qt::UniqueConnection);
	}

	void ModelActionWrapper::removeAction (QAction *action)
	{
		Wrapped_->removeRow (ActionItems_.take (action)->row ());
		disconnect (action,
				SIGNAL (changed ()),
				this,
				SLOT (handleActionChanged ()));
	}

	void ModelActionWrapper::handleActionChanged ()
	{
		SetActionData (qobject_cast<QAction*> (sender ()));
	}
}
}
