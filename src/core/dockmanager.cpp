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

#include "dockmanager.h"
#include <QDockWidget>
#include <QToolButton>
#include "mainwindow.h"
#include "tabmanager.h"
#include "core.h"

namespace LeechCraft
{
	DockManager::DockManager (MainWindow *mw, QObject *parent)
	: QObject (parent)
	, MW_ (mw)
	{
		MW_->GetDockListWidget (Qt::LeftDockWidgetArea)->hide ();
		MW_->GetDockListWidget (Qt::RightDockWidgetArea)->hide ();
	}

	void DockManager::AddDockWidget (QDockWidget *dw, Qt::DockWidgetArea area)
	{
		MW_->addDockWidget (area, dw);
		connect (dw,
				SIGNAL (dockLocationChanged (Qt::DockWidgetArea)),
				this,
				SLOT (handleDockLocationChanged (Qt::DockWidgetArea)));

		connect (dw,
				SIGNAL (destroyed (QObject*)),
				this,
				SLOT (handleDockDestroyed ()));

		TabifyDW (dw, area);
		Area2Widgets_ [area] << dw;
	}

	void DockManager::AssociateDockWidget (QDockWidget *dock, QWidget *tab)
	{
		dock->installEventFilter (this);

		TabAssociations_ [dock] = tab;

		handleTabChanged (Core::Instance ().GetTabManager ()->GetCurrentWidget ());

		auto toggleAct = dock->toggleViewAction ();
		ToggleAct2Dock_ [toggleAct] = dock;
		connect (toggleAct,
				SIGNAL (triggered (bool)),
				this,
				SLOT (handleDockToggled (bool)));
	}

	void DockManager::TabifyDW (QDockWidget *dw, Qt::DockWidgetArea area)
	{
		auto widgets = Area2Widgets_ [area];
		widgets.removeAll (dw);
		if (!widgets.isEmpty ())
		{
			MW_->tabifyDockWidget (dw, widgets.last ());
			dw->show ();
			dw->raise ();
		}
	}

	bool DockManager::eventFilter (QObject *obj, QEvent *event)
	{
		if (event->type () != QEvent::Close)
			return false;

		auto dock = qobject_cast<QDockWidget*> (obj);
		if (!dock)
			return false;

		ForcefullyClosed_ << dock;

		return false;
	}

	void DockManager::handleDockDestroyed ()
	{
		auto dock = static_cast<QDockWidget*> (sender ());
		TabAssociations_.remove (dock);
		ToggleAct2Dock_.remove (ToggleAct2Dock_.key (dock));
		ForcefullyClosed_.remove (dock);

		for (const auto& key : Area2Widgets_.keys ())
			Area2Widgets_ [key].removeAll (dock);
	}

	void DockManager::handleDockLocationChanged (Qt::DockWidgetArea area)
	{
		auto dw = qobject_cast<QDockWidget*> (sender ());
		if (!dw)
			return;

		Qt::DockWidgetArea from = Qt::NoDockWidgetArea;
		Q_FOREACH (from, Area2Widgets_.keys ())
		{
			if (Area2Widgets_ [from].removeAll (dw))
				break;
			from = Qt::NoDockWidgetArea;
		}

		Area2Widgets_ [area] << dw;
	}

	void DockManager::handleDockToggled (bool isVisible)
	{
		auto dock = ToggleAct2Dock_ [static_cast<QAction*> (sender ())];
		if (!dock)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown toggler"
					<< sender ();
			return;
		}

		if (isVisible)
			ForcefullyClosed_.remove (dock);
		else
			ForcefullyClosed_ << dock;
	}

	void DockManager::handleTabChanged (QWidget *tabWidget)
	{
		Q_FOREACH (QDockWidget *dock, TabAssociations_.keys ())
		{
			if (TabAssociations_ [dock] != tabWidget)
				dock->setVisible (false);
			else if (!ForcefullyClosed_.contains (dock))
			{
				dock->setVisible (true);
				TabifyDW (dock, MW_->dockWidgetArea (dock));
			}
		}
	}
}
