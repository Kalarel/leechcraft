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

#ifndef PLUGINS_AZOTH_CORE_H
#define PLUGINS_AZOTH_CORE_H
#include <functional>
#include <boost/scoped_ptr.hpp>
#include <QObject>
#include <QSet>
#include <QIcon>
#include <QDateTime>
#ifdef ENABLE_CRYPT
#include <QtCrypto>
#endif
#include <util/resourceloader.h>
#include <interfaces/core/ihookproxy.h>
#include <interfaces/ianemitter.h>
#include <interfaces/iinfo.h>
#include "interfaces/azoth/iclentry.h"
#include "interfaces/azoth/azothcommon.h"
#include "interfaces/azoth/imucentry.h"
#include "interfaces/azoth/iprotocol.h"
#include "interfaces/azoth/iauthable.h"
#include "interfaces/azoth/ichatstyleresourcesource.h"
#include "interfaces/azoth/isupportriex.h"
#include "sourcetrackingmodel.h"
#include "animatediconmanager.h"

class QStandardItemModel;
class QStandardItem;

namespace LeechCraft
{
namespace Util
{
	class ResourceLoader;
	class ShortcutManager;
}

namespace Azoth
{
	class ICLEntry;
	class IAccount;
	class IMessage;
	class IEmoticonResourceSource;
	class IChatStyleResourceSource;

	class ChatTabsManager;
	class PluginManager;
	class ProxyObject;
	class TransferJobManager;
	class CallManager;
	class EventsNotifier;
	class ActionsManager;
	class ImportManager;
	class CLModel;
	class ServiceDiscoveryWidget;
	class UnreadQueueManager;
	class ChatStyleOptionManager;

	class Core : public QObject
	{
		Q_OBJECT
		Q_ENUMS (CLRoles CLEntryType CLEntryActionArea)

		ICoreProxy_ptr Proxy_;
		QList<ANFieldData> ANFields_;

		QRegExp ImageRegexp_;

#ifdef ENABLE_CRYPT
		boost::scoped_ptr<QCA::Initializer> QCAInit_;
		boost::scoped_ptr<QCA::KeyStoreManager> KeyStoreMgr_;
		boost::scoped_ptr<QCA::EventHandler> QCAEventHandler_;
		QMap<QString, QString> StoredPublicKeys_;
#endif

		QObjectList ProtocolPlugins_;
		QList<QAction*> AccountCreatorActions_;

		CLModel *CLModel_;
		ChatTabsManager *ChatTabsManager_;

		typedef QHash<QString, QStandardItem*> Category2Item_t;
		typedef QHash<QStandardItem*, Category2Item_t> Account2Category2Item_t;
		Account2Category2Item_t Account2Category2Item_;

		QHash<IAccount*, QDateTime> LastAccountStatusChange_;

		QHash<IAccount*, EntryStatus> SavedStatus_;

		typedef QHash<ICLEntry*, QList<QStandardItem*>> Entry2Items_t;
		Entry2Items_t Entry2Items_;

		ActionsManager *ActionsManager_;

		typedef QHash<QString, QObject*> ID2Entry_t;
		ID2Entry_t ID2Entry_;

		typedef QHash<ICLEntry*, QMap<QString, QIcon>> EntryClientIconCache_t;
		EntryClientIconCache_t EntryClientIconCache_;

		typedef QHash<ICLEntry*, QImage> Entry2SmoothAvatarCache_t;
		Entry2SmoothAvatarCache_t Entry2SmoothAvatarCache_;

		AnimatedIconManager<QStandardItem*> *ItemIconManager_;

		QMap<State, int> StateCounter_;
	public:
		enum ResourceLoaderType
		{
			RLTStatusIconLoader,
			RLTClientIconLoader,
			RLTAffIconLoader,
			RLTSystemIconLoader,
			RLTActivityIconLoader,
			RLTMoodIconLoader
		};
	private:
		QMap<ResourceLoaderType, std::shared_ptr<Util::ResourceLoader>> ResourceLoaders_;
		std::shared_ptr<SourceTrackingModel<IEmoticonResourceSource>> SmilesOptionsModel_;
		std::shared_ptr<SourceTrackingModel<IChatStyleResourceSource>> ChatStylesOptionsModel_;

		std::shared_ptr<PluginManager> PluginManager_;
		std::shared_ptr<ProxyObject> PluginProxyObject_;
		std::shared_ptr<TransferJobManager> XferJobManager_;
		std::shared_ptr<CallManager> CallManager_;
		std::shared_ptr<EventsNotifier> EventsNotifier_;
		std::shared_ptr<ImportManager> ImportManager_;
		std::shared_ptr<UnreadQueueManager> UnreadQueueManager_;
		QMap<QByteArray, std::shared_ptr<ChatStyleOptionManager>> StyleOptionManagers_;
		std::shared_ptr<Util::ShortcutManager> ShortcutManager_;

		Core ();
	public:
		enum CLRoles
		{
			CLRAccountObject = Qt::UserRole + 1,
			CLREntryObject,
			CLREntryType,
			CLREntryCategory,
			CLRUnreadMsgCount,
			CLRRole,
			CLRAffiliation,
			CLRNumOnline,
			CLRIsMUCCategory
		};

		enum CLEntryType
		{
			/** Self account.
				*/
			CLETAccount,
			/** Category (under self account).
				*/
			CLETCategory,
			/** Remote contact.
				*/
			CLETContact
		};

		static Core& Instance ();
		void Release ();

		void SetProxy (ICoreProxy_ptr);
		ICoreProxy_ptr GetProxy () const;

		QList<ANFieldData> GetANFields () const;

		Util::ResourceLoader* GetResourceLoader (ResourceLoaderType) const;
		QAbstractItemModel* GetSmilesOptionsModel () const;
		IEmoticonResourceSource* GetCurrentEmoSource () const;
		ChatStyleOptionManager* GetChatStylesOptionsManager (const QByteArray&) const;
		Util::ShortcutManager* GetShortcutManager () const;

		QSet<QByteArray> GetExpectedPluginClasses () const;
		void AddPlugin (QObject*);
		void RegisterHookable (QObject*);

		bool CouldHandle (const Entity&) const;
		void Handle (Entity);

		bool CouldHandleURL (const QUrl&) const;
		void HandleURL (const QUrl&, ICLEntry* = 0);

		const QObjectList& GetProtocolPlugins () const;

		QAbstractItemModel* GetCLModel () const;
		ChatTabsManager* GetChatTabsManager () const;
		QList<IAccount*> GetAccounts (std::function<bool (IProtocol*)> = [] (IProtocol*) { return true; }) const;
		QList<IProtocol*> GetProtocols () const;
		IAccount* GetAccount (const QByteArray&) const;

#ifdef ENABLE_CRYPT
		QList<QCA::PGPKey> GetPublicKeys () const;
		QList<QCA::PGPKey> GetPrivateKeys () const;

		void AssociatePrivateKey (IAccount*, const QCA::PGPKey&) const;
#endif

		/** Returns the list of all groups of all chat entries.
		 */
		QStringList GetChatGroups () const;

		void SendEntity (const Entity&);

		/** Returns contact list entry with the given id. The id is the
		 * same as returned by ICLEntry::GetEntryID(). If no such entry
		 * entry could be found, NULL is returned.
		 */
		QObject* GetEntry (const QString& id) const;

		/** Opens chat with the remote contact identified by index
		 * (which is from GetCLModel() model). If the index identifies
		 * account or category, this function does nothing.
		 */
		void OpenChat (const QModelIndex& index);

		TransferJobManager* GetTransferJobManager () const;

		CallManager* GetCallManager () const;

		/** Whether the given from the given entry should be counted as
		 * unread message. For example, messages in currently visible
		 * chat session or status messages shouldn't be counted as
		 * unread.
		 */
		bool ShouldCountUnread (const ICLEntry *entry,
				IMessage *message);

		/** Whether this message should be considered as a the one that
		 * highlights the participant.
		 */
		bool IsHighlightMessage (IMessage*);

		/** Returns the name of the icon from the current iconset for
		 * the given contact list entry state.
		 */
		Util::QIODevice_ptr GetIconPathForState (State state) const;

		/** Returns an icon from the current iconset for the given
		 * contact list entry state.
		 */
		QIcon GetIconForState (State state) const;

		/** Returns an icon from the current iconset for the given
		 * affiliation.
		 */
		QIcon GetAffIcon (const QByteArray& affName) const;

		/** @brief Returns icons for the given CL entry.
		 *
		 * This function returns an icon for each variant of the entry,
		 * since different variants may have different clients. If the
		 * protocol which the entry belongs doesn't support variants,
		 * the map would have only one key/value pair of null QString
		 * and corresponding icon.
		 *
		 * This function returns the icons from the currently selected
		 * (in settings) iconset.
		 *
		 * @param[in] entry Entry for which to return the icons.
		 * @return Map from entity variant to corresponding
		 * client icon.
		 */
		QMap<QString, QIcon> GetClientIconForEntry (ICLEntry *entry);

		/** @brief Returns the avatar for the given CL entry scaled to
		 * the given size.
		 *
		 * The scale is performed using SmoothTransform and keeping the
		 * aspect ratio.
		 *
		 * @param[in] entry Entry for which to get the avatar.
		 * @return Entry's avatar scaled to the given size.
		 */
		QImage GetAvatar (ICLEntry *entry, int size);
		QImage GetDefaultAvatar (int size) const;

		ActionsManager* GetActionsManager () const;

		QString GetSelectedChatTemplate (QObject *entry, QWebFrame *frame) const;
		QUrl GetSelectedChatTemplateURL (QObject*) const;

		bool AppendMessageByTemplate (QWebFrame*, QObject*, const ChatMsgAppendInfo&);

		void FrameFocused (QObject*, QWebFrame*);

		// Theming stuff
		QList<QColor> GenerateColors (const QString& coloringScheme) const;

		QString GetNickColor (const QString& nick, const QList<QColor>& colors) const;

		QString FormatDate (QDateTime, IMessage*);
		QString FormatNickname (QString, IMessage*, const QString& color);
		QString FormatBody (QString body, IMessage *msg);
		QString HandleSmiles (QString body);

		/** This function increases the number of unread messages by
		 * the given amount, which may be negative.
		 */
		void IncreaseUnreadCount (ICLEntry *entry, int amount = 1);
	private:
		/** Adds the protocol object. The object must implement
		 * IProtocolPlugin interface.
		 *
		 * Creates an entry in the contact list for accounts from the
		 * protocol plugin and creates the actions for adding a new
		 * account in this protocol or joining groupchats.
		 */
		void AddProtocolPlugin (QObject *object);

		/** Adds the resource source object. Currently only smile
		 * resources are supported.
		 */
		void AddResourceSourcePlugin (QObject *object);
		void AddSmileResourceSource (IEmoticonResourceSource*);
		void AddChatStyleResourceSource (IChatStyleResourceSource*);

		/** Adds the given contact list entry to the given account and
		 * performs common initialization tasks.
		 */
		void AddCLEntry (ICLEntry *entry, QStandardItem *accItem);

		/** Returns the list of category items for the given account and
		 * categories list. Creates the items if needed. The returned
		 * items are children of account item.
		 *
		 * Categories could be, for example, tags/groups in XMPP client
		 * and such.
		 */
		QList<QStandardItem*> GetCategoriesItems (QStringList categories, QStandardItem *account);

		/** Returns the QStandardItem for the given account.
		 */
		QStandardItem* GetAccountItem (const QObject *accountObj);

		/** Returns the QStandardItem for the given account and adds it
		 * into accountItemCache.
		 */
		QStandardItem* GetAccountItem (const QObject *accountObj,
				QMap<const QObject*, QStandardItem*>& accountItemCache);

		/** Creates the tooltip text for the roster entry to be shown in
		 * the tree.
		 */
		QString MakeTooltipString (ICLEntry *entry) const;

		void RebuildTooltip (ICLEntry *entry);

		Entity BuildStatusNotification (const EntryStatus&,
				ICLEntry*, const QString&);

		/** Handles the event of status changes in a contact list entry.
		 */
		void HandleStatusChanged (const EntryStatus& status,
				ICLEntry *entry, const QString& variant, bool asSignal = false, bool rebuildTooltip = true);

		/** Checks whether icon representing incoming file should be
		 * drawn for the entry with the given id.
		 */
		void CheckFileIcon (const QString& id);

		/** This functions calculates new value of number of unread
		 * items for the chain of parents of the given item.
		 */
		void RecalculateUnreadForParents (QStandardItem*);

		void RecalculateOnlineForCat (QStandardItem*);

		void NotifyWithReason (QObject*, const QString&,
				const char*, const QString&,
				const QString&, const QString&);

		void HandlePowerNotification (Entity);

		/** Removes one item representing the given CL entry.
		 */
		void RemoveCLItem (QStandardItem*);

		/** Adds the given entry to the given category item.
		 */
		void AddEntryTo (ICLEntry*, QStandardItem*);

		void SuggestJoiningMUC (IAccount*, const QVariantMap&);

		IChatStyleResourceSource* GetCurrentChatStyle (QObject*) const;

		void FillANFields ();

		void UpdateInitState (State);

#ifdef ENABLE_CRYPT
		void RestoreKeyForAccount (IAccount*);
		void RestoreKeyForEntry (ICLEntry*);
#endif
	public slots:
		/** Initiates MUC join by calling the corresponding protocol
		 * plugin's IProtocol::InitiateMUCJoin() function.
		 */
		void handleMucJoinRequested ();

		void handleShowNextUnread ();

		void saveAccountVisibility (IAccount*);
	private slots:
		void handleNewProtocols (const QList<QObject*>&);

		/** Handles a new account. This account may be both a new one
		 * (added as a result of user's actions) and already existing
		 * one (in case it was just read from settings, for example).
		 *
		 * account is expected to implement IAccount interface.
		 */
		void addAccount (QObject *account);

		/** Handles account removal. Basically, just removes it and its
		 * children from the contact list.
		 *
		 * account is expected to implement IAccount interface.
		 */
		void handleAccountRemoved (QObject *account);

		/** Handles newly added contact list items. Each item is
		 * expected to implement ICLEntry. This slot appends the items
		 * to already existing ones, so only really new ones (during the
		 * session lifetime) should be in the items list.
		 */
		void handleGotCLItems (const QList<QObject*>& items);

		/** Handles removal of items previously added to the contact
		 * list. Each item is expected to implement the ICLEntry
		 * interface.
		 *
		 * This slot removes the model items corresponding to the items
		 * removed and also removes those categories that became empty
		 * because of items removal, if any.
		 */
		void handleRemovedCLItems (const QList<QObject*>& items);

		/** Handles the status change of an account to new status.
		 */
		void handleAccountStatusChanged (const EntryStatus& status);

		void handleAccountRenamed (const QString&);

		/** Handles the status change of a CL entry to new status.
		 */
		void handleStatusChanged (const EntryStatus& status, const QString& variant);

		/** Handles ICLEntry's PEP-like (XEP-0163) event from the given
		 * variant.
		 */
		void handleEntryPEPEvent (const QString& variant);

		/** Handles the event of name changes in plugin.
		 */
		void handleEntryNameChanged (const QString& newName);

		/** Handles the event of groups change in plugin.
		 *
		 * If obj is null, the sender() is used, otherwise obj is used.
		 */
		void handleEntryGroupsChanged (QStringList, QObject *obj = 0);

		/** Handles the event of permissions change in entry from plugin.
		 *
		 * If the passed entry is not NULL, it will be used, otherwise
		 * sender() will be used.
		 */
		void handleEntryPermsChanged (ICLEntry *entry = 0, bool rebuildTooltip = true);

		void remakeTooltipForSender ();

		/** Handles the message receival from contact list entries.
		 */
		void handleEntryGotMessage (QObject *msg);

		/** Handles the authorization requests from accounts.
		 */
		void handleAuthorizationRequested (QObject*, const QString&);

		/** Handles the IAdvancedCLEntry::attentionDrawn().
		 */
		void handleAttentionDrawn (const QString&, const QString&);

		/** Handles nick conflict.
		 */
		void handleNicknameConflict (const QString&);

		/** Handles kicks.
		 */
		void handleBeenKicked (const QString&);

		/** Handles bans.
		 */
		void handleBeenBanned (const QString&);

		void handleItemSubscribed (QObject*, const QString&);
		void handleItemUnsubscribed (QObject*, const QString&);
		void handleItemUnsubscribed (const QString&, const QString&);
		void handleItemCancelledSubscription (QObject*, const QString&);
		void handleItemGrantedSubscription (QObject*, const QString&);

		void handleMUCInvitation (const QVariantMap&, const QString&, const QString&);

		/** Is registered in the XmlSettingsManager as handler for
		 * changes of the "StatusIcons" property.
		 */
		void updateStatusIconset ();

		/** Is registered in the XmlSettingsManager as handler for
		 * changes of the "GroupContacts" property.
		 */
		void handleGroupContactsChanged ();

		/** This slot is used to update the model item which is
		 * corresponding to the sender() which is expected to be a
		 * ICLEntry.
		 */
		void updateItem ();

		/** Asks the corresponding CL entry to show its dialog with
		 * information about the user.
		 */
		void showVCard ();

		/** Handles the number of unread messages for the given contact
		 * list entry identified by object. Object should implement
		 * ICLEntry, obviously.
		 */
		void handleClearUnreadMsgCount (QObject *object);

		void handleGotSDSession (QObject*);

		void handleFileOffered (QObject*);
		void handleJobDeoffered (QObject*);

		void handleRIEXItemsSuggested (QList<LeechCraft::Azoth::RIEXItem>, QObject*, QString);

		/** Removes the entries in the client icon cache for the sender,
		 * if obj is null, or for obj, if it is not null.
		 *
		 * If the object can't be casted to ICLEntry, this function does
		 * nothing.
		 */
		void invalidateClientsIconCache (QObject *obj = 0);
		void invalidateClientsIconCache (ICLEntry*);

		void invalidateSmoothAvatarCache ();

		void flushIconCaches ();

#ifdef ENABLE_CRYPT
		void handleQCAEvent (int, const QCA::Event&);
		void handleQCABusyFinished ();
#endif
	signals:
		void gotEntity (const LeechCraft::Entity&);
		void delegateEntity (const LeechCraft::Entity&, int*, QObject**);
		void topStatusChanged (LeechCraft::Azoth::State);

		/** Convenient signal for rethrowing the event of an account
		 * being added.
		 */
		void accountAdded (IAccount*);

		/** Convenient signal for rethrowing the event of an account
		 * being removed.
		 */
		void accountRemoved (IAccount*);

		void gotSDWidget (ServiceDiscoveryWidget*);

		// Plugin API
		void hookAddingCLEntryBegin (LeechCraft::IHookProxy_ptr proxy,
				QObject *entry);
		void hookAddingCLEntryEnd (LeechCraft::IHookProxy_ptr proxy,
				QObject *entry);
		void hookEntryStatusChanged (LeechCraft::IHookProxy_ptr proxy,
				QObject *entry,
				QString variant);
		void hookFormatDateTime (LeechCraft::IHookProxy_ptr proxy,
				QObject *chatTab,
				QDateTime dateTime,
				QObject *message);
		void hookFormatNickname (LeechCraft::IHookProxy_ptr proxy,
				QObject *chatTab,
				QString nick,
				QObject *message);
		void hookFormatBodyBegin (LeechCraft::IHookProxy_ptr proxy,
				QObject *message);
		void hookFormatBodyEnd (LeechCraft::IHookProxy_ptr proxy,
				QObject *message);
		void hookGonnaHandleSmiles (LeechCraft::IHookProxy_ptr proxy,
				QString body,
				QString pack);
		void hookGotAuthRequest (LeechCraft::IHookProxy_ptr proxy,
				QObject *entry,
				QString msg);
		void hookGotMessage (LeechCraft::IHookProxy_ptr proxy,
				QObject *message);
		void hookGotMessage2 (LeechCraft::IHookProxy_ptr proxy,
				QObject *message);
		void hookIsHighlightMessage (LeechCraft::IHookProxy_ptr proxy,
				QObject *message);
		void hookShouldCountUnread (LeechCraft::IHookProxy_ptr proxy,
				QObject *message);
		void hookTooltipBeforeVariants (LeechCraft::IHookProxy_ptr proxy,
				QObject *entry) const;
	};
}
}

Q_DECLARE_METATYPE (LeechCraft::Azoth::Core::CLEntryType);
Q_DECLARE_METATYPE (LeechCraft::Azoth::ICLEntry*);

#endif
