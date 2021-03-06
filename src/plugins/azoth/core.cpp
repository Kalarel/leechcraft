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

#include "core.h"
#include <cmath>
#include <QIcon>
#include <QAction>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QDir>
#include <QMenu>
#include <QMetaMethod>
#include <QInputDialog>
#include <QMainWindow>
#include <QStringListModel>
#include <QMessageBox>
#include <QClipboard>
#include <QtDebug>
#include <util/resourceloader.h>
#include <util/util.h>
#include <util/defaulthookproxy.h>
#include <util/tags/categoryselector.h>
#include <util/notificationactionhandler.h>
#include <util/shortcuts/shortcutmanager.h>
#include <interfaces/iplugin2.h>
#include <interfaces/core/icoreproxy.h>
#include "interfaces/azoth/iprotocolplugin.h"
#include "interfaces/azoth/iprotocol.h"
#include "interfaces/azoth/iaccount.h"
#include "interfaces/azoth/iclentry.h"
#include "interfaces/azoth/iadvancedclentry.h"
#include "interfaces/azoth/imucentry.h"
#include "interfaces/azoth/imucperms.h"
#include "interfaces/azoth/iauthable.h"
#include "interfaces/azoth/iresourceplugin.h"
#include "interfaces/azoth/iurihandler.h"
#include "interfaces/azoth/irichtextmessage.h"
#include "interfaces/azoth/ihaveservicediscovery.h"
#include "interfaces/azoth/iextselfinfoaccount.h"
#ifdef ENABLE_CRYPT
#include "interfaces/azoth/isupportpgp.h"
#endif
#include "chattabsmanager.h"
#include "pluginmanager.h"
#include "proxyobject.h"
#include "xmlsettingsmanager.h"
#include "joinconferencedialog.h"
#include "transferjobmanager.h"
#include "accounthandlerchooserdialog.h"
#include "util.h"
#include "eventsnotifier.h"
#include "activitydialog.h"
#include "mooddialog.h"
#include "callmanager.h"
#include "addcontactdialog.h"
#include "clmodel.h"
#include "actionsmanager.h"
#include "servicediscoverywidget.h"
#include "importmanager.h"
#include "unreadqueuemanager.h"
#include "chatstyleoptionmanager.h"
#include "riexhandler.h"

Q_DECLARE_METATYPE (QList<QColor>);

namespace LeechCraft
{
namespace Azoth
{
	QDataStream& operator<< (QDataStream& out, const EntryStatus& status)
	{
		quint8 version = 1;
		out << version
			<< static_cast<quint8> (status.State_)
			<< status.StatusString_;
		return out;
	}

	QDataStream& operator>> (QDataStream& in, EntryStatus& status)
	{
		quint8 version = 0;
		in >> version;
		if (version != 1)
		{
			qWarning () << Q_FUNC_INFO
					<< "unknown version"
					<< version;
			return in;
		}

		quint8 state;
		in >> state
			>> status.StatusString_;
		status.State_ = static_cast<State> (state);
		return in;
	}

	namespace
	{
		QByteArray GetStyleOptName (QObject *entry)
		{
			if (XmlSettingsManager::Instance ().property ("CustomMUCStyle").toBool () &&
					qobject_cast<IMUCEntry*> (entry))
				return "MUCWindowStyle";
			else
				return "ChatWindowStyle";
		}

		class ModelUpdateSafeguard
		{
			QAbstractItemModel *Model_;
			const bool Recursive_;
			const bool BlockEnabled_;
		public:
			ModelUpdateSafeguard (QAbstractItemModel *model)
			: Model_ (model)
			, Recursive_ (Model_->signalsBlocked ())
			, BlockEnabled_ (!XmlSettingsManager::Instance ().property ("OptimizedTreeRebuild").toBool ())
			{
				if (!Recursive_ && BlockEnabled_)
				{
					QMetaObject::invokeMethod (Model_, "modelAboutToBeReset");
					Model_->blockSignals (true);
				}
			}

			ModelUpdateSafeguard (const ModelUpdateSafeguard&) = delete;
			ModelUpdateSafeguard& operator= (const ModelUpdateSafeguard&) = delete;

			~ModelUpdateSafeguard ()
			{
				if (!Recursive_ && BlockEnabled_)
				{
					Model_->blockSignals (false);
					QMetaObject::invokeMethod (Model_, "modelReset");
				}
			}
		};
	}

	QList<IAccount*> GetAccountsPred (const QObjectList& protocols,
			std::function<bool (IProtocol*)> protoPred = [] (IProtocol*) { return true; })
	{
		QList<IAccount*> accounts;
		Q_FOREACH (QObject *protoPlugin, protocols)
		{
			QObjectList protocols =
					qobject_cast<IProtocolPlugin*> (protoPlugin)->GetProtocols ();
			Q_FOREACH (QObject *protoObj, protocols)
			{
				IProtocol *proto = qobject_cast<IProtocol*> (protoObj);
				if (!protoPred (proto))
					continue;

				QObjectList accountObjs = proto->GetRegisteredAccounts ();
				Q_FOREACH (QObject *accountObj, accountObjs)
					accounts << qobject_cast<IAccount*> (accountObj);
			}
		}
		return accounts;
	}

	Core::Core ()
	: ImageRegexp_ ("(\\b(?:data:image/)[\\w\\d/\\?.=:@&%#_;\\(?:\\)\\+\\-\\~\\*\\,]+)",
			Qt::CaseInsensitive, QRegExp::RegExp2)
#ifdef ENABLE_CRYPT
	, QCAInit_ (new QCA::Initializer)
	, KeyStoreMgr_ (new QCA::KeyStoreManager)
	, QCAEventHandler_ (new QCA::EventHandler)
#endif
	, CLModel_ (new CLModel (this))
	, ChatTabsManager_ (new ChatTabsManager (this))
	, ActionsManager_ (new ActionsManager (this))
	, ItemIconManager_ (new AnimatedIconManager<QStandardItem*> ([] (QStandardItem *it, const QIcon& ic)
						{ it->setIcon (ic); }))
	, SmilesOptionsModel_ (new SourceTrackingModel<IEmoticonResourceSource> (QStringList (tr ("Smile pack"))))
	, ChatStylesOptionsModel_ (new SourceTrackingModel<IChatStyleResourceSource> (QStringList (tr ("Chat style"))))
	, PluginManager_ (new PluginManager)
	, PluginProxyObject_ (new ProxyObject)
	, XferJobManager_ (new TransferJobManager)
	, CallManager_ (new CallManager)
	, EventsNotifier_ (new EventsNotifier)
	, ImportManager_ (new ImportManager)
	, UnreadQueueManager_ (new UnreadQueueManager)
	{
		FillANFields ();

		auto addSOM = [this] (const QByteArray& option)
		{
			StyleOptionManagers_ [option].reset (new ChatStyleOptionManager (option, this));
		};
		addSOM ("ChatWindowStyle");
		addSOM ("MUCWindowStyle");

#ifdef ENABLE_CRYPT
		connect (QCAEventHandler_.get (),
				SIGNAL (eventReady (int, const QCA::Event&)),
				this,
				SLOT (handleQCAEvent (int, const QCA::Event&)));
		if (KeyStoreMgr_->isBusy ())
			connect (KeyStoreMgr_.get (),
					SIGNAL (busyFinished ()),
					this,
					SLOT (handleQCABusyFinished ()),
					Qt::QueuedConnection);
		QCAEventHandler_->start ();
		KeyStoreMgr_->start ();

		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth");
		settings.beginGroup ("PublicEntryKeys");
		Q_FOREACH (const QString& entryId, settings.childKeys ())
			StoredPublicKeys_ [entryId] = settings.value (entryId).toString ();
		settings.endGroup ();
#endif
		ResourceLoaders_ [RLTStatusIconLoader].reset (new Util::ResourceLoader ("azoth/iconsets/contactlist/", this));
		ResourceLoaders_ [RLTClientIconLoader].reset (new Util::ResourceLoader ("azoth/iconsets/clients/", this));
		ResourceLoaders_ [RLTAffIconLoader].reset (new Util::ResourceLoader ("azoth/iconsets/affiliations/", this));
		ResourceLoaders_ [RLTSystemIconLoader].reset (new Util::ResourceLoader ("azoth/iconsets/system/", this));
		ResourceLoaders_ [RLTActivityIconLoader].reset (new Util::ResourceLoader ("azoth/iconsets/activities/", this));
		ResourceLoaders_ [RLTMoodIconLoader].reset (new Util::ResourceLoader ("azoth/iconsets/moods/", this));

		Q_FOREACH (std::shared_ptr<Util::ResourceLoader> rl, ResourceLoaders_.values ())
		{
			rl->AddLocalPrefix ();
			rl->AddGlobalPrefix ();

			rl->SetCacheParams (1000, 0);
		}

		connect (ChatTabsManager_,
				SIGNAL (clearUnreadMsgCount (QObject*)),
				this,
				SLOT (handleClearUnreadMsgCount (QObject*)));
		connect (this,
				SIGNAL (hookAddingCLEntryEnd (LeechCraft::IHookProxy_ptr, QObject*)),
				ChatTabsManager_,
				SLOT (handleAddingCLEntryEnd (LeechCraft::IHookProxy_ptr, QObject*)));
		connect (XferJobManager_.get (),
				SIGNAL (jobNoLongerOffered (QObject*)),
				this,
				SLOT (handleJobDeoffered (QObject*)));
		connect (EventsNotifier_.get (),
				SIGNAL (gotEntity (const LeechCraft::Entity&)),
				this,
				SIGNAL (gotEntity (const LeechCraft::Entity&)));
		connect (ChatTabsManager_,
				SIGNAL (entryMadeCurrent (QObject*)),
				EventsNotifier_.get (),
				SLOT (handleEntryMadeCurrent (QObject*)));
		connect (ChatTabsManager_,
				SIGNAL (entryMadeCurrent (QObject*)),
				UnreadQueueManager_.get (),
				SLOT (clearMessagesForEntry (QObject*)));

		PluginManager_->RegisterHookable (this);
		PluginManager_->RegisterHookable (CLModel_);
		PluginManager_->RegisterHookable (ActionsManager_);

		SmilesOptionsModel_->AddModel (new QStringListModel (QStringList (QString ())));

		qRegisterMetaType<IMessage*> ("LeechCraft::Azoth::IMessage*");
		qRegisterMetaType<IMessage*> ("IMessage*");

		XmlSettingsManager::Instance ().RegisterObject ("StatusIcons",
				this, "updateStatusIconset");
		XmlSettingsManager::Instance ().RegisterObject ("GroupContacts",
				this, "handleGroupContactsChanged");
	}

	Core& Core::Instance ()
	{
		static Core c;
		return c;
	}

	void Core::Release ()
	{
		ResourceLoaders_.clear ();
		ShortcutManager_.reset ();
		StyleOptionManagers_.clear ();

#ifdef ENABLE_CRYPT
		QCAEventHandler_.reset ();
		KeyStoreMgr_.reset ();
		QCAInit_.reset ();
#endif
	}

	void Core::SetProxy (ICoreProxy_ptr proxy)
	{
		Proxy_ = proxy;
		ShortcutManager_.reset (new Util::ShortcutManager (proxy));
	}

	ICoreProxy_ptr Core::GetProxy () const
	{
		return Proxy_;
	}

	QList<ANFieldData> Core::GetANFields () const
	{
		return ANFields_;
	}

	Util::ResourceLoader* Core::GetResourceLoader (Core::ResourceLoaderType type) const
	{
		return ResourceLoaders_ [type].get ();
	}

	QAbstractItemModel* Core::GetSmilesOptionsModel () const
	{
		return SmilesOptionsModel_.get ();
	}

	IEmoticonResourceSource* Core::GetCurrentEmoSource () const
	{
		const QString& pack = XmlSettingsManager::Instance ()
				.property ("SmileIcons").toString ();
		return SmilesOptionsModel_->GetSourceForOption (pack);
	}

	ChatStyleOptionManager* Core::GetChatStylesOptionsManager (const QByteArray& name) const
	{
		return StyleOptionManagers_ [name].get ();
	}

	Util::ShortcutManager* Core::GetShortcutManager () const
	{
		return ShortcutManager_.get ();
	}

	QSet<QByteArray> Core::GetExpectedPluginClasses () const
	{
		QSet<QByteArray> classes;
		classes << "org.LeechCraft.Plugins.Azoth.Plugins.IGeneralPlugin";
		classes << "org.LeechCraft.Plugins.Azoth.Plugins.IProtocolPlugin";
		classes << "org.LeechCraft.Plugins.Azoth.Plugins.IResourceSourcePlugin";
		return classes;
	}

	void Core::AddPlugin (QObject *plugin)
	{
		IPlugin2 *plugin2 = qobject_cast<IPlugin2*> (plugin);
		if (!plugin2)
		{
			qWarning () << Q_FUNC_INFO
					<< plugin
					<< "isn't a IPlugin2";
			return;
		}

		QByteArray sig = QMetaObject::normalizedSignature ("initPlugin (QObject*)");
		if (plugin->metaObject ()->indexOfMethod (sig) != -1)
			QMetaObject::invokeMethod (plugin,
					"initPlugin",
					Q_ARG (QObject*, PluginProxyObject_.get ()));

		PluginManager_->AddPlugin (plugin);

		QSet<QByteArray> classes = plugin2->GetPluginClasses ();
		if (classes.contains ("org.LeechCraft.Plugins.Azoth.Plugins.IProtocolPlugin"))
			AddProtocolPlugin (plugin);

		if (classes.contains ("org.LeechCraft.Plugins.Azoth.Plugins.IResourceSourcePlugin"))
			AddResourceSourcePlugin (plugin);
	}

	void Core::RegisterHookable (QObject *object)
	{
		PluginManager_->RegisterHookable (object);
	}

	bool Core::CouldHandle (const Entity& e) const
	{
		if (e.Mime_ == "x-leechcraft/power-state-changed" ||
				e.Mime_ == "x-leechcraft/im-account-import" ||
				e.Mime_ == "x-leechcraft/im-history-import")
			return true;

		if (!e.Entity_.canConvert<QUrl> ())
			return false;

		const QUrl& url = e.Entity_.toUrl ();
		if (!url.isValid ())
			return false;

		return CouldHandleURL (url);
	}

	void Core::Handle (Entity e)
	{
		if (e.Mime_ == "x-leechcraft/power-state-changed")
		{
			HandlePowerNotification (e);
			return;
		}
		else if (e.Mime_ == "x-leechcraft/im-account-import")
		{
			ImportManager_->HandleAccountImport (e);
			return;
		}
		else if (e.Mime_ == "x-leechcraft/im-history-import")
		{
			ImportManager_->HandleHistoryImport (e);
			return;
		}

		const QUrl& url = e.Entity_.toUrl ();
		if (!url.isValid ())
			return;

		HandleURL (url);
	}

	bool Core::CouldHandleURL (const QUrl& url) const
	{
		Q_FOREACH (QObject *obj, ProtocolPlugins_)
		{
			IProtocolPlugin *protoPlug = qobject_cast<IProtocolPlugin*> (obj);
			if (!protoPlug)
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cast"
						<< obj
						<< "to IProtocolPlugin";
				continue;
			}

			Q_FOREACH (QObject *protoObj, protoPlug->GetProtocols ())
			{
				IURIHandler *handler = qobject_cast<IURIHandler*> (protoObj);
				if (!handler)
					continue;
				if (handler->SupportsURI (url))
					return true;
			}
		}

		return false;
	}

	void Core::HandleURL (const QUrl& url, ICLEntry *source)
	{
		QList<QObject*> accounts;
		Q_FOREACH (QObject *obj, ProtocolPlugins_)
		{
			IProtocolPlugin *protoPlug = qobject_cast<IProtocolPlugin*> (obj);
			if (!protoPlug)
			{
				qWarning () << Q_FUNC_INFO
						<< "unable to cast"
						<< obj
						<< "to IProtocolPlugin";
				continue;
			}

			Q_FOREACH (QObject *protoObj, protoPlug->GetProtocols ())
			{
				IURIHandler *handler = qobject_cast<IURIHandler*> (protoObj);
				if (!handler)
					continue;
				if (!handler->SupportsURI (url))
					continue;

				IProtocol *proto = qobject_cast<IProtocol*> (protoObj);
				if (!proto)
				{
					qWarning () << Q_FUNC_INFO
							<< protoObj
							<< "doesn't implement IProtocol";
					continue;
				}
				accounts << proto->GetRegisteredAccounts ();
			}
		}

		if (accounts.isEmpty ())
			return;

		if (source && accounts.contains (source->GetParentAccount ()))
		{
			accounts.clear ();
			accounts << source->GetParentAccount ();
		}

		QObject *selected = 0;

		if (accounts.size () > 1)
		{
			std::auto_ptr<AccountHandlerChooserDialog> dia (new AccountHandlerChooserDialog (accounts,
						tr ("Please select account to handle URI %1")
							.arg (url.toString ())));
			if (dia->exec () != QDialog::Accepted)
				return;

			selected = dia->GetSelectedAccount ();
		}
		else
			selected = accounts.at (0);

		if (!selected)
			return;

		QObject *selProto = qobject_cast<IAccount*> (selected)->GetParentProtocol ();
		qobject_cast<IURIHandler*> (selProto)->HandleURI (url, selected);
	}

	const QObjectList& Core::GetProtocolPlugins () const
	{
		return ProtocolPlugins_;
	}

	QAbstractItemModel* Core::GetCLModel () const
	{
		return CLModel_;
	}

	ChatTabsManager* Core::GetChatTabsManager () const
	{
		return ChatTabsManager_;
	}

	QList<IAccount*> Core::GetAccounts (std::function<bool (IProtocol*)> protoPred) const
	{
		return GetAccountsPred (ProtocolPlugins_, protoPred);
	}

	QList<IProtocol*> Core::GetProtocols () const
	{
		QList<IProtocol*> result;
		Q_FOREACH (QObject *protoPlugin, ProtocolPlugins_)
		{
			QObjectList protos = qobject_cast<IProtocolPlugin*> (protoPlugin)->GetProtocols ();
			Q_FOREACH (QObject *obj, protos)
				result << qobject_cast<IProtocol*> (obj);
		}
		result.removeAll (0);
		return result;
	}

	IAccount* Core::GetAccount (const QByteArray& id) const
	{
		Q_FOREACH (IProtocol *proto, GetProtocols ())
			Q_FOREACH (QObject *accObj, proto->GetRegisteredAccounts ())
			{
				auto acc = qobject_cast<IAccount*> (accObj);
				if (acc && acc->GetAccountID () == id)
					return acc;
			}

		return 0;
	}

#ifdef ENABLE_CRYPT
	QList<QCA::PGPKey> Core::GetPublicKeys () const
	{
		QList<QCA::PGPKey> result;

		QCA::KeyStore store ("qca-gnupg", KeyStoreMgr_.get ());

		Q_FOREACH (const QCA::KeyStoreEntry& entry, store.entryList ())
		{
			const QCA::PGPKey& key = entry.pgpPublicKey ();
			if (!key.isNull ())
				result << key;
		}

		return result;
	}

	QList<QCA::PGPKey> Core::GetPrivateKeys () const
	{
		QList<QCA::PGPKey> result;

		QCA::KeyStore store ("qca-gnupg", KeyStoreMgr_.get ());

		Q_FOREACH (const QCA::KeyStoreEntry& entry, store.entryList ())
		{
			const QCA::PGPKey& key = entry.pgpSecretKey ();
			if (!key.isNull ())
				result << key;
		}

		return result;
	}

	void Core::AssociatePrivateKey (IAccount *acc, const QCA::PGPKey& key) const
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_Azoth");
		settings.beginGroup ("PrivateKeys");
		if (key.isNull ())
			settings.remove (acc->GetAccountID ());
		else
			settings.setValue (acc->GetAccountID (), key.keyId ());
		settings.endGroup ();
	}
#endif

	QStringList Core::GetChatGroups () const
	{
		QStringList result;
		Q_FOREACH (const ICLEntry *entry, Entry2Items_.keys ())
		{
			if (entry->GetEntryType () != ICLEntry::ETChat)
				continue;

			Q_FOREACH (const QString& group, entry->Groups ())
				if (!result.contains (group))
					result << group;
		}
		result.sort ();
		return result;
	}

	void Core::SendEntity (const LeechCraft::Entity& e)
	{
		emit gotEntity (e);
	}

	QObject* Core::GetEntry (const QString& id) const
	{
		return ID2Entry_.value (id);
	}

	void Core::OpenChat (const QModelIndex& contactIndex)
	{
		ChatTabsManager_->OpenChat (contactIndex);
	}

	TransferJobManager* Core::GetTransferJobManager () const
	{
		return XferJobManager_.get ();
	}

	CallManager* Core::GetCallManager () const
	{
		return CallManager_.get ();
	}

	bool Core::ShouldCountUnread (const ICLEntry *entry,
			IMessage *msg)
	{
		if (msg->GetObject ()->property ("Azoth/HiddenMessage").toBool ())
			return false;

		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookShouldCountUnread (proxy, msg->GetObject ());
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toBool ();

		return !ChatTabsManager_->IsActiveChat (entry) &&
				(msg->GetMessageType () == IMessage::MTChatMessage ||
				 msg->GetMessageType () == IMessage::MTMUCMessage);
	}

	bool Core::IsHighlightMessage (IMessage *msg)
	{
		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookIsHighlightMessage (proxy, msg->GetObject ());
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toBool ();

		IMUCEntry *mucEntry =
				qobject_cast<IMUCEntry*> (msg->ParentCLEntry ());
		if (!mucEntry)
			return false;

		return msg->GetBody ().contains (mucEntry->GetNick (), Qt::CaseInsensitive);
	}

	void Core::AddProtocolPlugin (QObject *plugin)
	{
		IProtocolPlugin *ipp =
			qobject_cast<IProtocolPlugin*> (plugin);
		if (!ipp)
			qWarning () << Q_FUNC_INFO
				<< "plugin"
				<< plugin
				<< "tells it implements the IProtocolPlugin but cast failed";
		else
		{
			ProtocolPlugins_ << plugin;

			handleNewProtocols (ipp->GetProtocols ());

			connect (plugin,
					SIGNAL (gotNewProtocols (QList<QObject*>)),
					this,
					SLOT (handleNewProtocols (QList<QObject*>)));
		}
	}

	void Core::AddResourceSourcePlugin (QObject *rp)
	{
		IResourcePlugin *irp = qobject_cast<IResourcePlugin*> (rp);
		if (!irp)
		{
			qWarning () << Q_FUNC_INFO
					<< rp
					<< "doesn't implement IResourcePlugin";
			return;
		}

		Q_FOREACH (QObject *object, irp->GetResourceSources ())
		{
			auto smileSrc = qobject_cast<IEmoticonResourceSource*> (object);
			if (smileSrc)
				AddSmileResourceSource (smileSrc);

			auto chatStyleSrc = qobject_cast<IChatStyleResourceSource*> (object);
			if (chatStyleSrc)
				AddChatStyleResourceSource (chatStyleSrc);
		}
	}

	void Core::AddSmileResourceSource (IEmoticonResourceSource *src)
	{
		SmilesOptionsModel_->AddSource (src);
	}

	void Core::AddChatStyleResourceSource (IChatStyleResourceSource *src)
	{
		ChatStylesOptionsModel_->AddSource (src);

		Q_FOREACH (auto manager, StyleOptionManagers_.values ())
			manager->AddChatStyleResourceSource (src);
	}

	QString Core::GetSelectedChatTemplate (QObject *entry, QWebFrame *frame) const
	{
		IChatStyleResourceSource *src = GetCurrentChatStyle (entry);
		if (!src)
			return QString ();

		const QByteArray& optName = GetStyleOptName (entry);
		const QString& opt = XmlSettingsManager::Instance ()
				.property (optName).toString ();
		const QString& var = XmlSettingsManager::Instance ()
				.property (optName + "Variant").toString ();
		return src->GetHTMLTemplate (opt, var, entry, frame);
	}

	QUrl Core::GetSelectedChatTemplateURL (QObject *entry) const
	{
		IChatStyleResourceSource *src = GetCurrentChatStyle (entry);
		if (!src)
			return QUrl ();

		const QString& opt = XmlSettingsManager::Instance ()
				.property (GetStyleOptName (entry)).toString ();
		return src->GetBaseURL (opt);
	}

	bool Core::AppendMessageByTemplate (QWebFrame *frame,
			QObject *message, const ChatMsgAppendInfo& info)
	{
		IChatStyleResourceSource *src = GetCurrentChatStyle (qobject_cast<IMessage*> (message)->ParentCLEntry ());
		if (!src)
		{
			qWarning () << Q_FUNC_INFO
					<< "empty result for"
					<< message;
			return false;
		}

		return src->AppendMessage (frame, message, info);
	}

	void Core::FrameFocused (QObject *entry, QWebFrame *frame)
	{
		IChatStyleResourceSource *src = GetCurrentChatStyle (entry);
		if (!src)
			return;

		src->FrameFocused (frame);
	}

	QList<QColor> Core::GenerateColors (const QString& coloring) const
	{
		auto fix = [] (qreal h) -> qreal
		{
			while (h < 0)
				h += 1;
			while (h >= 1)
				h -= 1;
			return h;
		};

		QList<QColor> result;
		if (XmlSettingsManager::Instance ().property ("OverrideHashColors").toBool ())
		{
			result = XmlSettingsManager::Instance ()
					.property ("OverrideColorsList").value<decltype (result)> ();
			if (!result.isEmpty ())
				return result;
		}

		if (coloring == "hash" ||
				coloring.isEmpty ())
		{
			const QColor& bg = QApplication::palette ().color (QPalette::Base);

			const qreal lower = 25. / 360.;
			const qreal delta = 50. / 360.;
			const qreal higher = 180. / 360. - delta / 2;

			const qreal alpha = bg.alphaF ();

			qreal h = bg.hueF ();

			QColor color;
			for (qreal d = lower; d <= higher; d += delta)
			{
				color.setHsvF (fix (h + d), 1, 0.6, alpha);
				result << color;
				color.setHsvF (fix (h - d), 1, 0.6, alpha);
				result << color;
				color.setHsvF (fix (h + d), 1, 0.9, alpha);
				result << color;
				color.setHsvF (fix (h - d), 1, 0.9, alpha);
				result << color;
			}
		}
		else
			Q_FOREACH (const QString& str,
					coloring.split (' ', QString::SkipEmptyParts))
				result << QColor (str);

		return result;
	}

	QString Core::GetNickColor (const QString& nick, const QList<QColor>& colors) const
	{
		if (colors.isEmpty ())
			return "green";

		int hash = 0;
		for (int i = 0; i < nick.length (); ++i)
		{
			const QChar& c = nick.at (i);
			hash += c.toLatin1 () ?
					c.toLatin1 () :
					c.unicode ();
			hash += nick.length ();
		}
		QColor nc = colors.at (std::abs (hash) % colors.size ());
		return nc.name ();
	}

	QString Core::FormatDate (QDateTime dt, IMessage *msg)
	{
		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookFormatDateTime (proxy, this, dt, msg->GetObject ());
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toString ();

		proxy->FillValue ("dateTime", dt);

		return dt.time ().toString ();
	}

	QString Core::FormatNickname (QString nick, IMessage *msg, const QString& color)
	{
		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookFormatNickname (proxy, this, nick, msg->GetObject ());
		if (proxy->IsCancelled ())
			return proxy->GetReturnValue ().toString ();

		proxy->FillValue ("nick", nick);

		QString string;

		if (msg->GetMessageType () == IMessage::MTMUCMessage)
		{
			QUrl url ("azoth://insertnick/");
			url.addEncodedQueryItem ("nick", QUrl::toPercentEncoding (nick));

			ICLEntry *other = qobject_cast<ICLEntry*> (msg->OtherPart ());
			if (other)
				url.addEncodedQueryItem ("entryId",
						QUrl::toPercentEncoding (other->GetEntryID ()));

			string.append ("<span class='nickname'><a href=\"");
			string.append (url.toEncoded ());
			string.append ("\" class='nicklink' style='text-decoration:none; color:");
			string.append (color);
			string.append ("'>");
			string.append (nick);
			string.append ("</a></span>");
		}
		else
			string = QString ("<span class='nickname'>%1</span>")
					.arg (nick);

		return string;
	}

	QString Core::FormatBody (QString body, IMessage *msg)
	{
		QObject *msgObj = msg->GetObject ();

		IRichTextMessage *rtMsg = qobject_cast<IRichTextMessage*> (msgObj);
		const bool isRich = rtMsg && rtMsg->GetRichBody () == body;

		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		proxy->SetValue ("body", body);
		emit hookFormatBodyBegin (proxy, msgObj);
		if (!proxy->IsCancelled ())
		{
			proxy->FillValue ("body", body);

			if (!isRich)
			{
				PluginProxyObject_->FormatLinks (body);
				body.replace ('\n', "<br />");
				body.replace ("  ", "&nbsp; ");
			}

			body = HandleSmiles (body);

			proxy.reset (new Util::DefaultHookProxy);
			proxy->SetValue ("body", body);
			emit hookFormatBodyEnd (proxy, msgObj);
			proxy->FillValue ("body", body);
		}

		return proxy->IsCancelled () ?
				proxy->GetReturnValue ().toString () :
				body;
	}

	QString Core::HandleSmiles (QString body)
	{
		const QString& pack = XmlSettingsManager::Instance ()
				.property ("SmileIcons").toString ();

		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookGonnaHandleSmiles (proxy, body, pack);
		if (proxy->IsCancelled ())
		{
			const QString& cand = proxy->GetReturnValue ().toString ();
			return cand.isEmpty () ? body : cand;
		}

		if (pack.isEmpty ())
			return body;

		IEmoticonResourceSource *src = SmilesOptionsModel_->GetSourceForOption (pack);
		if (!src)
			return body;

		const QString& img = QString ("<img src=\"%2\" title=\"%1\" />");
		QList<QByteArray> rawDatas;
		Q_FOREACH (const QString& str, src->GetEmoticonStrings (pack))
		{
			const QString& escaped = Qt::escape (str);
			if (!body.contains (escaped))
				continue;

			bool safeReplace = true;
			Q_FOREACH (const QByteArray& rd, rawDatas)
				if (rd.indexOf (escaped) != -1)
				{
					safeReplace = false;
					break;
				}
			if (!safeReplace)
				continue;

			const QByteArray& rawData = src->GetImage (pack, str).toBase64 ();
			rawDatas << rawData;
			const QString& smileStr = img
					.arg (str)
					.arg (QString ("data:image/png;base64," + rawData));
			if (body.startsWith (escaped))
				body.replace (0, escaped.size (), smileStr);

			auto whites = { " ", "\n", "\t", "<br/>", "<br />", "<br>" };
			Q_FOREACH (auto white, whites)
				body.replace (white + escaped, white + smileStr);
		}

		return body;
	}

	namespace
	{
		QStringList GetDisplayGroups (const ICLEntry *clEntry)
		{
			QStringList groups;
			if (clEntry->GetEntryType () == ICLEntry::ETUnauthEntry)
				groups << Core::tr ("Unauthorized users");
			else if (clEntry->GetEntryType () != ICLEntry::ETChat ||
					XmlSettingsManager::Instance ()
						.property ("GroupContacts").toBool ())
				groups = clEntry->Groups ();
			else
				groups << Core::tr ("Contacts");
			return groups;
		}
	}

	void Core::AddCLEntry (ICLEntry *clEntry,
			QStandardItem *accItem)
	{
		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookAddingCLEntryBegin (proxy, clEntry->GetObject ());
		if (proxy->IsCancelled ())
			return;

		connect (clEntry->GetObject (),
				SIGNAL (statusChanged (const EntryStatus&, const QString&)),
				this,
				SLOT (handleStatusChanged (const EntryStatus&, const QString&)));
		connect (clEntry->GetObject (),
				SIGNAL (availableVariantsChanged (const QStringList&)),
				this,
				SLOT (invalidateClientsIconCache ()));
		connect (clEntry->GetObject (),
				SIGNAL (gotMessage (QObject*)),
				this,
				SLOT (handleEntryGotMessage (QObject*)));
		connect (clEntry->GetObject (),
				SIGNAL (nameChanged (const QString&)),
				this,
				SLOT (handleEntryNameChanged (const QString&)));
		connect (clEntry->GetObject (),
				SIGNAL (groupsChanged (const QStringList&)),
				this,
				SLOT (handleEntryGroupsChanged (const QStringList&)));
		connect (clEntry->GetObject (),
				SIGNAL (permsChanged ()),
				this,
				SLOT (handleEntryPermsChanged ()));
		connect (clEntry->GetObject (),
				SIGNAL (entryGenerallyChanged ()),
				this,
				SLOT (remakeTooltipForSender ()));
		connect (clEntry->GetObject (),
				SIGNAL (avatarChanged (const QImage&)),
				this,
				SLOT (remakeTooltipForSender ()));
		connect (clEntry->GetObject (),
				SIGNAL (avatarChanged (const QImage&)),
				this,
				SLOT (invalidateSmoothAvatarCache ()));

		if (qobject_cast<IMUCEntry*> (clEntry->GetObject ()))
		{
			connect (clEntry->GetObject (),
					SIGNAL (nicknameConflict (const QString&)),
					this,
					SLOT (handleNicknameConflict (const QString&)));
			connect (clEntry->GetObject (),
					SIGNAL (beenKicked (const QString&)),
					this,
					SLOT (handleBeenKicked (const QString&)));
			connect (clEntry->GetObject (),
					SIGNAL (beenBanned (const QString&)),
					this,
					SLOT (handleBeenBanned (const QString&)));
		}

		if (qobject_cast<IAdvancedCLEntry*> (clEntry->GetObject ()))
		{
			connect (clEntry->GetObject (),
					SIGNAL (attentionDrawn (const QString&, const QString&)),
					this,
					SLOT (handleAttentionDrawn (const QString&, const QString&)));
			connect (clEntry->GetObject (),
					SIGNAL (activityChanged (const QString&)),
					this,
					SLOT (handleEntryPEPEvent (const QString&)));
			connect (clEntry->GetObject (),
					SIGNAL (moodChanged (const QString&)),
					this,
					SLOT (handleEntryPEPEvent (const QString&)));
			connect (clEntry->GetObject (),
					SIGNAL (tuneChanged (const QString&)),
					this,
					SLOT (handleEntryPEPEvent (const QString&)));
			connect (clEntry->GetObject (),
					SIGNAL (locationChanged (const QString&)),
					this,
					SLOT (handleEntryPEPEvent (const QString&)));
		}

#ifdef ENABLE_CRYPT
		if (!KeyStoreMgr_->isBusy ())
			RestoreKeyForEntry (clEntry);
#endif

		EventsNotifier_->RegisterEntry (clEntry);

		const QString& id = clEntry->GetEntryID ();
		ID2Entry_ [id] = clEntry->GetObject ();

		const QStringList& groups = GetDisplayGroups (clEntry);
		{
			ModelUpdateSafeguard outerGuard (CLModel_);
			QList<QStandardItem*> catItems = GetCategoriesItems (groups, accItem);
			Q_FOREACH (QStandardItem *catItem, catItems)
			{
				AddEntryTo (clEntry, catItem);

				bool isMucCat = catItem->data (CLRIsMUCCategory).toBool ();
				if (!isMucCat)
					isMucCat = clEntry->GetEntryType () == ICLEntry::ETPrivateChat;
				catItem->setData (isMucCat, CLRIsMUCCategory);
			}
		}

		HandleStatusChanged (clEntry->GetStatus (), clEntry, QString (), false, false);

		if (clEntry->GetEntryType () == ICLEntry::ETPrivateChat)
			handleEntryPermsChanged (clEntry, false);

		RebuildTooltip (clEntry);

		ChatTabsManager_->UpdateEntryMapping (id, clEntry->GetObject ());
		ChatTabsManager_->SetChatEnabled (id, true);

		proxy.reset (new Util::DefaultHookProxy);
		emit hookAddingCLEntryEnd (proxy, clEntry->GetObject ());
	}

	QList<QStandardItem*> Core::GetCategoriesItems (QStringList cats, QStandardItem *account)
	{
		if (cats.isEmpty ())
			cats << tr ("General");

		QList<QStandardItem*> result;
		ModelUpdateSafeguard guard (CLModel_);
		Q_FOREACH (const QString& cat, cats)
		{
			if (!Account2Category2Item_ [account].keys ().contains (cat))
			{
				QStandardItem *catItem = new QStandardItem (cat);
				catItem->setEditable (false);
				catItem->setData (account->data (CLRAccountObject), CLRAccountObject);
				catItem->setData (QVariant::fromValue<CLEntryType> (CLETCategory),
						CLREntryType);
				catItem->setData (cat, CLREntryCategory);
				catItem->setFlags (catItem->flags () | Qt::ItemIsDropEnabled);
				Account2Category2Item_ [account] [cat] = catItem;
				account->appendRow (catItem);
			}

			result << Account2Category2Item_ [account] [cat];
		}

		return result;
	}

	QStandardItem* Core::GetAccountItem (const QObject *accountObj)
	{
		for (int i = 0, size = CLModel_->rowCount ();
				i < size; ++i)
			if (CLModel_->item (i)->
						data (CLRAccountObject).value<QObject*> () ==
					accountObj)
				return CLModel_->item (i);
		return 0;
	}

	QStandardItem* Core::GetAccountItem (const QObject *accountObj,
			QMap<const QObject*, QStandardItem*>& accountItemCache)
	{
		if (accountItemCache.contains (accountObj))
			return accountItemCache [accountObj];
		else
		{
			QStandardItem *accountItem = GetAccountItem (accountObj);
			if (accountItem)
				accountItemCache [accountObj] = accountItem;
			return accountItem;
		}
	}

	namespace
	{
		QString Status2Str (const EntryStatus& status, std::shared_ptr<IProxyObject> obj)
		{
			QString result = obj->StateToString (status.State_);
			const QString& statusString = Qt::escape (status.StatusString_);
			if (!statusString.isEmpty ())
				result += " (" + statusString + ")";
			return result;
		}

		void FormatMood (QString& tip, const QMap<QString, QVariant>& moodInfo)
		{
			tip += "<br />" + Core::tr ("Mood:") + ' ';
			tip += MoodDialog::ToHumanReadable (moodInfo ["mood"].toString ());
			const QString& text = moodInfo ["text"].toString ();
			if (!text.isEmpty ())
				tip += " (" + text + ")";
		}

		void FormatActivity (QString& tip, const QMap<QString, QVariant>& actInfo)
		{
			tip += "<br />" + Core::tr ("Activity:") + ' ';
			tip += ActivityDialog::ToHumanReadable (actInfo ["general"].toString ());
			const QString& specific = ActivityDialog::ToHumanReadable (actInfo ["specific"].toString ());
			if (!specific.isEmpty ())
				tip += " (" + specific + ")";
			const QString& text = actInfo ["text"].toString ();
			if (!text.isEmpty ())
				tip += " (" + text + ")";
		}

		void FormatTune (QString& tip, const QMap<QString, QVariant>& tuneInfo)
		{
			const QString& artist = tuneInfo ["artist"].toString ();
			const QString& source = tuneInfo ["source"].toString ();
			const QString& title = tuneInfo ["title"].toString ();

			tip += "<br />" + Core::tr ("Now listening to:") + ' ';
			if (!artist.isEmpty () && !title.isEmpty ())
				tip += "<em>" + artist + "</em>" +
						QString::fromUtf8 (" — ") +
						"<em>" + title + "</em>";
			else if (!artist.isEmpty ())
				tip += "<em>" + artist + "</em>";
			else if (!title.isEmpty ())
				tip += "<em>" + title + "</em>";

			if (!source.isEmpty ())
				tip += ' ' + Core::tr ("from") +
						" <em>" + source + "</em>";

			const int length = tuneInfo ["length"].toInt ();
			if (length)
				tip += " (" + Util::MakeTimeFromLong (length) + ")";
		}
	}

	QString Core::MakeTooltipString (ICLEntry *entry) const
	{
		QString tip = "<table border='0'><tr><td>";

		if (entry->GetEntryType () != ICLEntry::ETMUC)
		{
			const int avatarSize = 75;
			const int minAvatarSize = 32;
			auto avatar = entry->GetAvatar ();
			if (avatar.isNull ())
				avatar = GetDefaultAvatar (avatarSize);

			if (std::max (avatar.width (), avatar.height ()) > avatarSize)
				avatar = avatar.scaled (avatarSize, avatarSize, Qt::KeepAspectRatio, Qt::FastTransformation);
			else if (std::max (avatar.width (), avatar.height ()) < minAvatarSize)
				avatar = avatar.scaled (minAvatarSize, minAvatarSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			tip += "<img src='" + Util::GetAsBase64Src (avatar) + "' />";

			tip += "</td><td>";
		}

		tip += "<strong>" + Qt::escape (entry->GetEntryName ()) + "</strong>";
		tip += "&nbsp;(<em>" + Qt::escape (entry->GetHumanReadableID ()) + "</em>)<br />";
		tip += Status2Str (entry->GetStatus (), PluginProxyObject_);
		if (entry->GetEntryType () != ICLEntry::ETPrivateChat)
		{
			tip += "<br />";
			tip += tr ("In groups:") + ' ' + Qt::escape (entry->Groups ().join ("; "));
		}

		const QStringList& variants = entry->Variants ();

		IMUCEntry *mucEntry = qobject_cast<IMUCEntry*> (entry->GetParentCLEntry ());
		if (mucEntry)
		{
			const QString& jid = mucEntry->GetRealID (entry->GetObject ());
			tip += "<br />" + tr ("Real ID:") + ' ';
			tip += jid.isEmpty () ? tr ("unknown") : Qt::escape (jid);
		}

		IMUCPerms *mucPerms = qobject_cast<IMUCPerms*> (entry->GetParentCLEntry ());
		if (mucPerms)
		{
			tip += "<hr />";
			const QMap<QByteArray, QList<QByteArray>>& perms =
					mucPerms->GetPerms (entry->GetObject ());
			Q_FOREACH (const QByteArray& permClass, perms.keys ())
			{
				tip += mucPerms->GetUserString (permClass);
				tip += ": ";

				QStringList users;
				Q_FOREACH (const QByteArray& perm, perms [permClass])
					users << mucPerms->GetUserString (perm);
				tip += users.join ("; ");
				tip += "<br />";
			}
		}

		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		proxy->SetValue ("tooltip", tip);
		emit hookTooltipBeforeVariants (proxy, entry->GetObject ());
		proxy->FillValue ("tooltip", tip);

		auto cleanupBR = [&tip] ()
		{
			tip = tip.simplified ();
			while (tip.endsWith ("<br />"))
			{
				tip.chop (6);
				tip = tip.simplified ();
			}
		};

		cleanupBR ();

		if (entry->GetEntryType () != ICLEntry::ETPrivateChat)
			Q_FOREACH (const QString& variant, variants)
			{
				const QMap<QString, QVariant>& info = entry->GetClientInfo (variant);
				if (info.isEmpty ())
					continue;

				tip += "<hr />";
				if (!variant.isEmpty ())
					tip += "<strong>" + variant;

				if (info.contains ("priority"))
					tip += " (" + QString::number (info.value ("priority").toInt ()) + ")";
				tip += "</strong><br />";
				tip += Status2Str (entry->GetStatus (variant), PluginProxyObject_);

				if (info.contains ("client_name"))
					tip += "<br />" + tr ("Using:") + ' ' + Qt::escape (info.value ("client_name").toString ());
				if (info.contains ("client_version"))
					tip += " " + Qt::escape (info.value ("client_version").toString ());
				if (info.contains ("client_remote_name"))
					tip += "<br />" + tr ("Claiming:") + ' ' + Qt::escape (info.value ("client_remote_name").toString ());
				if (info.contains ("client_os"))
					tip += "<br />" + tr ("OS:") + ' ' + Qt::escape (info.value ("client_os").toString ());

				if (info.contains ("user_mood"))
					FormatMood (tip, info ["user_mood"].toMap ());
				if (info.contains ("user_activity"))
					FormatActivity (tip, info ["user_activity"].toMap ());
				if (info.contains ("user_tune"))
					FormatTune (tip, info ["user_tune"].toMap ());

				if (info.contains ("custom_user_visible_map"))
				{
					const QVariantMap& map = info ["custom_user_visible_map"].toMap ();
					Q_FOREACH (const QString& key, map.keys ())
						tip += "<br />" + key + ": " + Qt::escape (map [key].toString ()) + "<br />";
				}
			}

		cleanupBR ();

		tip += "</td></tr></table>";

		return tip;
	}

	void Core::RebuildTooltip (ICLEntry *entry)
	{
		const QString& tip = MakeTooltipString (entry);
		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
			item->setToolTip (tip);
	}

	Entity Core::BuildStatusNotification (const EntryStatus& entrySt,
		ICLEntry *entry, const QString& variant)
	{
		if (entry->GetEntryType () != ICLEntry::ETChat)
			return Entity ();

		IAccount *acc = qobject_cast<IAccount*> (entry->GetParentAccount ());
		if (!LastAccountStatusChange_.contains (acc) ||
				LastAccountStatusChange_ [acc].secsTo (QDateTime::currentDateTime ()) < 5)
			return Entity ();

		IExtSelfInfoAccount *extAcc =
				qobject_cast<IExtSelfInfoAccount*> (entry->GetParentAccount ());
		if (extAcc &&
				extAcc->GetSelfContact () == entry->GetObject ())
			return Entity ();

		const QString& name = entry->GetEntryName ();
		const QString& status = Status2Str (entrySt, PluginProxyObject_);

		const QString& text = variant.isEmpty () ?
				Core::tr ("%1 is now %2.")
					.arg (name)
					.arg (status) :
				Core::tr ("%1/%2 is now %3.")
					.arg (name)
					.arg (variant)
					.arg (status);

		Entity e = Util::MakeNotification ("LeechCraft", text, PInfo_);
		e.Mime_ += "+advanced";

		BuildNotification (e, entry);
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = "org.LC.AdvNotifications.IM.StatusChange";
		e.Additional_ ["NotificationPixmap"] =
				QVariant::fromValue<QPixmap> (QPixmap::fromImage (entry->GetAvatar ()));

		e.Additional_ ["org.LC.AdvNotifications.FullText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = text;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;

		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = entrySt.StatusString_;
		e.Additional_ ["org.LC.Plugins.Azoth.NewStatus"] =
				PluginProxyObject_->StateToString (entrySt.State_);

		return e;
	}

	void Core::HandleStatusChanged (const EntryStatus& status,
			ICLEntry *entry, const QString& variant, bool asSignal, bool rebuildTooltip)
	{
		emit hookEntryStatusChanged (Util::DefaultHookProxy_ptr (new Util::DefaultHookProxy),
				entry->GetObject (), variant);

		invalidateClientsIconCache (entry);

		const State state = entry->GetStatus ().State_;
		Util::QIODevice_ptr icon = GetIconPathForState (state);

		if (rebuildTooltip)
			RebuildTooltip (entry);

		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
		{
			ItemIconManager_->SetIcon (item, icon.get ());
			RecalculateOnlineForCat (item->parent ());
		}

		const QString& id = entry->GetEntryID ();
		if (!XferJobManager_->GetPendingIncomingJobsFor (id).isEmpty ())
			CheckFileIcon (id);

		if (asSignal)
		{
			const Entity& e = BuildStatusNotification (status, entry, variant);
			if (!e.Mime_.isEmpty ())
				emit gotEntity (e);
		}
	}

	void Core::CheckFileIcon (const QString& id)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (GetEntry (id));
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "got null entry for"
					<< id;
			return;
		}

		if (XferJobManager_->GetPendingIncomingJobsFor (id).isEmpty ())
		{
			const QString& variant = entry->Variants ().value (0);
			HandleStatusChanged (entry->GetStatus (variant), entry, variant);
			return;
		}

		const QString& filename = XmlSettingsManager::Instance ()
				.property ("StatusIcons").toString () + "/file";
		Util::QIODevice_ptr fileIcon = ResourceLoaders_ [RLTStatusIconLoader]->LoadIcon (filename, true);
		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
			ItemIconManager_->SetIcon (item, fileIcon.get ());
	}

	void Core::IncreaseUnreadCount (ICLEntry* entry, int amount)
	{
		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
			{
				int prevValue = item->data (CLRUnreadMsgCount).toInt ();
				item->setData (std::max (0, prevValue + amount), CLRUnreadMsgCount);
				RecalculateUnreadForParents (item);
			}
	}

	namespace
	{
		QString GetStateIconFilename (State state)
		{
			QString iconName;
			switch (state)
			{
			case SOnline:
				iconName = "online";
				break;
			case SChat:
				iconName = "chatty";
				break;
			case SAway:
				iconName = "away";
				break;
			case SDND:
				iconName = "dnd";
				break;
			case SXA:
				iconName = "xa";
				break;
			case SOffline:
				iconName = "offline";
				break;
			case SConnecting:
				iconName = "connect";
				break;
			default:
				iconName = "perr";
				break;
			}

			QString filename = XmlSettingsManager::Instance ()
					.property ("StatusIcons").toString ();
			filename += '/';
			filename += iconName;

			return filename;
		}
	}

	Util::QIODevice_ptr Core::GetIconPathForState (State state) const
	{
		const QString& filename = GetStateIconFilename (state);
		return ResourceLoaders_ [RLTStatusIconLoader]->LoadIcon (filename, true);
	}

	QIcon Core::GetIconForState (State state) const
	{
		const QString& filename = GetStateIconFilename (state);
		return ResourceLoaders_ [RLTStatusIconLoader]->LoadPixmap (filename);
	}

	QIcon Core::GetAffIcon (const QByteArray& affName) const
	{
		QString filename = XmlSettingsManager::Instance ()
				.property ("AffIcons").toString ();
		filename += '/';
		filename += affName;

		return QIcon (ResourceLoaders_ [RLTAffIconLoader]->LoadPixmap (filename));
	}

	QMap<QString, QIcon> Core::GetClientIconForEntry (ICLEntry *entry)
	{
		if (EntryClientIconCache_.contains (entry))
			return EntryClientIconCache_ [entry];

		QMap<QString, QIcon> result;

		const QString& pack = XmlSettingsManager::Instance ()
					.property ("ClientIcons").toString () + '/';
		Q_FOREACH (const QString& variant, entry->Variants ())
		{
			const auto& type = entry->GetClientInfo (variant) ["client_type"].toString ();
			if (type.isNull ())
			{
				result [variant] = QIcon ();
				continue;
			}

			const QString& filename = pack + type;

			QPixmap pixmap = ResourceLoaders_ [RLTClientIconLoader]->LoadPixmap (filename);
			if (pixmap.isNull ())
				pixmap = ResourceLoaders_ [RLTClientIconLoader]->LoadPixmap (pack + "unknown");

			result [variant] = QIcon (pixmap);
		}

		EntryClientIconCache_ [entry] = result;
		return result;
	}

	QImage Core::GetAvatar (ICLEntry *entry, int size)
	{
		if (Entry2SmoothAvatarCache_.contains (entry) &&
				(Entry2SmoothAvatarCache_ [entry].width () == size ||
				 Entry2SmoothAvatarCache_ [entry].height () == size))
			return Entry2SmoothAvatarCache_ [entry];

		QImage avatar = entry ? entry->GetAvatar () : QImage ();
		if (avatar.isNull () || !avatar.width ())
			avatar = GetDefaultAvatar (size);

		const QImage& scaled = avatar.isNull () ?
				QImage () :
				avatar.scaled (size, size,
						Qt::KeepAspectRatio, Qt::SmoothTransformation);
		Entry2SmoothAvatarCache_ [entry] = scaled;
		return scaled;
	}

	QImage Core::GetDefaultAvatar (int size) const
	{
		const QString& name = XmlSettingsManager::Instance ()
				.property ("SystemIcons").toString () + "/default_avatar";
		const QImage& image = ResourceLoaders_ [RLTSystemIconLoader]->
				LoadPixmap (name).toImage ();
		return image.isNull () ?
				QImage () :
				image.scaled (size, size,
						Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	ActionsManager* Core::GetActionsManager () const
	{
		return ActionsManager_;
	}

	void Core::RecalculateUnreadForParents (QStandardItem *clItem)
	{
		QStandardItem *category = clItem->parent ();
		int sum = 0;
		for (int i = 0, rc = category->rowCount ();
				i < rc; ++i)
			sum += category->child (i)->data (CLRUnreadMsgCount).toInt ();
		category->setData (sum, CLRUnreadMsgCount);
	}

	void Core::RecalculateOnlineForCat (QStandardItem *catItem)
	{
		int result = 0;
		for (int i = 0; i < catItem->rowCount (); ++i)
		{
			auto entryObj = catItem->child (i)->
					data (CLREntryObject).value<QObject*> ();
			result += qobject_cast<ICLEntry*> (entryObj)->GetStatus ().State_ != SOffline;
		}

		catItem->setData (result, CLRNumOnline);
	}

	void Core::HandlePowerNotification (Entity e)
	{
		auto accs = GetAccountsPred (ProtocolPlugins_);

		qDebug () << Q_FUNC_INFO << e.Entity_;

		if (e.Entity_ == "Sleeping")
			Q_FOREACH (IAccount *acc, accs)
			{
				const auto& state = acc->GetState ();
				if (state.State_ == SOffline)
					continue;

				SavedStatus_ [acc] = state;
				acc->ChangeState ({SOffline, tr ("Client went to sleep")});
			}
		else if (e.Entity_ == "WokeUp")
		{
			Q_FOREACH (IAccount *acc, SavedStatus_.keys ())
				acc->ChangeState (SavedStatus_ [acc]);
			SavedStatus_.clear ();
		}
	}

	void Core::RemoveCLItem (QStandardItem *item)
	{
		QObject *entryObj = item->data (CLREntryObject).value<QObject*> ();
		Entry2Items_ [qobject_cast<ICLEntry*> (entryObj)].removeAll (item);

		QStandardItem *category = item->parent ();
		const int unread = item->data (CLRUnreadMsgCount).toInt ();

		ItemIconManager_->Cancel (item);

		ModelUpdateSafeguard guard (CLModel_);
		category->removeRow (item->row ());

		if (!category->rowCount ())
		{
			QStandardItem *account = category->parent ();
			ItemIconManager_->Cancel (category);

			const QString& text = category->text ();

			account->removeRow (category->row ());
			Account2Category2Item_ [account].remove (text);
		}
		else if (unread)
		{
			const int sum = category->data (CLRUnreadMsgCount).toInt ();
			category->setData (std::max (sum - unread, 0), CLRUnreadMsgCount);
		}
	}

	void Core::AddEntryTo (ICLEntry *clEntry, QStandardItem *catItem)
	{
		QStandardItem *clItem = new QStandardItem (clEntry->GetEntryName ());
		clItem->setEditable (false);
		QObject *accObj = clEntry->GetParentAccount ();
		clItem->setData (QVariant::fromValue<QObject*> (accObj),
				CLRAccountObject);
		clItem->setData (QVariant::fromValue<QObject*> (clEntry->GetObject ()),
				CLREntryObject);
		clItem->setData (QVariant::fromValue<CLEntryType> (CLETContact),
				CLREntryType);
		clItem->setData (catItem->data (CLREntryCategory),
				CLREntryCategory);

		clItem->setFlags (clItem->flags () |
				Qt::ItemIsDragEnabled |
				Qt::ItemIsDropEnabled);

		ModelUpdateSafeguard guard (CLModel_);
		catItem->appendRow (clItem);

		Entry2Items_ [clEntry] << clItem;
	}

	void Core::SuggestJoiningMUC (IAccount *acc, const QVariantMap& ident)
	{
		QList<IAccount*> accs;
		accs << acc;

		JoinConferenceDialog *dia = new JoinConferenceDialog (accs, Proxy_->GetMainWindow ());
		dia->SetIdentifyingData (ident);
		dia->show ();
	}

	IChatStyleResourceSource* Core::GetCurrentChatStyle (QObject *entry) const
	{
		const QString& opt = XmlSettingsManager::Instance ()
				.property (GetStyleOptName (entry)).toString ();
		IChatStyleResourceSource *src = ChatStylesOptionsModel_->GetSourceForOption (opt);
		if (!src)
			qWarning () << Q_FUNC_INFO
					<< "empty result for"
					<< opt;
		return src;
	}

	void Core::FillANFields ()
	{
		const QStringList commonFields = QStringList ("org.LC.AdvNotifications.IM.MUCHighlightMessage")
						<< "org.LC.AdvNotifications.IM.MUCMessage"
						<< "org.LC.AdvNotifications.IM.IncomingMessage"
						<< "org.LC.AdvNotifications.IM.AttentionDrawn"
						<< "org.LC.AdvNotifications.IM.Subscr.Granted"
						<< "org.LC.AdvNotifications.IM.Subscr.Revoked"
						<< "org.LC.AdvNotifications.IM.Subscr.Requested"
						<< "org.LC.AdvNotifications.IM.StatusChange";

		ANFields_ << ANFieldData ("org.LC.Plugins.Azoth.Msg",
				tr ("Message body"),
				tr ("Original human-readable message body."),
				QVariant::String,
				commonFields);

		ANFields_ << ANFieldData ("org.LC.Plugins.Azoth.SourceName",
				tr ("Sender name"),
				tr ("Human-readable name of the sender of the message."),
				QVariant::String,
				commonFields);

		ANFields_ << ANFieldData ("org.LC.Plugins.Azoth.SourceID",
				tr ("Sender ID"),
				tr ("Human-readable ID of the sender (protocol-specific)."),
				QVariant::String,
				commonFields);

		ANFields_ << ANFieldData ("org.LC.Plugins.Azoth.SourceGroups",
				tr ("Sender groups"),
				tr ("Groups to which the sender belongs."),
				QVariant::StringList,
				commonFields);

		ANFields_ << ANFieldData ("org.LC.Plugins.Azoth.NewStatus",
				tr ("New status"),
				tr ("The new status string of the contact."),
				QVariant::String,
				QStringList ("org.LC.AdvNotifications.IM.StatusChange"));
	}

	namespace
	{
		template<typename T>
		T FindTop (const QMap<T, int>& map)
		{
			T maxT = T ();
			int max = 0;
			Q_FOREACH (const T& t, map.keys ())
			{
				const int val = map [t];
				if (val > max)
				{
					max = val;
					maxT = t;
				}
			}

			return maxT;
		}
	}

	void Core::UpdateInitState (State state)
	{
		if (state == SConnecting)
			return;

		const State prevTop = FindTop (StateCounter_);

		StateCounter_.clear ();
		Q_FOREACH (IAccount *acc, GetAccounts ())
			++StateCounter_ [acc->GetState ().State_];

		StateCounter_.remove (SOffline);

		const State newTop = FindTop (StateCounter_);

		if (newTop != prevTop)
			emit topStatusChanged (newTop);
	}

#ifdef ENABLE_CRYPT
	void Core::RestoreKeyForAccount (IAccount *acc)
	{
		ISupportPGP *pgp = qobject_cast<ISupportPGP*> (acc->GetObject ());
		if (!pgp)
			return;

		QSettings settings (QCoreApplication::organizationName (),
			QCoreApplication::applicationName () + "_Azoth");
		settings.beginGroup ("PrivateKeys");
		const QString& keyId = settings.value (acc->GetAccountID ()).toString ();
		settings.endGroup ();

		if (keyId.isEmpty ())
			return;

		Q_FOREACH (const QCA::PGPKey& key, GetPrivateKeys ())
			if (key.keyId () == keyId)
			{
				pgp->SetPrivateKey (key);
				break;
			}
	}

	void Core::RestoreKeyForEntry (ICLEntry *clEntry)
	{
		if (!StoredPublicKeys_.contains (clEntry->GetEntryID ()))
			return;

		ISupportPGP *pgp = qobject_cast<ISupportPGP*> (clEntry->GetParentAccount ());
		if (!pgp)
		{
			qWarning () << Q_FUNC_INFO
					<< clEntry->GetObject ()
					<< clEntry->GetParentAccount ()
					<< "doesn't implement ISupportGPG though "
						"we have the key";
			return;
		}

		const QString& keyId = StoredPublicKeys_.take (clEntry->GetEntryID ());
		Q_FOREACH (const QCA::PGPKey& key, GetPublicKeys ())
			if (key.keyId () == keyId)
			{
				pgp->SetEntryKey (clEntry->GetObject (), key);
				break;
			}
	}
#endif

	void Core::handleMucJoinRequested ()
	{
		auto accounts = GetAccountsPred (ProtocolPlugins_,
				[] (IProtocol *proto) { return proto->GetFeatures () & IProtocol::PFMUCsJoinable; });

		JoinConferenceDialog *dia = new JoinConferenceDialog (accounts, Proxy_->GetMainWindow ());
		dia->show ();
	}

	void Core::handleShowNextUnread ()
	{
		UnreadQueueManager_->ShowNext ();
	}

	void Core::saveAccountVisibility (IAccount *account)
	{
		const auto& id = "ShowAccount_" + account->GetAccountID ();
		XmlSettingsManager::Instance ().setProperty (id, account->IsShownInRoster ());
	}

	void Core::handleNewProtocols (const QList<QObject*>& protocols)
	{
		Q_FOREACH (QObject *protoObj, protocols)
		{
			IProtocol *proto = qobject_cast<IProtocol*> (protoObj);

			Q_FOREACH (QObject *accObj,
					proto->GetRegisteredAccounts ())
				addAccount (accObj);

			connect (proto->GetObject (),
					SIGNAL (accountAdded (QObject*)),
					this,
					SLOT (addAccount (QObject*)));
			connect (proto->GetObject (),
					SIGNAL (accountRemoved (QObject*)),
					this,
					SLOT (handleAccountRemoved (QObject*)));
		}
	}

	void Core::addAccount (QObject *accObject)
	{
		IAccount *account = qobject_cast<IAccount*> (accObject);
		if (!account)
		{
			qWarning () << Q_FUNC_INFO
					<< "account doesn't implement IAccount*"
					<< accObject
					<< sender ();
			return;
		}

		const auto& showKey = QString::fromUtf8 ("ShowAccount_" + account->GetAccountID ());
		const bool show = XmlSettingsManager::Instance ().Property (showKey, true).toBool ();
		account->SetShownInRoster (show);

		emit accountAdded (account);

#ifdef ENABLE_CRYPT
		if (!KeyStoreMgr_->isBusy ())
			RestoreKeyForAccount (account);
#endif

		QStandardItem *accItem = new QStandardItem (account->GetAccountName ());
		accItem->setData (QVariant::fromValue<QObject*> (accObject),
				CLRAccountObject);
		accItem->setData (QVariant::fromValue<CLEntryType> (CLETAccount),
				CLREntryType);
		ItemIconManager_->SetIcon (accItem,
				GetIconPathForState (account->GetState ().State_).get ());

		{
			ModelUpdateSafeguard guard (CLModel_);
			CLModel_->appendRow (accItem);
		}

		accItem->setEditable (false);

		QList<QStandardItem*> clItems;
		Q_FOREACH (QObject *clObj, account->GetCLEntries ())
		{
			ICLEntry *clEntry = qobject_cast<ICLEntry*> (clObj);
			if (!clEntry)
			{
				qWarning () << Q_FUNC_INFO
						<< "entry doesn't implement ICLEntry"
						<< clObj
						<< account;
				continue;
			}

			AddCLEntry (clEntry, accItem);
		}

		connect (accObject,
				SIGNAL (gotCLItems (const QList<QObject*>&)),
				this,
				SLOT (handleGotCLItems (const QList<QObject*>&)));
		connect (accObject,
				SIGNAL (removedCLItems (const QList<QObject*>&)),
				this,
				SLOT (handleRemovedCLItems (const QList<QObject*>&)));
		connect (accObject,
				SIGNAL (authorizationRequested (QObject*, const QString&)),
				this,
				SLOT (handleAuthorizationRequested (QObject*, const QString&)));

		connect (accObject,
				SIGNAL (itemSubscribed (QObject*, const QString&)),
				this,
				SLOT (handleItemSubscribed (QObject*, const QString&)));
		connect (accObject,
				SIGNAL (itemUnsubscribed (QObject*, const QString&)),
				this,
				SLOT (handleItemUnsubscribed (QObject*, const QString&)));
		connect (accObject,
				SIGNAL (itemUnsubscribed (const QString&, const QString&)),
				this,
				SLOT (handleItemUnsubscribed (const QString&, const QString&)));
		connect (accObject,
				SIGNAL (itemCancelledSubscription (QObject*, const QString&)),
				this,
				SLOT (handleItemCancelledSubscription (QObject*, const QString&)));
		connect (accObject,
				SIGNAL (itemGrantedSubscription (QObject*, const QString&)),
				this,
				SLOT (handleItemGrantedSubscription (QObject*, const QString&)));
		connect (accObject,
				SIGNAL (mucInvitationReceived (QVariantMap, QString, QString)),
				this,
				SLOT (handleMUCInvitation (QVariantMap, QString, QString)));

		connect (accObject,
				SIGNAL (statusChanged (const EntryStatus&)),
				this,
				SLOT (handleAccountStatusChanged (const EntryStatus&)));

		connect (accObject,
				SIGNAL (accountRenamed (const QString&)),
				this,
				SLOT (handleAccountRenamed (const QString&)));

		if (qobject_cast<IHaveServiceDiscovery*> (accObject))
			connect (accObject,
					SIGNAL (gotSDSession (QObject*)),
					this,
					SLOT (handleGotSDSession (QObject*)));

		IProtocol *proto = qobject_cast<IProtocol*> (account->GetParentProtocol ());
		if (proto && account->IsShownInRoster ())
		{
			const QByteArray& id = proto->GetProtocolID () + account->GetAccountID ();
			const QVariant& var = XmlSettingsManager::Instance ().property (id);
			if (!var.isNull () && var.canConvert<QByteArray> ())
			{
				EntryStatus s;
				QDataStream stream (var.toByteArray ());
				stream >> s;
				account->ChangeState (s);
			}
			else
				UpdateInitState (account->GetState ().State_);
		}
		else if (!proto)
			qWarning () << Q_FUNC_INFO
					<< "account's parent proto isn't IProtocol"
					<< account->GetParentProtocol ();

		QObject *xferMgr = account->GetTransferManager ();
		if (xferMgr)
		{
			XferJobManager_->AddAccountManager (xferMgr);

			connect (xferMgr,
					SIGNAL (fileOffered (QObject*)),
					this,
					SLOT (handleFileOffered (QObject*)));
		}

		CallManager_->AddAccount (account->GetObject ());

		if (qobject_cast<ISupportRIEX*> (account->GetObject ()))
			connect (account->GetObject (),
					SIGNAL (riexItemsSuggested (QList<LeechCraft::Azoth::RIEXItem>, QObject*, QString)),
					this,
					SLOT (handleRIEXItemsSuggested (QList<LeechCraft::Azoth::RIEXItem>, QObject*, QString)));
	}

	void Core::handleAccountRemoved (QObject *account)
	{
		IAccount *accFace =
				qobject_cast<IAccount*> (account);
				if (!accFace)
		{
			qWarning () << Q_FUNC_INFO
					<< "account doesn't implement IAccount*"
					<< account
					<< sender ();
			return;
		}

		emit accountRemoved (accFace);

		for (int i = 0; i < CLModel_->rowCount (); ++i)
		{
			QStandardItem *item = CLModel_->item (i);
			QObject *obj = item->data (CLRAccountObject).value<QObject*> ();
			if (obj == account)
			{
				ItemIconManager_->Cancel (item);
				{
					ModelUpdateSafeguard guard (CLModel_);
					CLModel_->removeRow (i);
				}
				break;
			}
		}

		Q_FOREACH (ICLEntry *entry, Entry2Items_.keys ())
			if (entry->GetParentAccount () == account)
				Entry2Items_.remove (entry);
	}

	void Core::handleGotCLItems (const QList<QObject*>& items)
	{
		ModelUpdateSafeguard outerGuard (CLModel_);

		QMap<const QObject*, QStandardItem*> accountItemCache;
		Q_FOREACH (QObject *item, items)
		{
			ICLEntry *entry = qobject_cast<ICLEntry*> (item);
			if (!entry)
			{
				qWarning () << Q_FUNC_INFO
						<< item
						<< "is not a valid ICLEntry";
				continue;
			}

			if (Entry2Items_.contains (entry))
				continue;

			QObject *accountObj = entry->GetParentAccount ();
			if (!accountObj)
			{
				qWarning () << Q_FUNC_INFO
						<< "account object of"
						<< item
						<< "is null";
				continue;
			}

			QStandardItem *accountItem = GetAccountItem (accountObj, accountItemCache);

			if (!accountItem)
			{
				qWarning () << Q_FUNC_INFO
						<< "could not find account item for"
						<< item
						<< accountObj;
				continue;
			}

			AddCLEntry (entry, accountItem);

			if (entry->GetEntryType () & ICLEntry::ETMUC)
			{
				QStandardItem *item = Entry2Items_ [entry].first ();
				OpenChat (CLModel_->indexFromItem (item));
			}

			ChatTabsManager_->HandleEntryAdded (entry);
		}
	}

	void Core::handleRemovedCLItems (const QList<QObject*>& items)
	{
		Q_FOREACH (QObject *clitem, items)
		{
			ICLEntry *entry = qobject_cast<ICLEntry*> (clitem);
			if (!entry)
			{
				qWarning () << Q_FUNC_INFO
						<< clitem
						<< "is not a valid ICLEntry";
				continue;
			}

			disconnect (clitem,
					0,
					this,
					0);

			ChatTabsManager_->HandleEntryRemoved (entry);

			Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
				RemoveCLItem (item);

			Entry2Items_.remove (entry);

			ActionsManager_->HandleEntryRemoved (entry);

			ID2Entry_.remove (entry->GetEntryID ());

			EntryClientIconCache_.remove (entry);
			Entry2SmoothAvatarCache_.remove (entry);
			invalidateClientsIconCache (clitem);
		}
	}

	void Core::handleAccountStatusChanged (const EntryStatus& status)
	{
		IAccount *acc = qobject_cast<IAccount*> (sender ());
		if (!acc)
		{
			qWarning () << Q_FUNC_INFO
					<< "sender is not an IAccount"
					<< sender ();
			return;
		}

		IProtocol *proto = qobject_cast<IProtocol*> (acc->GetParentProtocol ());
		if (!proto)
		{
			qWarning () << Q_FUNC_INFO
					<< "account's proto is not a IProtocol"
					<< acc->GetParentProtocol ();
			return;
		}

		UpdateInitState (status.State_);

		if (status.State_ == SOffline)
			LastAccountStatusChange_.remove (acc);
		else if (!LastAccountStatusChange_.contains (acc))
			LastAccountStatusChange_ [acc] = QDateTime::currentDateTime ();

		const QByteArray& id = proto->GetProtocolID () + acc->GetAccountID ();
		QByteArray serializedStatus;
		{
			QDataStream stream (&serializedStatus, QIODevice::WriteOnly);
			stream << status;
		}
		XmlSettingsManager::Instance ().setProperty (id,
				serializedStatus);

		for (int i = 0, size = CLModel_->rowCount (); i < size; ++i)
		{
			QStandardItem *item = CLModel_->item (i);
			if (item->data (CLRAccountObject).value<QObject*> () != sender ())
				continue;

			ItemIconManager_->SetIcon (item,
					GetIconPathForState (status.State_).get ());
			return;
		}

		qWarning () << Q_FUNC_INFO
				<< "item for account"
				<< sender ()
				<< "not found";
	}

	void Core::handleAccountRenamed (const QString& name)
	{
		for (int i = 0, size = CLModel_->rowCount (); i < size; ++i)
		{
			QStandardItem *item = CLModel_->item (i);
			if (item->data (CLRAccountObject).value<QObject*> () != sender ())
				continue;

			item->setText (name);
			return;
		}
	}

	void Core::handleStatusChanged (const EntryStatus& status, const QString& variant)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "sender is not a ICLEntry:"
					<< sender ();
			return;
		}

		HandleStatusChanged (status, entry, variant, true);
	}

	void Core::handleEntryPEPEvent (const QString&)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "sender is not a ICLEntry"
					<< sender ();
			return;
		}

		RebuildTooltip (entry);
	}

	void Core::handleEntryNameChanged (const QString& newName)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "sender is not a ICLEntry:"
					<< sender ();
			return;
		}

		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
			item->setText (newName);

		if (entry->Variants ().size ())
			HandleStatusChanged (entry->GetStatus (), entry, entry->Variants ().first ());
	}

	void Core::handleEntryGroupsChanged (QStringList newGroups, QObject *perform)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (perform ? perform : sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "could not be casted to ICLEntry";
			return;
		}

		if (entry->GetEntryType () == ICLEntry::ETChat)
			newGroups = GetDisplayGroups (entry);

		if (!Entry2Items_.contains (entry))
			return;

		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
		{
			const QString& oldCat = item->data (CLREntryCategory).toString ();
			if (newGroups.removeAll (oldCat))
				continue;

			RemoveCLItem (item);
		}

		if (newGroups.isEmpty () && Entry2Items_ [entry].size ())
			return;

		QStandardItem *accItem =
				GetAccountItem (entry->GetParentAccount ());

		QList<QStandardItem*> catItems =
				GetCategoriesItems (newGroups, accItem);
		Q_FOREACH (QStandardItem *catItem, catItems)
			AddEntryTo (entry, catItem);

		HandleStatusChanged (entry->GetStatus (), entry, QString ());
	}

	void Core::handleEntryPermsChanged (ICLEntry *suggest, bool rebuildTooltip)
	{
		ICLEntry *entry = suggest ? suggest : qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "could not be casted to ICLEntry";
			return;
		}

		QObject *entryObj = entry->GetObject ();
		IMUCPerms *mucPerms = qobject_cast<IMUCPerms*> (entry->GetParentCLEntry ());
		if (!mucPerms)
			return;

		const QString& name = mucPerms->GetAffName (entryObj);
		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
			item->setData (name, CLRAffiliation);

		if (rebuildTooltip)
			RebuildTooltip (entry);
	}

	void Core::remakeTooltipForSender ()
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "could not be casted to ICLEntry";
			return;
		}

		RebuildTooltip (entry);
	}

	void Core::handleEntryGotMessage (QObject *msgObj)
	{
		IMessage *msg = qobject_cast<IMessage*> (msgObj);
		if (!msg)
		{
			qWarning () << Q_FUNC_INFO
					<< msgObj
					<< "doesn't implement IMessage";
			return;
		}

		ICLEntry *other = qobject_cast<ICLEntry*> (msg->OtherPart ());

		if (!other && msg->OtherPart ())
		{
			qWarning () << Q_FUNC_INFO
					<< "message's other part cannot be cast to ICLEntry"
					<< msg->OtherPart ();
			return;
		}

		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookGotMessage (proxy, msgObj);
		if (proxy->IsCancelled ())
			return;

		proxy.reset (new Util::DefaultHookProxy);
		emit hookGotMessage2 (proxy, msgObj);

		if (msg->GetMessageType () != IMessage::MTMUCMessage &&
				msg->GetMessageType () != IMessage::MTChatMessage)
			return;

		ICLEntry *parentCL = qobject_cast<ICLEntry*> (msg->ParentCLEntry ());

		if (ShouldCountUnread (parentCL, msg))
		{
			IncreaseUnreadCount (parentCL);
			UnreadQueueManager_->AddMessage (msgObj);
		}

		if (msg->GetDirection () != IMessage::DIn ||
				ChatTabsManager_->IsActiveChat (parentCL))
			return;

		ChatTabsManager_->HandleInMessage (msg);

		bool showMsg = XmlSettingsManager::Instance ()
				.property ("ShowMsgInNotifications").toBool ();

		QString msgString;
		bool isHighlightMsg = false;
		switch (msg->GetMessageType ())
		{
		case IMessage::MTChatMessage:
			if (XmlSettingsManager::Instance ()
					.property ("NotifyAboutIncomingMessages").toBool ())
			{
				if (!showMsg)
					msgString = tr ("Incoming chat message from <em>%1</em>.")
							.arg (other->GetEntryName ());
				else
				{
					const QString& body = msg->GetBody ();
					const QString& notifMsg = body.size () > 50 ?
							body.left (50) + "..." :
							body;
					msgString = tr ("Incoming chat message from <em>%1</em>: <em>%2</em>.")
							.arg (other->GetEntryName ())
							.arg (notifMsg);
				}
			}
			break;
		case IMessage::MTMUCMessage:
		{
			isHighlightMsg = IsHighlightMessage (msg);
			if (isHighlightMsg && XmlSettingsManager::Instance ()
					.property ("NotifyAboutConferenceHighlights").toBool ())
			{
				if (!showMsg)
					msgString = tr ("Highlighted in conference <em>%1</em> by <em>%2</em>.")
							.arg (parentCL->GetEntryName ())
							.arg (other->GetEntryName ());
				else
				{
					const QString& body = msg->GetBody ();
					const QString& notifMsg = body.size () > 50 ?
							body.left (50) + "..." :
							body;
					msgString = tr ("Highlighted in conference <em>%1</em> by <em>%2</em>: <em>%3</em>.")
							.arg (parentCL->GetEntryName ())
							.arg (other->GetEntryName ())
							.arg (notifMsg);
				}
			}
			break;
		}
		default:
			return;
		}

		Entity e = Util::MakeNotification ("Azoth",
				msgString,
				PInfo_);

		if (msgString.isEmpty ())
			e.Mime_ += "+advanced";

		ICLEntry *entry = msg->GetMessageType () == IMessage::MTMUCMessage ?
				parentCL :
				other;
		BuildNotification (e, entry);
		QStandardItem *someItem = Entry2Items_ [entry].value (0);
		const int count = someItem ?
				someItem->data (CLRUnreadMsgCount).toInt () :
				0;
		if (msg->GetMessageType () == IMessage::MTMUCMessage)
		{
			e.Additional_ ["org.LC.AdvNotifications.EventType"] = isHighlightMsg ?
					"org.LC.AdvNotifications.IM.MUCHighlightMessage" :
					"org.LC.AdvNotifications.IM.MUCMessage";
			e.Additional_ ["NotificationPixmap"] =
					QVariant::fromValue<QPixmap> (QPixmap::fromImage (other->GetAvatar ()));

			if (isHighlightMsg)
				e.Additional_ ["org.LC.AdvNotifications.FullText"] =
					tr ("%n message(s) from", 0, count) + ' ' + other->GetEntryName () +
							" <em>(" + parentCL->GetEntryName () + ")</em>";
			else
				e.Additional_ ["org.LC.AdvNotifications.FullText"] =
					tr ("%n message(s) in", 0, count) + ' ' + parentCL->GetEntryName ();
		}
		else
		{
			e.Additional_ ["org.LC.AdvNotifications.EventType"] =
					"org.LC.AdvNotifications.IM.IncomingMessage";
			e.Additional_ ["org.LC.AdvNotifications.FullText"] =
				tr ("%n message(s) from", 0, count) +
						' ' + other->GetEntryName ();
		}

		e.Additional_ ["org.LC.AdvNotifications.Count"] = count;

		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = tr ("%n message(s)", 0, count);
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = msg->GetBody ();

		Util::NotificationActionHandler *nh =
				new Util::NotificationActionHandler (e, this);
		nh->AddFunction (tr ("Open chat"),
				[parentCL, this] () { ChatTabsManager_->OpenChat (parentCL); });
		nh->AddDependentObject (parentCL->GetObject ());

		emit gotEntity (e);
	}

	void Core::handleAuthorizationRequested (QObject *entryObj, const QString& msg)
	{
		Util::DefaultHookProxy_ptr proxy (new Util::DefaultHookProxy);
		emit hookGotAuthRequest (proxy, entryObj, msg);
		if (proxy->IsCancelled ())
			return;

		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< entryObj
					<< "doesn't implement ICLEntry";
			return;
		}

		QString str = msg.isEmpty () ?
				tr ("Subscription requested by %1.")
					.arg (entry->GetEntryName ()) :
				tr ("Subscription requested by %1: %2.")
					.arg (entry->GetEntryName ())
					.arg (msg);
		Entity e = Util::MakeNotification ("Azoth", str, PInfo_);

		BuildNotification (e, entry);
		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.AuthRequestFrom/" + entry->GetEntryID ();
		e.Additional_ ["org.LC.AdvNotifications.EventType"] =
				"org.LC.AdvNotifications.IM.Subscr.Requested";
		e.Additional_ ["org.LC.AdvNotifications.FullText"] = str;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = msg;

		Util::NotificationActionHandler *nh =
				new Util::NotificationActionHandler (e, this);
		nh->AddFunction (tr ("Authorize"), [this, entry] () { AuthorizeEntry (entry); });
		nh->AddFunction (tr ("Deny"), [this, entry] () { DenyAuthForEntry (entry); });
		nh->AddFunction (tr ("View info"), [entry] () { entry->ShowInfo (); });
		nh->AddDependentObject (entry->GetObject ());
		emit gotEntity (e);
	}

	void Core::handleAttentionDrawn (const QString& text, const QString&)
	{
		if (XmlSettingsManager::Instance ()
				.property ("IgnoreDrawAttentions").toBool ())
			return;

		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "doesn't implement ICLEntry";
			return;
		}

		const QString& str = text.isEmpty () ?
				tr ("%1 requests your attention")
					.arg (entry->GetEntryName ()) :
				tr ("%1 requests your attention: %2")
					.arg (entry->GetEntryName ())
					.arg (text);

		Entity e = Util::MakeNotification ("Azoth", str, PInfo_);
		BuildNotification (e, entry);
		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.AttentionDrawnBy/" + entry->GetEntryID ();
		e.Additional_ ["org.LC.AdvNotifications.DeltaCount"] = 1;
		e.Additional_ ["org.LC.AdvNotifications.EventType"] =
				"org.LC.AdvNotifications.IM.AttentionDrawn";
		e.Additional_ ["org.LC.AdvNotifications.ExtendedText"] = tr ("Attention requested");
		e.Additional_ ["org.LC.AdvNotifications.FullText"] = tr ("Attention requested by %1")
				.arg (entry->GetEntryName ());
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = text;

		Util::NotificationActionHandler *nh =
				new Util::NotificationActionHandler (e, this);
		nh->AddFunction (tr ("Open chat"),
				[entry, this] () { ChatTabsManager_->OpenChat (entry); });
		nh->AddDependentObject (entry->GetObject ());

		emit gotEntity (e);
	}

	void Core::handleNicknameConflict (const QString& usedNick)
	{
		ICLEntry *clEntry = qobject_cast<ICLEntry*> (sender ());
		IMUCEntry *entry = qobject_cast<IMUCEntry*> (sender ());
		if (!entry || !clEntry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "doesn't implement ICLEntry or IMUCEntry";
			return;
		}

		QString altNick;
		if (XmlSettingsManager::Instance ().property ("UseAltNick").toBool ())
		{
			QString append = XmlSettingsManager::Instance ()
				.property ("AlternativeNickname").toString ();
			if (append.isEmpty ())
				append = "_azoth";
			altNick = usedNick + append;
		}

		if ((altNick.isEmpty () || altNick == usedNick) &&
				QMessageBox::question (0,
						tr ("Nickname conflict"),
						tr ("You have specified a nickname for %1 that's "
							"already used. Would you like to try to "
							"join with another nick?")
							.arg (clEntry->GetEntryName ()),
						QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			return;

		const QString& newNick = altNick.isEmpty () || altNick == usedNick ?
				QInputDialog::getText (0,
						tr ("Enter new nick"),
						tr ("Enter new nick for joining %1 (%2 is already used):")
							.arg (clEntry->GetEntryName ())
							.arg (usedNick),
						QLineEdit::Normal,
						usedNick) :
				altNick;
		if (newNick.isEmpty ())
			return;

		entry->SetNick (newNick);
		entry->Join ();
	}

	void Core::handleBeenKicked (const QString& reason)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		IMUCEntry *mucEntry = qobject_cast<IMUCEntry*> (sender ());
		if (!entry || !mucEntry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "doesn't implement ICLEntry or IMUCEntry";
			return;
		}

		const QString& text = reason.isEmpty () ?
				tr ("You have been kicked from %1. Do you want to rejoin?")
					.arg (entry->GetEntryName ()) :
				tr ("You have been kicked from %1: %2. Do you want to rejoin?")
					.arg (entry->GetEntryName ())
					.arg (reason);

		if (QMessageBox::question (0,
				"LeechCraft Azoth",
				text,
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			mucEntry->Join ();
	}

	void Core::handleBeenBanned (const QString& reason)
	{
		ICLEntry* entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "doesn't implement ICLEntry";
			return;
		}

		const QString& text = reason.isEmpty () ?
				tr ("You have been banned from %1.")
					.arg (entry->GetEntryName ()) :
				tr ("You have been banned from %1: %2.")
					.arg (entry->GetEntryName ())
					.arg (reason);
		QMessageBox::warning (0,
				"LeechCraft Azoth",
				text);
	}

	void Core::NotifyWithReason (QObject *entryObj, const QString& msg,
			const char *func, const QString& eventType,
			const QString& patternLite, const QString& patternFull)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
		if (!entry)
		{
			qWarning () << func
					<< entryObj
					<< "doesn't implement ICLEntry";
			return;
		}

		QString str = msg.isEmpty () ?
				patternLite
					.arg (entry->GetEntryName ())
					.arg (entry->GetHumanReadableID ()) :
				patternFull
					.arg (entry->GetEntryName ())
					.arg (entry->GetHumanReadableID ())
					.arg (msg);

		Entity e = Util::MakeNotification ("Azoth", str, PInfo_);
		BuildNotification (e, entry);

		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.Event/" + eventType + entry->GetEntryID ();
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = eventType;
		e.Additional_ ["org.LC.AdvNotifications.FullText"] = str;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = msg;

		emit gotEntity (e);
	}

	/** @todo Option for disabling notifications of subscription events.
		*/
	void Core::handleItemSubscribed (QObject *entryObj, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifySubscriptions").toBool ())
			return;

		NotifyWithReason (entryObj, msg, Q_FUNC_INFO,
				"org.LC.AdvNotifications.IM.Subscr.Subscribed",
				tr ("%1 (%2) subscribed to us."),
				tr ("%1 (%2) subscribed to us: %3."));
	}

	/** @todo Option for disabling notifications of unsubscription events.
		*/
	void Core::handleItemUnsubscribed (QObject *entryObj, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifyUnsubscriptions").toBool ())
			return;

		NotifyWithReason (entryObj, msg, Q_FUNC_INFO,
				"org.LC.AdvNotifications.IM.Subscr.Unsubscribed",
				tr ("%1 (%2) unsubscribed from us."),
				tr ("%1 (%2) unsubscribed from us: %3."));
	}

	/** @todo Option for disabling notifications of unsubscription events from
		* non-roster items.
		*/
	void Core::handleItemUnsubscribed (const QString& entryId, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifyAboutNonrosterUnsub").toBool ())
			return;

		QString str = msg.isEmpty () ?
				tr ("%1 unsubscribed from us.")
					.arg (entryId) :
				tr ("%1 unsubscribed from us: %2.")
					.arg (entryId)
					.arg (msg);
		emit gotEntity (Util::MakeNotification ("Azoth", str, PInfo_));
	}

	void Core::handleItemCancelledSubscription (QObject *entryObj, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifySubCancels").toBool ())
			return;

		NotifyWithReason (entryObj, msg, Q_FUNC_INFO,
				"org.LC.AdvNotifications.IM.Subscr.Revoked",
				tr ("%1 (%2) cancelled our subscription."),
				tr ("%1 (%2) cancelled our subscription: %3."));
	}

	void Core::handleItemGrantedSubscription (QObject *entryObj, const QString& msg)
	{
		if (!XmlSettingsManager::Instance ()
				.property ("NotifySubGrants").toBool ())
			return;

		NotifyWithReason (entryObj, msg, Q_FUNC_INFO,
				"org.LC.AdvNotifications.IM.Subscr.Granted",
				tr ("%1 (%2) granted subscription."),
				tr ("%1 (%2) granted subscription: %3."));
	}

	void Core::handleMUCInvitation (const QVariantMap& ident,
			const QString& inviter, const QString& reason)
	{
		IAccount *acc = qobject_cast<IAccount*> (sender ());
		if (!acc)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "doesn't implement IAccount";
			return;
		}

		const QString& name = ident ["HumanReadableName"].toString ();

		const QString str = reason.isEmpty () ?
				tr ("You have been invited to %1 by %2.")
					.arg (name)
					.arg (inviter) :
				tr ("You have been invited to %1 by %2: %3")
					.arg (name)
					.arg (inviter)
					.arg (reason);

		Entity e = Util::MakeNotification ("Azoth", str, PInfo_);
		e.Additional_ ["org.LC.AdvNotifications.SenderID"] = "org.LeechCraft.Azoth";
		e.Additional_ ["org.LC.AdvNotifications.EventCategory"] =
				"org.LC.AdvNotifications.IM";
		e.Additional_ ["org.LC.AdvNotifications.VisualPath"] = QStringList (name);
		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.Invited/" + name + '/' + inviter;
		e.Additional_ ["org.LC.AdvNotifications.EventType"] = "org.LC.AdvNotifications.IM.MUCInvitation";
		e.Additional_ ["org.LC.AdvNotifications.FullText"] = str;
		e.Additional_ ["org.LC.AdvNotifications.Count"] = 1;
		e.Additional_ ["org.LC.Plugins.Azoth.Msg"] = reason;

		const auto cancel = Util::MakeANCancel (e);

		Util::NotificationActionHandler *nh = new Util::NotificationActionHandler (e);
		nh->AddFunction (tr ("Join"), [this, acc, ident, cancel] ()
				{
					SuggestJoiningMUC (acc, ident);
					emit gotEntity (cancel);
				});
		nh->AddDependentObject (acc->GetObject ());

		emit gotEntity (e);
	}

	void Core::updateStatusIconset ()
	{
		QMap<State, Util::QIODevice_ptr> State2IconCache_;
		Q_FOREACH (ICLEntry *entry, Entry2Items_.keys ())
		{
			State state = entry->GetStatus ().State_;
			if (!State2IconCache_.contains (state))
				State2IconCache_ [state] = GetIconPathForState (state);

			Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
			{
				Util::QIODevice_ptr dev = State2IconCache_ [state];
				ItemIconManager_->SetIcon (item, dev.get ());
			}
		}
	}

	void Core::handleGroupContactsChanged ()
	{
		Q_FOREACH (ICLEntry *entry, Entry2Items_.keys ())
			if (entry->GetEntryType () == ICLEntry::ETChat)
				handleEntryGroupsChanged (GetDisplayGroups (entry), entry->GetObject ());
	}

	void Core::showVCard ()
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "sender doesn't implement ICLEntry"
					<< sender ();
			return;
		}

		entry->ShowInfo ();
	}

	void Core::updateItem ()
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< "sender doesn't implement ICLEntry"
					<< sender ();
			return;
		}

		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
			item->setText (entry->GetEntryName ());
	}

	void Core::handleClearUnreadMsgCount (QObject *object)
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (object);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< object
					<< "doesn't implement ICLEntry";
			return;
		}

		Q_FOREACH (QStandardItem *item, Entry2Items_ [entry])
		{
			item->setData (0, CLRUnreadMsgCount);
			RecalculateUnreadForParents (item);
		}

		Entity e = Util::MakeNotification ("Azoth", QString (), PInfo_);
		e.Additional_ ["org.LC.AdvNotifications.SenderID"] = "org.LeechCraft.Azoth";
		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.IncomingMessageFrom/" + entry->GetEntryID ();
		e.Additional_ ["org.LC.AdvNotifications.EventCategory"] =
				"org.LC.AdvNotifications.Cancel";

		emit gotEntity (e);

		e.Additional_ ["org.LC.AdvNotifications.EventID"] =
				"org.LC.Plugins.Azoth.AttentionDrawnBy/" + entry->GetEntryID ();

		emit gotEntity (e);
	}

	void Core::handleGotSDSession (QObject *sdObj)
	{
		ISDSession *sess = qobject_cast<ISDSession*> (sdObj);
		if (!sess)
		{
			qWarning () << Q_FUNC_INFO
					<< sdObj
					<< "is not a ISDSession";
			return;
		}

		ServiceDiscoveryWidget *w = new ServiceDiscoveryWidget;
		w->SetAccount (sender ());
		w->SetSDSession (sess);
		emit gotSDWidget (w);
	}

	void Core::handleFileOffered (QObject *jobObj)
	{
		ITransferJob *job = qobject_cast<ITransferJob*> (jobObj);
		if (!job)
		{
			qWarning () << Q_FUNC_INFO
					<< jobObj
					<< "could not be casted to ITransferJob";
			return;
		}

		const QString& id = job->GetSourceID ();
		IncreaseUnreadCount (qobject_cast<ICLEntry*> (GetEntry (id)));

		CheckFileIcon (id);
	}

	void Core::handleJobDeoffered (QObject *jobObj)
	{
		ITransferJob *job = qobject_cast<ITransferJob*> (jobObj);
		if (!job)
		{
			qWarning () << Q_FUNC_INFO
					<< jobObj
					<< "could not be casted to ITransferJob";
			return;
		}

		const QString& id = job->GetSourceID ();
		IncreaseUnreadCount (qobject_cast<ICLEntry*> (GetEntry (id)), -1);
		CheckFileIcon (id);
	}

	void Core::handleRIEXItemsSuggested (QList<RIEXItem> items, QObject *from, QString message)
	{
		RIEX::HandleRIEXItemsSuggested (items, from, message);
	}

	void Core::invalidateClientsIconCache (QObject *passedObj)
	{
		QObject *obj = passedObj ? passedObj : sender ();
		ICLEntry *entry = qobject_cast<ICLEntry*> (obj);
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< obj
					<< "could not be casted to ICLEntry";
			return;
		}

		invalidateClientsIconCache (entry);
	}

	void Core::invalidateClientsIconCache (ICLEntry *entry)
	{
		EntryClientIconCache_.remove (entry);
	}

	void Core::invalidateSmoothAvatarCache ()
	{
		ICLEntry *entry = qobject_cast<ICLEntry*> (sender ());
		if (!entry)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "could not be casted to ICLEntry";
			return;
		}

		Entry2SmoothAvatarCache_.remove (entry);
		updateItem ();
	}

	void Core::flushIconCaches ()
	{
		Q_FOREACH (std::shared_ptr<Util::ResourceLoader> rl, ResourceLoaders_.values ())
			rl->FlushCache ();
	}

#ifdef ENABLE_CRYPT
	void Core::handleQCAEvent (int id, const QCA::Event& event)
	{
		qDebug () << Q_FUNC_INFO << id << event.type ();
	}

	void Core::handleQCABusyFinished ()
	{
		Q_FOREACH (IAccount *acc, GetAccounts ())
		{
			RestoreKeyForAccount (acc);

			Q_FOREACH (QObject *entryObj, acc->GetCLEntries ())
			{
				ICLEntry *entry = qobject_cast<ICLEntry*> (entryObj);
				if (!entry)
				{
					qWarning () << Q_FUNC_INFO
							<< entry
							<< "doesn't implement ICLEntry";
					continue;
				}

				RestoreKeyForEntry (entry);
			}
		}
	}
#endif
}
}
