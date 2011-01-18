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

#ifndef PLUGINS_AZOTH_INTERFACES_AZOTHCOMMON_H
#define PLUGINS_AZOTH_INTERFACES_AZOTHCOMMON_H
#include <QMetaType>

namespace LeechCraft
{
namespace Plugins
{
namespace Azoth
{
namespace Plugins
{
	enum State
	{
		SOnline,
		SChat,
		SAway,
		SDND,
		SXA,
		SOffline,
		SProbe,
		SError,
		SInvalid
	};

	inline bool IsLess (State s1, State s2)
	{
		return static_cast<int> (s1) < static_cast<int> (s2);
	}

	/** Represents possible state of authorizations between two
	 * entities: our user and a remote contact.
	 *
	 * Modelled after RFC 3921, Section 9.
	 */
	enum AuthStatus
	{
		/** Contact and user are not subscribed to each other, and
		 * neither has requested a subscription from the other.
		 */
		ASNone,

		/** Contact and user are not subscribed to each other, and
		 * user has sent contact a subscription request but contact has
		 * not replied yet.
		 */
		ASNoneOut,

		/** Contact and user are not subscribed to each other, and
		 * contact has sent user a subscription request but user has not
		 * replied yet.
		 */
		ASNoneIn,

		/** Contact and user are not subscribed to each other, contact
		 * has sent user a subscription request but user has not replied
		 * yet, and user has sent contact a subscription request but
		 * contact has not replied yet.
		 */
		ASNoneOutIn,

		/** User is subscribed to contact (one-way).
		 */
		ASTo,

		/** User is subscribed to contact, and contact has sent user a
		 * subscription request but user has not replied yet.
		 */
		ASToIn,

		/** Contact is subscribed to user (one-way).
		 */
		ASFrom,

		/** Contact is subscribed to user, and user has sent contact a
		 * subscription request but contact has not replied yet.
		 */
		ASFromIn,

		/** User and contact are subscribed to each other (two-way).
		 */
		ASBoth
	};
}
}
}
}

Q_DECLARE_METATYPE (LeechCraft::Plugins::Azoth::Plugins::State);

#endif
