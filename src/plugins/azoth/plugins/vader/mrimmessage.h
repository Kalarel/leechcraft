/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2011  Georg Rudoy
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

#ifndef PLUGINS_AZOTH_PLUGINS_VADER_MRIMMESSAGE_H
#define PLUGINS_AZOTH_PLUGINS_VADER_MRIMMESSAGE_H
#include <QObject>
#include <interfaces/imessage.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Vader
{
	class MRIMBuddy;
	class MRIMAccount;

	class MRIMMessage : public QObject
					  , public IMessage
	{
		Q_OBJECT
		Q_INTERFACES (LeechCraft::Azoth::IMessage);

		MRIMBuddy *Buddy_;
		MRIMAccount *A_;
		Direction Dir_;
		MessageType MT_;

		QString Body_;
		QDateTime DateTime_;

		quint32 SendID_;
	public:
		MRIMMessage (Direction, MessageType, MRIMBuddy*);

		// ICLEntry
		QObject* GetObject ();
		void Send ();
		void Store ();
		Direction GetDirection () const;
		MessageType GetMessageType () const;
		MessageSubType GetMessageSubType () const;
		QObject* OtherPart () const;
		QString GetOtherVariant () const;
		QString GetBody () const;
		void SetBody (const QString&);
		QDateTime GetDateTime () const;
		void SetDateTime (const QDateTime&);
	private slots:
		void checkMessageDelivery (quint32);
	};
}
}
}

#endif