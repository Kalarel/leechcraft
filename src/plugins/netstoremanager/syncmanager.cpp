/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include "syncmanager.h"
#include <QtDebug>
#include <QStringList>
#include <QTimer>
#include <QDir>
#include <QThread>
#include "interfaces/netstoremanager/istorageaccount.h"
#include "interfaces/netstoremanager/isupportfilelistings.h"
#include "accountsmanager.h"
#include "fileswatcher.h"
#include "xmlsettingsmanager.h"
#include "utils.h"

namespace LeechCraft
{
namespace NetStoreManager
{
	SyncManager::SyncManager (AccountsManager *am, QObject *parent)
	: QObject (parent)
	, AM_ (am)
	, Timer_ (new QTimer (this))
	, Thread_ (new QThread (this))
	, FilesWatcher_ (0)
	{
// 		connect (Timer_,
// 				SIGNAL (timeout ()),
// 				this,
// 				SLOT (handleTimeout ()));

		XmlSettingsManager::Instance ().RegisterObject ("ExceptionsList",
				this, "handleUpdateExceptionsList");
	}

	void SyncManager::Release ()
	{
		if (FilesWatcher_)
			QMetaObject::invokeMethod (FilesWatcher_,
					"Release");
		Thread_->exit ();
	}

	void SyncManager::handleDirectoryAdded (const QVariantMap& dirs)
	{
		if (!FilesWatcher_)
		{
			try
			{
				FilesWatcher_ = new FilesWatcher;
				FilesWatcher_->moveToThread (Thread_);
				Thread_->start ();

				connect (FilesWatcher_,
						SIGNAL (dirWasCreated (QString)),
						this,
						SLOT (handleDirWasCreated (QString)));
				connect (FilesWatcher_,
						SIGNAL (fileWasCreated (QString)),
						this,
						SLOT (handleFileWasCreated (QString)));
				connect (FilesWatcher_,
						SIGNAL (dirWasRemoved (QString)),
						this,
						SLOT (handleDirWasRemoved (QString)));
				connect (FilesWatcher_,
						SIGNAL (fileWasRemoved (QString)),
						this,
						SLOT (handleFileWasRemoved (QString)));
				connect (FilesWatcher_,
						SIGNAL (entryWasRenamed (QString, QString)),
						this,
						SLOT (handleEntryWasRenamed (QString, QString)));
				connect (FilesWatcher_,
						SIGNAL (entryWasMoved (QString, QString)),
						this,
						SLOT (handleEntryWasMoved (QString, QString)));
				connect (FilesWatcher_,
						SIGNAL (fileWasUpdated (QString)),
						this,
						SLOT (handleFileWasUpdated (QString)));
			}
			catch (const std::exception& e)
			{
				FilesWatcher_ = 0;
				qWarning () << e.what ();
				return;
			}
		}

		handleUpdateExceptionsList ();
		for (const auto& key : dirs.keys ())
		{
			const QString& dirPath = dirs [key].toString ();
			Path2Account_ [dirPath] = AM_->GetAccountFromUniqueID (key);
			qDebug () << "watching directory "
					<< dirPath;

			QStringList pathes = Utils::ScanDir (QDir::NoDotAndDotDot | QDir::Dirs, dirPath, true);
			QMetaObject::invokeMethod (FilesWatcher_,
					"AddPath",
					Q_ARG (QString, dirPath));
			QMetaObject::invokeMethod (FilesWatcher_,
					"AddPathes",
					Q_ARG (QStringList, pathes));
			auto isfl = qobject_cast<ISupportFileListings*> (Path2Account_ [dirPath]->GetObject ());
// 			isfl->CheckForSyncUpload (pathes, dirPath);
		}

		// check for changes every minute
// 		Timer_->start (60000);
// 		handleTimeout ();
	}

// 	void SyncManager::handleDirectoryChanged (const QString& path)
// 	{
// 		QStringList pathesInDir = ScanDir (path, false);
// 		qDebug () << "pathes in dir" << pathesInDir;
// // 		QStringList watchedFiles = WatchedPathes_;
// // 		QDir dir (path);
// //
// // 		QStringList baseDirs;
// // 		for (const auto& str : Path2Account_.keys ())
// // 			if (path.contains (str))
// // 			{
// // 				baseDirs << str;
// // 				WatchedPathes_ << ScanDir (str);
// // 			}
// //
// // 		//check for removed files
// // 		QStringList removedFiles;
// // 		std::set_difference (watchedFiles.begin (), watchedFiles.end (),
// // 				WatchedPathes_.begin (), WatchedPathes_.end (),
// // 				std::back_inserter (removedFiles));
// //
// // 		//check for adding files
// // 		for (const auto& info : dir.entryInfoList (QDir::AllEntries | QDir::NoDotAndDotDot))
// // 		{
// // 			if (!watchedFiles.contains (info.absoluteFilePath ()))
// // 				FileSystemWatcher_->addPath (info.absoluteFilePath ());
// // 		}
// //
// // 		for (const auto& str : Path2Account_.keys ())
// // 		{
// // 			if (path.contains (str))
// // 			{
// // 				auto isfl = qobject_cast<ISupportFileListings*> (Path2Account_ [str]->GetObject ());
// // 				isfl->CheckForSyncUpload (FileSystemWatcher_->files (), str);
// // 			}
// // 		}
// 	}

	void SyncManager::handleTimeout ()
	{
		for (auto account : Path2Account_.values ())
		{
			if (!(account->GetAccountFeatures () & FileListings))
				continue;

			auto isfl = qobject_cast<ISupportFileListings*> (account->GetObject ());
			isfl->RequestFileChanges ();
		}
	}

	void SyncManager::handleUpdateExceptionsList ()
	{
		QStringList masks = XmlSettingsManager::Instance ()
				.property ("ExceptionsList").toStringList ();

		if (FilesWatcher_)
			QMetaObject::invokeMethod (FilesWatcher_,
					"UpdateExceptions",
					Q_ARG (QStringList, masks));
	}

		void SyncManager::handleDirWasCreated (const QString& path)
		{
			for (const auto& basePath : Path2Account_.keys ())
			{
				if (!path.startsWith (basePath))
					continue;

				auto isfl = qobject_cast<ISupportFileListings*> (Path2Account_ [basePath]->GetObject ());
				if (!isfl)
				{
					qWarning () << Q_FUNC_INFO
							<< Path2Account_ [basePath]->GetObject ()
							<< "isn't an ISupportFileListings";
					continue;
				}

// 				isfl->CreateDirectory ();
			}
		}

		void SyncManager::handleFileWasCreated (const QString& path)
		{
			for (const auto& basePath : Path2Account_.keys ())
			{
				if (!path.startsWith (basePath))
					continue;


			}
		}

		void SyncManager::handleDirWasRemoved (const QString& path)
		{
			for (const auto& basePath : Path2Account_.keys ())
			{
				if (!path.startsWith (basePath))
					continue;


			}
		}

		void SyncManager::handleFileWasRemoved (const QString& path)
		{
			for (const auto& basePath : Path2Account_.keys ())
			{
				if (!path.startsWith (basePath))
					continue;


			}
		}

		void SyncManager::handleEntryWasRenamed (const QString& oldPath, const QString& newPath)
		{
		}

		void SyncManager::handleEntryWasMoved (const QString& oldPath, const QString& newPath)
		{
		}

		void SyncManager::handleFileWasUpdated (const QString& path)
		{
			for (const auto& basePath : Path2Account_.keys ())
			{
				if (!path.startsWith (basePath))
					continue;


			}
		}

}
}

