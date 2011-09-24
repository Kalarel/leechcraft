/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
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

#include "delicious.h"
#include <QIcon>
#include "deliciousauthwidget.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace OnlineBookmarks
{
namespace Delicious
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
	}

	void Plugin::SecondInit ()
	{
	}
	
	void Plugin::Release ()
	{
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.Poshuku.OnlineBookmarks.Delicious";
	}

	QString Plugin::GetName () const
	{
		return "Poshuku OB: Delicious";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Sync local bookmarks with your account in Delicious");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	QSet<QByteArray> Plugin::GetPluginClasses () const
	{
		QSet<QByteArray> classes;
		classes << "org.LeechCraft.Plugins.Poshuku.Plugins.OnlineBookmarks.IServicePlugin";
		return classes;
	}

	IBookmarksService::Features Plugin::GetFeatures () const
	{
		return 0;
	}

	QString Plugin::GetServiceName () const
	{
		return "Delicious";
	}

	QIcon Plugin::GetServiceIcon () const
	{
		return QIcon (":/plugins/poshuku/plugins/onlinebookmarks/plugins/delicious/resources/images/delicious.png");
	}

	QWidget* Plugin::GetAuthWidget ()
	{
		return new DeliciousAuthWidget ();
	}

}
}
}
}

Q_EXPORT_PLUGIN2 (leechcraft_poshuku_onlinebookmarks_delicious,
		LeechCraft::Poshuku::OnlineBookmarks::Delicious::Plugin);