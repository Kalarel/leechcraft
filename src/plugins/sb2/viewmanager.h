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
#include <QUrl>
#include <QHash>
#include <interfaces/core/icoreproxy.h>
#include <xmlsettingsdialog/xmlsettingsdialog.h>

class QStandardItemModel;

namespace LeechCraft
{
struct QuarkComponent;

namespace SB2
{
	class SBView;
	class QuarkSettingsManager;

	class ViewManager : public QObject
	{
		Q_OBJECT

		ICoreProxy_ptr Proxy_;
		QStandardItemModel *ViewItemsModel_;
		SBView *View_;

		struct QuarkSettings
		{
			Util::XmlSettingsDialog_ptr XSD_;
			QuarkSettingsManager *SettingsManager_;
		};
		QHash<QUrl, QuarkSettings> Quark2Settings_;
	public:
		ViewManager (ICoreProxy_ptr, QObject* = 0);

		SBView* GetView () const;

		void SecondInit ();
		void AddComponent (const QuarkComponent&);

		void ShowSettings (const QUrl&);
	private:
		bool CreateSettings (const QUrl&);
	};
}
}
