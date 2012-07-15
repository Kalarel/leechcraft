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

#include "sidebarview.h"
#include <QStandardItemModel>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include "modelactionwrapper.h"
#include "actioniconprovider.h"

namespace LeechCraft
{
namespace Sidebar
{
	namespace
	{
		class ActionsModel : public QStandardItemModel
		{
		public:
			ActionsModel (QObject *parent = 0)
			: QStandardItemModel (parent)
			{
				QHash<int, QByteArray> names;
				names [Qt::DisplayRole] = "actionText";
				names [ActionIconID] = "actionIcon";
				setRoleNames (names);
			}
		};
	}

	SidebarView::SidebarView (QWidget *parent)
	: QDeclarativeView (parent)
	, IconSize_ (30, 30)
	{
		setMaximumWidth (IconSize_.width () + 2);
		setResizeMode (SizeRootObjectToView);

		auto iconProv = new ActionIconProvider;

		auto initModel = [iconProv, this] (AreaType type)
		{
			Models_ [type] = new ActionsModel (this);
			Wrappers_ [type] = new ModelActionWrapper (GetAreaModel (type), iconProv);
		};
		initModel (AreaType::TabOpen);
		initModel (AreaType::QuickLaunch);
		initModel (AreaType::CurrentTab);

		engine ()->addImageProvider ("actionIcons", iconProv);

		rootContext ()->setContextProperty ("tabOpenListModel", GetAreaModel (AreaType::TabOpen));
		rootContext ()->setContextProperty ("quickLaunchModel", GetAreaModel (AreaType::QuickLaunch));
		rootContext ()->setContextProperty ("currentTabModel", GetAreaModel (AreaType::CurrentTab));

		setSource (QUrl ("qrc:/sidebar/resources/qml/Sidebar.qml"));
	}

	QStandardItemModel* SidebarView::GetAreaModel (AreaType area) const
	{
		return Models_.value (area);
	}

	ModelActionWrapper* SidebarView::GetWrapper (AreaType area) const
	{
		return Wrappers_.value (area);
	}
}
}
