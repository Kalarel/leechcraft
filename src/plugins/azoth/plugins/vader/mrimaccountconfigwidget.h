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

#ifndef PLUGINS_AZOTH_PLUGINS_VADER_MRIMACCOUNTCONFIGWIDGET_H
#define PLUGINS_AZOTH_PLUGINS_VADER_MRIMACCOUNTCONFIGWIDGET_H
#include <QWidget>
#include "ui_mrimaccountconfigwidget.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Vader
{
	class MRIMAccountConfigWidget : public QWidget
	{
		Q_OBJECT

		Ui::MRIMAccountConfigWidget Ui_;
	public:
		MRIMAccountConfigWidget (QWidget* = 0);

		QString GetLogin () const;
		QString GetPassword () const;
	};
}
}
}

#endif