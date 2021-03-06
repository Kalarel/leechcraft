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

#ifndef PLUGINS_AZOTH_PLUGINS_ZHEET_CORE_H
#define PLUGINS_AZOTH_PLUGINS_ZHEET_CORE_H
#include <QObject>

namespace LeechCraft
{
struct Entity;

namespace Azoth
{
class IProxyObject;

namespace Zheet
{
	class MSNProtocol;

	class Core : public QObject
	{
		Q_OBJECT

		MSNProtocol *Protocol_;
		IProxyObject *ProxyObject_;

		Core ();
	public:
		static Core& Instance ();
		void SecondInit ();

		void SetPluginProxy (QObject*);
		IProxyObject* GetPluginProxy () const;

		MSNProtocol* GetProtocol () const;

		void SendEntity (const Entity&);
	signals:
		void gotEntity (const LeechCraft::Entity&);
	};
}
}
}

#endif
