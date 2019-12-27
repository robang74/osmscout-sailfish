/*
  OSMScout for SFOS
  Copyright (C) 2019 Lukas Karas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#pragma once

#include "Storage.h"

class Tracker : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY(Tracker)

signals:
  void openTrackRequested();
  void openTrackLoaded(QString trackId, QString name);
  void error(QString message);

public slots:
  // for Storage
  void init();
  void onOpenTrackLoaded(Track track, bool ok);

  // slot for UI
  void resumeTrack(QString trackId);
  void startTracking(QString collectionId, QString trackName, QString trackDescription);
  void stopTracking();

  void locationChanged(bool locationValid,
                       double lat, double lon,
                       bool horizontalAccuracyValid, double horizontalAccuracy,
                       bool elevationValid, double elevation);

public:
  Tracker();
  virtual ~Tracker() = default;

public:
  Track track;
};
