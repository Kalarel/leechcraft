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

#ifndef INTERFACES_IHAVETABS_H
#define INTERFACES_IHAVETABS_H
#include <QMetaType>
#include <QList>
#include <QMap>
#include <QByteArray>
#include <QIcon>

namespace LeechCraft
{
	/** @brief Defines different behavior features of tab classes.
	 */
	enum TabFeature
	{
		/** @brief No special features.
		 */
		TFEmpty = 0x0,

		/** @brief This tab could be opened by user request.
		 *
		 * If tab class has this feature, a corresponding action in new
		 * tab menu would be created to allow the user to open this tab.
		 *
		 * If tab class doesn't have this feature, the only way for the
		 * tab to be opened is for the corresponding to emit the
		 * IHaveTabs::addNewTab() signal when needed.
		 *
		 * @sa TFSingle.
		 */
		TFOpenableByRequest = 0x01,

		/** @brief There could be only one instance of this tab.
		 *
		 * By default, LeechCraft considers that each tab having the
		 * TFOpenableByRequest feature could be opened multiple times,
		 * but sometimes it doesn't make sense to have more than one tab
		 * of some class. In this case, this feature should also be
		 * present for that tab class.
		 *
		 * This feature requires the TFOpenableByRequest feature as
		 * well.
		 *
		 * @sa TFOpenableByRequest.
		 */
		TFSingle = 0x02,

		/** @brief The tab should be opened by default.
		 *
		 * By default, all tabs are hidden, both having TFSingle feature
		 * and lacking it. If a tab wants to be shown after LeechCraft
		 * startup until the user manually closes it, the corresponding
		 * tab class should have this feature as well.
		 */
		TFByDefault = 0x04,

		/** @brief The tab is to be suggested in a quick launch area.
		 *
		 * Tabs having this flag are expected to be contained by default
		 * in some kind of quick launch area like the one in the Sidebar
		 * plugin.
		 *
		 * Consider adding this flag if you think user would often open
		 * tabs of your class.
		 */
		TFSuggestOpening = 0x08
	};

	Q_DECLARE_FLAGS (TabFeatures, LeechCraft::TabFeature);

	/** @brief The structure describing a single tab class.
	 */
	struct TabClassInfo
	{
		/** @brief The tab class ID, which should be globally unique.
		 *
		 * This ID would be passed to the IHaveTabs::TabOpenRequested()
		 * method if the user decides to open a tab of this class.
		 */
		QByteArray TabClass_;

		/** @brief Visible name for the given tab class.
		 *
		 * The visible name is used, for example, on actions used to
		 * open tabs of this class.
		 */
		QString VisibleName_;

		/** @brief The description of the given tab class.
		 *
		 * A human-readable string with description of the purpose of
		 * the tabs belonging to this tab class.
		 */
		QString Description_;

		/** @brief The icon for the given tab class.
		 *
		 * This icon is used, for example, on actions used to open tabs
		 * of this class.
		 */
		QIcon Icon_;

		/** @brief The priority of this tab class.
		 *
		 * Refer to the documentation of IHaveTabs for the explanation
		 * of the priorities system.
		 */
		quint16 Priority_;

		/** @brief The features of this tab class.
		 */
		TabFeatures Features_;
	};

	typedef QList<TabClassInfo> TabClasses_t;
};

class QToolBar;
class QAction;

/** @brief This interface defines methods that should be implemented in
 * widgets added to the main tab widget.
 */
class ITabWidget
{
public:
	virtual ~ITabWidget () {}

	/** @brief Returns the description of the tab class of this tab.
	 *
	 * The entry must be the same as the one with the same TabClass_
	 * returned from the IHaveTabs::GetTabClasses().
	 *
	 * @return The tab class description.
	 *
	 * @sa IHavetabs::GetTabClasses()
	 */
	virtual LeechCraft::TabClassInfo GetTabClassInfo () const = 0;

	/** @brief Returns the pointer to the plugin this tab belongs to.
	 *
	 * The returned object must implement IHaveTabs and must be the one
	 * that called IHaveTabs::addNewTab() with this tab as the
	 * parameter.
	 *
	 * @return The pointer to the plugin that this tab belongs to.
	 */
	virtual QObject* ParentMultiTabs () = 0;

	/** @brief Requests to remove the tab.
	 *
	 * This method is called by LeechCraft Core (or other plugins) when
	 * this tab should be closed, for example, when user clicked on the
	 * 'x' in the tab bar. The tab may ask the user if he really wants
	 * to close the tab, for example. The tab is free to ignore the
	 * close request (in this case nothing should be done at all) or
	 * accept it by emitting IHavetabs::removeTab() signal, passing this
	 * tab widget as its parameter.
	 */
	virtual void Remove () = 0;

	/** @brief Requests tab's toolbar.
	 *
	 * This method is called to obtain the tab toolbar. In current
	 * implementation, it would be shown on top of the LeechCraft's main
	 * window.
	 *
	 * If the tab has no toolbar, 0 should be returned.
	 *
	 * @return The tab's toolbar, or 0 if there is no toolbar.
	 */
	virtual QToolBar* GetToolBar () const = 0;

	/** @brief Returns the list of QActions for the context menu of the
	 * tabbar.
	 *
	 * These actions would be shown in the context menu that would
	 * pop-up when the user right-clicks this tab's button in the
	 * tabbar.
	 *
	 * The default implementation returns an empty list.
	 *
	 * @return The list of actions for tabbar context menu.
	 */
	virtual QList<QAction*> GetTabBarContextMenuActions () const
	{
		return QList<QAction*> ();
	}

	/** @brief Returns the list of QActions to be inserted into global
	 * menu.
	 *
	 * For each key in the map (except special values, which would be
	 * defined later), the corresponding submenu would be created in
	 * LeechCraft global menu, and the corresponding list of actions
	 * would be inserted into that submenu. The submenus created would
	 * only be visible when this tab is active.
	 *
	 * There are special values for the keys:
	 * - "view" for the View menu.
	 * - "tools" for the Tools menu.
	 *
	 * The default implementation returns an empty map.
	 *
	 * @return The map with keys identifying menus and values containing
	 * lists of actions to be inserted into corresponding menus.
	 */
	virtual QMap<QString, QList<QAction*>> GetWindowMenus () const
	{
		return QMap<QString, QList<QAction*>> ();
	}

	/** @brief This method is called when this tab becomes active.
	 *
	 * The default implementation does nothing.
	 */
	virtual void TabMadeCurrent ()
	{
	}

	/** @brief This method is called when another tab becomes active.
	 *
	 * This method is called only if this tab was active before the
	 * other tab activates.
	 */
	virtual void TabLostCurrent ()
	{
	}
};

/** @brief Interface for plugins that have one or more tabs.
 *
 * Each plugin that may have tabs in LeechCraft should implement this
 * interface.
 *
 * Plugins implementing this interface may have one or more tabs of
 * different semantics, like chat tabs and service discovery tabs in an
 * IM or download tab and hub browse tab in a DirectConnect plugin.
 *
 * Different tabs with different semantics are said to belong to
 * different tab classes. Different tab classes may have different
 * behavior, but tabs of the same tab class are considered to be
 * semantically equivalent. For example, there may be only one opened
 * tab with the list of active downloads at a time, but there may be
 * many simultaneously opened tabs for hub browsing. Tab behavior is
 * defined by the LeechCraft::TabFeature enum.
 *
 * Different tab classes may have different priorities. The priorities
 * system is used to try to guess the most-currently-wanted tab by the
 * user. When user requests a new tab, but doesn't specify its type (for
 * example, just hits Ctrl+T), priorities of two tab classes are
 * compared: the priority of the class of the current tab and the
 * highest priority among all the tabs. If current priority plus some
 * delta is higher than maximum one, a new instance of current tab class
 * is opened, otherwise the tab with the highest priority is opened. For
 * example, if web browser tab has priority of 80, text editor — 70 and
 * search plugin — 60, and delta is 15, then if current tab is web
 * browser or search plugin, the new tab will be a web browser tab (since
 * 60 + 15 < 80), but if the current tab is text editor's one, then the
 * new tab will also be a text editor (70 + 15 > 80).
 *
 * In future implementations user may be allowed to adjust the delta and
 * priorities of different classes to his liking.
 *
 * @note You mustn't use tab-related signals before SecondInit() has
 * been called on your plugin, but you may use them in SecondInit() or
 * later.
 *
 * @sa ITabWidget, LeechCraft::TabClassInfo
 */
class IHaveTabs
{
public:
	virtual ~IHaveTabs () {}

	/** @brief Returns the list of tab classes provided by this plugin.
	 *
	 * This list must not change between different calls to this
	 * function.
	 *
	 * @note Actually, not all tab classes returned from this method
	 * have to result in a new tab being opened when called the
	 * TabOpenRequested() method. For example, the Azoth plugin returns
	 * a tab class for a fictional tab that, when passed to the
	 * TabOpenRequested() method, results in MUC join dialog appearing.
	 *
	 * @return The list of tab classes this plugin provides.
	 *
	 * @sa TabClassInfo, ITabWidget::GetTabClass(), TabOpenRequested().
	 */
	virtual LeechCraft::TabClasses_t GetTabClasses () const = 0;

	/** @brief Opens the new tab from the given tabClass.
	 *
	 * This method is called to notify the plugin that a tab of the
	 * given tabClass is requested by the user.
	 *
	 * @note Please note that the call to this method doesn't have to
	 * result in a new tab being opened. Refer to the note in
	 * GetTabClasses() documentation for more information.
	 *
	 * @param[in] tabClass The class of the requested tab, from the
	 * returned from GetTabClasses() list.
	 *
	 * @sa GetTabClasses()
	 */
	virtual void TabOpenRequested (const QByteArray& tabClass) = 0;

	/** @brief This signal is emitted by plugin to add a new tab.
	 *
	 * tabContents must implement the ITabWidget interface to be
	 * successfully added to the tab widget.
	 *
	 * For the tab to have an icon, emit the changeTabIcon() signal
	 * after emitting this one.
	 *
	 * @note This function is expected to be a signal in subclasses.
	 *
	 * @param[out] name The initial name of the tab.
	 * @param[out] tabContents The widget to be added, must implement
	 * ITabWidget.
	 *
	 * @sa removeTab(), changeTabName(), changeTabIcon(),
	 * statusBarChanged(), raiseTab().
	 */
	virtual void addNewTab (const QString& name, QWidget *tabContents) = 0;

	/** @brief This signal is emitted by plugin when it wants to remove
	 * a tab.
	 *
	 * tabContents must be a widget previously added by emitting the
	 * addNewTab() signal by this same plugin.
	 *
	 * @note This function is expected to be a signal in subclasses.
	 *
	 * @param[out] tabContents The widget to remove from the tab widget,
	 * must be previously added with addNewTab().
	 *
	 * @sa addNewTab()
	 */
	virtual void removeTab (QWidget *tabContents) = 0;

	/** @brief This signal is emitted by plugin to change the name of
	 * the tab with the given tabContents.
	 *
	 * The name of the tab is shown in the tab bar of the tab widget. It
	 * also may be shown in other places and contexts, like in the
	 * LeechCraft title bar when the corresponding tab is active.
	 *
	 * The tab is identified by tabContents, which should be the widget
	 * previously added by emitting the addNewTab() signal by this same
	 * plugin.
	 *
	 * @note This function is expected to be a signal in subclasses.
	 *
	 * @param[out] tabContents The widget with the contents of the tab
	 * which name should be changed, added previously with addNewTab().
	 * @param[out] name The new name of the tab with tabContents.
	 *
	 * @sa addNewTab().
	 */
	virtual void changeTabName (QWidget *tabContents, const QString& name) = 0;

	/** @brief This signal is emitted by plugin to change the icon of
	 * the tab with the given tabContents.
	 *
	 * The tab is identified by tabContents, which should be the widget
	 * previously added by emitting the addNewTab() signal by this same
	 * plugin.
	 *
	 * Null icon object may be used to clear the icon.
	 *
	 * @note This function is expected to be a signal in subclasses.
	 *
	 * @param[out] tabContents The widget with the contents of the tab
	 * which icon should be changed, added previously with addNewTab().
	 * @param[out] icon The new icon of the tab with tabContents.
	 *
	 * @sa addNewTab().
	 */
	virtual void changeTabIcon (QWidget *tabContents, const QIcon& icon) = 0;

	/** @brief This signal is emitted by plugin to change the status bar
	 * text for the tab with the given tabContents.
	 *
	 * The text set by this signal would be shown when the corresponding
	 * tab is active. To clear the status bar, this signal should be
	 * emitted with empty text.
	 *
	 * The tab is identified by tabContents, which should be the widget
	 * previously added by emitting the addNewTab() signal by this same
	 * plugin.
	 *
	 * @note This function is expected to be a signal in subclasses.
	 *
	 * @note User may choose to hide the status bar, so important
	 * information should not be presented this way.
	 *
	 * @param[out] tabContents The widget with the contents of the tab
	 * which statusbar text should be changed, added previously with
	 * addNewTab().
	 * @param[out] text The new statusbar text of the tab with
	 * tabContents.
	 *
	 * @sa addNewTab().
	 */
	virtual void statusBarChanged (QWidget *tabContents, const QString& text) = 0;

	/** @brief This signal is emitted by plugin to bring the tab with
	 * the given tabContents to the front.
	 *
	 * The tab is identified by tabContents, which should be the widget
	 * previously added by emitting the addNewTab() signal by this same
	 * plugin.
	 *
	 * @note This function is expected to be a signal in subclasses.
	 *
	 * @param[out] tabContents The widget with the contents of the tab
	 * that should be brought to the front.
	 */
	virtual void raiseTab (QWidget *tabContents) = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS (LeechCraft::TabFeatures);

Q_DECLARE_INTERFACE (ITabWidget, "org.Deviant.LeechCraft.ITabWidget/1.0");
Q_DECLARE_INTERFACE (IHaveTabs, "org.Deviant.LeechCraft.IHaveTabs/1.0");

#endif
