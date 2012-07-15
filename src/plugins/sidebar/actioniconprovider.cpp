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

#include "actioniconprovider.h"
#include <QAction>
#include <QIcon>

namespace LeechCraft
{
namespace Sidebar
{
	ActionIconProvider::ActionIconProvider ()
	: QDeclarativeImageProvider (Pixmap)
	{
	}

	void ActionIconProvider::UpdateAction (QAction *act, const QString& id)
	{
		Action2ID_ [act] = id;
	}

	QPixmap ActionIconProvider::requestPixmap (const QString& id, QSize *size, const QSize& requestedSize)
	{
		auto action = Action2ID_.key (id);
		if (!action)
			return QPixmap ();

		const auto& icon = action->icon ();
		const auto& targetSize = requestedSize.isValid () ?
				requestedSize :
				QSize (32, 32);

		const auto& res = icon
				.pixmap (targetSize)
				.scaled (targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		if (size)
			*size = res.size ();
		return res;
	}
}
}
