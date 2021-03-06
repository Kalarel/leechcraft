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

#include "tabsessmanager.h"
#include <algorithm>
#include <QIcon>
#include <QTimer>
#include <QSettings>
#include <QCoreApplication>
#include <QMenu>
#include <QInputDialog>
#include <QMainWindow>
#include <QtDebug>
#include <util/util.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ipluginsmanager.h>
#include <interfaces/core/icoretabwidget.h>
#include <interfaces/ihaverecoverabletabs.h>
#include <interfaces/ihavetabs.h>
#include "restoresessiondialog.h"
#include "recinfo.h"

namespace LeechCraft
{
namespace TabSessManager
{
	void Plugin::Init (ICoreProxy_ptr proxy)
	{
		Util::InstallTranslator ("tabsessmanager");

		IsScheduled_ = false;
		UncloseMenu_ = new QMenu (tr ("Unclose tabs"));

		Proxy_ = proxy;
		IsRecovering_ = true;

		const auto& roots = Proxy_->GetPluginsManager ()->
				GetAllCastableRoots<IHaveRecoverableTabs*> ();
		Q_FOREACH (QObject *root, roots)
		{
			connect (root,
					SIGNAL (addNewTab (const QString&, QWidget*)),
					this,
					SLOT (handleNewTab (const QString&, QWidget*)));
			connect (root,
					SIGNAL (removeTab (QWidget*)),
					this,
					SLOT (handleRemoveTab (QWidget*)));
		}

		SessMgrMenu_ = new QMenu (tr ("Sessions"));
		SessMgrMenu_->addAction (tr ("Save current session..."),
				this,
				SLOT (saveCustomSession ()));
		SessMgrMenu_->addSeparator ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_TabSessManager");
		Q_FOREACH (const auto& group, settings.childGroups ())
			AddCustomSession (group);

		connect (proxy->GetTabWidget ()->GetObject (),
				SIGNAL (tabWasMoved (int, int)),
				this,
				SLOT (handleTabMoved (int, int)));
	}

	void Plugin::SecondInit ()
	{
		QTimer::singleShot (5000,
				this,
				SLOT (recover ()));
	}

	QByteArray Plugin::GetUniqueID () const
	{
		return "org.LeechCraft.TabSessManager";
	}

	void Plugin::Release ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_TabSessManager");
		settings.setValue ("CleanShutdown", true);
	}

	QString Plugin::GetName () const
	{
		return "TabSessManager";
	}

	QString Plugin::GetInfo () const
	{
		return tr ("Manages sessions of tabs in LeechCraft.");
	}

	QIcon Plugin::GetIcon () const
	{
		return QIcon ();
	}

	QList<QAction*> Plugin::GetActions (ActionsEmbedPlace place) const
	{
		QList<QAction*> result;
		if (place == ActionsEmbedPlace::ToolsMenu)
		{
			result << SessMgrMenu_->menuAction ();
			result << UncloseMenu_->menuAction ();
		}
		else if (place == ActionsEmbedPlace::CommonContextMenu)
			result << UncloseMenu_->menuAction ();
		return result;
	}

	bool Plugin::eventFilter (QObject*, QEvent *e)
	{
		if (e->type () != QEvent::DynamicPropertyChange)
			return false;

		auto propEvent = static_cast<QDynamicPropertyChangeEvent*> (e);
		if (propEvent->propertyName ().startsWith ("SessionData/"))
			handleTabRecoverDataChanged ();

		return false;
	}

	namespace
	{
		QList<QPair<QByteArray, QVariant>> GetSessionProps (QObject *tab)
		{
			QList<QPair<QByteArray, QVariant>> props;
			Q_FOREACH (const QByteArray& propName, tab->dynamicPropertyNames ())
			{
				if (!propName.startsWith ("SessionData/"))
					continue;

				props << qMakePair (propName, tab->property (propName));
			}
			return props;
		}
	}

	QByteArray Plugin::GetCurrentSession () const
	{
		QByteArray result;
		QDataStream str (&result, QIODevice::WriteOnly);
		Q_FOREACH (auto tab, Tabs_)
		{
			ITabWidget *tw = qobject_cast<ITabWidget*> (tab);
			if (!tw)
				continue;

			IInfo *plugin = qobject_cast<IInfo*> (tw->ParentMultiTabs ());
			if (!plugin)
				continue;

			auto rec = qobject_cast<IRecoverableTab*> (tab);
			const auto& data = rec->GetTabRecoverData ();
			if (data.isEmpty ())
				continue;

			QIcon forRecover = QIcon (rec->GetTabRecoverIcon ().pixmap (32, 32));

			str << plugin->GetUniqueID ()
					<< data
					<< rec->GetTabRecoverName ()
					<< forRecover
					<< GetSessionProps (tab);
		}

		return result;
	}

	void Plugin::AddCustomSession (const QString& name)
	{
		QAction *act = SessMgrMenu_->addAction (name,
				this,
				SLOT (loadCustomSession ()));
		act->setProperty ("TabSessManager/SessName", name);
	}

	void Plugin::handleNewTab (const QString&, QWidget *widget)
	{
		auto tab = qobject_cast<IRecoverableTab*> (widget);
		if (!tab)
			return;

		Tabs_ << widget;

		connect (widget,
				SIGNAL (tabRecoverDataChanged ()),
				this,
				SLOT (handleTabRecoverDataChanged ()));

		widget->installEventFilter (this);

		if (!tab->GetTabRecoverData ().isEmpty ())
			handleTabRecoverDataChanged ();
	}

	void Plugin::handleRemoveTab (QWidget *widget)
	{
		auto tab = qobject_cast<ITabWidget*> (widget);
		auto recTab = qobject_cast<IRecoverableTab*> (widget);
		if (!recTab || !tab)
			return;

		auto removeGuard = [this, widget] (void*)
		{
			Tabs_.removeAll (widget);
			handleTabRecoverDataChanged ();
		};
		std::shared_ptr<void> guard (static_cast<void*> (0), removeGuard);

		const auto& recoverData = recTab->GetTabRecoverData ();
		if (recoverData.isEmpty ())
			return;

		const TabUncloseInfo info
		{
			{
				recoverData,
				GetSessionProps (widget)
			},
			qobject_cast<IHaveRecoverableTabs*> (tab->ParentMultiTabs ())
		};

		const auto pos = std::find_if (UncloseAct2Data_.begin (), UncloseAct2Data_.end (),
				[&info] (const TabUncloseInfo& that) { return that.RecInfo_.Data_ == info.RecInfo_.Data_; });
		if (pos != UncloseAct2Data_.end ())
		{
			auto act = pos.key ();
			UncloseMenu_->removeAction (act);
			UncloseAct2Data_.erase (pos);
			delete act;
		}

		const auto& fm = qApp->fontMetrics ();
		const QString& elided = fm.elidedText (recTab->GetTabRecoverName (), Qt::ElideMiddle, 300);
		QAction *action = new QAction (recTab->GetTabRecoverIcon (), elided, this);
		UncloseAct2Data_ [action] = info;

		connect (action,
				SIGNAL (triggered ()),
				this,
				SLOT (handleUnclose ()));

		if (UncloseMenu_->defaultAction ())
			UncloseMenu_->defaultAction ()->setShortcut (QKeySequence ());
		UncloseMenu_->insertAction (UncloseMenu_->actions ().value (0), action);
		UncloseMenu_->setDefaultAction (action);
		action->setShortcut (QString ("Ctrl+Shift+T"));
	}

	void Plugin::handleTabMoved (int from, int to)
	{
		if (std::max (from, to) >= Tabs_.size () ||
			std::min (from, to) < 0)
		{
			qWarning () << Q_FUNC_INFO
					<< "invalid"
					<< from
					<< "->"
					<< to
					<< "; total tabs:"
					<< Tabs_.size ();
			return;
		}

		auto tab = Tabs_.takeAt (from);
		Tabs_.insert (to, tab);

		handleTabRecoverDataChanged ();
	}

	namespace
	{
		QHash<QObject*, QList<RecInfo>> GetTabsFromStream (QDataStream& str, ICoreProxy_ptr proxy)
		{
			QHash<QByteArray, QObject*> pluginCache;
			QHash<QObject*, QList<RecInfo>> tabs;

			int order = 0;
			while (!str.atEnd ())
			{
				QByteArray pluginId;
				QByteArray recData;
				QString name;
				QIcon icon;
				QList<QPair<QByteArray, QVariant>> props;

				str >> pluginId >> recData >> name >> icon >> props;
				if (!pluginCache.contains (pluginId))
				{
					QObject *obj = proxy->GetPluginsManager ()->
							GetPluginByID (pluginId);
					pluginCache [pluginId] = obj;
				}

				QObject *plugin = pluginCache [pluginId];
				if (!plugin)
				{
					qWarning () << "null plugin for" << pluginId;
					continue;
				}

				tabs [plugin] << RecInfo { order++, recData, props, name, icon };

				qDebug () << Q_FUNC_INFO << "got restore data for"
						<< pluginId << name << plugin;
			}

			Q_FOREACH (QObject *obj, tabs.keys (QList<RecInfo> ()))
				tabs.remove (obj);

			return tabs;
		}

		void AskTabs (QHash<QObject*, QList<RecInfo>>& tabs)
		{
			if (tabs.isEmpty ())
				return;

			RestoreSessionDialog dia;
			dia.SetPages (tabs);

			if (dia.exec () != QDialog::Accepted)
			{
				tabs.clear ();
				return;
			}

			tabs = dia.GetPages ();
		}

		void OpenTabs (const QHash<QObject*, QList<RecInfo>>& tabs)
		{
			QList<QPair<IHaveRecoverableTabs*, RecInfo>> ordered;
			Q_FOREACH (auto plugin, tabs.keys ())
			{
				auto ihrt = qobject_cast<IHaveRecoverableTabs*> (plugin);
				if (!ihrt)
					continue;

				Q_FOREACH (const auto& info, tabs [plugin])
					ordered << qMakePair (ihrt, info);
			}

			std::sort (ordered.begin (), ordered.end (),
					[] (decltype (ordered.at (0)) left, decltype (ordered.at (0)) right)
						{ return left.second.Order_ < right.second.Order_; });

			Q_FOREACH (const auto& pair, ordered)
				pair.first->RecoverTabs ({ TabRecoverInfo { pair.second.Data_, pair.second.Props_ } });
		}
	}

	void Plugin::handleUnclose ()
	{
		auto action = qobject_cast<QAction*> (sender ());
		if (!action)
		{
			qWarning () << Q_FUNC_INFO
					<< "sender is not an action:"
					<< sender ();
			return;
		}

		if (!UncloseAct2Data_.contains (action))
			return;

		action->deleteLater ();

		auto data = UncloseAct2Data_.take (action);
		if (UncloseMenu_->defaultAction () == action)
		{
			auto nextAct = UncloseMenu_->actions ().value (1);
			if (nextAct)
			{
				UncloseMenu_->setDefaultAction (nextAct);
				nextAct->setShortcut (QString ("Ctrl+Shift+T"));
			}
		}
		UncloseMenu_->removeAction (action);

		data.Plugin_->RecoverTabs (QList<TabRecoverInfo> () << data.RecInfo_);
	}

	void Plugin::recover ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_TabSessManager");

		QDataStream str (settings.value ("Data").toByteArray ());
		auto tabs = GetTabsFromStream (str, Proxy_);

		if (!settings.value ("CleanShutdown", false).toBool ())
			AskTabs (tabs);

		OpenTabs (tabs);

		IsRecovering_ = false;
		settings.setValue ("CleanShutdown", false);
	}

	void Plugin::handleTabRecoverDataChanged ()
	{
		if (IsRecovering_ || Proxy_->IsShuttingDown ())
			return;

		if (IsScheduled_)
			return;

		IsScheduled_ = true;
		QTimer::singleShot (2000,
				this,
				SLOT (saveDefaultSession ()));
	}

	void Plugin::saveDefaultSession ()
	{
		IsScheduled_ = false;

		const auto& result = GetCurrentSession ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_TabSessManager");
		settings.setValue ("Data", result);
	}

	void Plugin::saveCustomSession ()
	{
		const QString& name = QInputDialog::getText (Proxy_->GetMainWindow (),
				tr ("Custom session"),
				tr ("Enter the name of the session:"));
		if (name.isEmpty ())
			return;

		const auto& result = GetCurrentSession ();
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_TabSessManager");
		settings.beginGroup (name);
		settings.setValue ("Data", result);
		settings.endGroup ();

		AddCustomSession (name);
	}

	void Plugin::loadCustomSession ()
	{
		const QString& name = sender ()->
				property ("TabSessManager/SessName").toString ();
		if (name.isEmpty ())
			return;

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_TabSessManager");
		settings.beginGroup (name);
		QDataStream str (settings.value ("Data").toByteArray ());
		settings.endGroup ();

		auto tabs = GetTabsFromStream (str, Proxy_);
		OpenTabs (tabs);
	}
}
}

LC_EXPORT_PLUGIN (leechcraft_tabsessmanager, LeechCraft::TabSessManager::Plugin);
