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

#pragma once

#include <QWidget>
#include "ui_biowidget.h"

class QStandardItemModel;
class QStandardItem;

namespace Media
{
	struct ArtistBio;
	struct AlbumInfo;
	class IArtistBioFetcher;
}

namespace LeechCraft
{
namespace LMP
{
	class BioPropProxy;

	class BioWidget : public QWidget
	{
		Q_OBJECT

		Ui::BioWidget Ui_;

		QList<Media::IArtistBioFetcher*> Providers_;
		QString CurrentArtist_;

		BioPropProxy *BioPropProxy_;
		QStandardItemModel *DiscoModel_;
	public:
		BioWidget (QWidget* = 0);

		void SetCurrentArtist (const QString&);
	private:
		QStandardItem* FindAlbumItem (const QString&) const;
		void SetAlbumImage (const QString&, const QImage&);
	private slots:
		void saveLastUsedProv ();
		void requestBiography ();

		void handleBioReady ();
		void handleDiscographyReady ();
		void handleAlbumArt (const Media::AlbumInfo&, const QList<QImage>&);
		void handleImageScaled ();

		void handleLink (const QString&);
	signals:
		void gotArtistImage (const QString&, const QUrl&);
	};
}
}
