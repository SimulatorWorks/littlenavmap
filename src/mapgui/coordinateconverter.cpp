/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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
*****************************************************************************/
#include "coordinateconverter.h"

#include "geo/pos.h"

#include <marble/GeoDataCoordinates.h>
#include <marble/ViewportParams.h>

using namespace Marble;
using namespace atools::geo;

CoordinateConverter::CoordinateConverter(const ViewportParams *viewportParams)
  : viewport(viewportParams)
{

}

bool CoordinateConverter::wToS(const atools::geo::Pos& coords, int& x, int& y)
{
  qreal xr, yr;
  bool hidden = false;
  bool visible = viewport->screenCoordinates(
    Marble::GeoDataCoordinates(coords.getLonX(), coords.getLatY(), 0,
                               GeoDataCoordinates::Degree), xr, yr, hidden);

  x = static_cast<int>(std::round(xr));
  y = static_cast<int>(std::round(yr));
  return visible && !hidden;
}

bool CoordinateConverter::wToS(const atools::geo::Pos& coords, float& x, float& y)
{
  qreal xr, yr;
  bool hidden = false;
  bool visible = viewport->screenCoordinates(
    Marble::GeoDataCoordinates(coords.getLonX(), coords.getLatY(), 0,
                               GeoDataCoordinates::Degree), xr, yr, hidden);

  x = static_cast<float>(xr);
  y = static_cast<float>(yr);
  return visible && !hidden;
}

bool CoordinateConverter::wToS(const atools::geo::Pos& coords, double& x, double& y)
{
  bool hidden = false;
  bool visible = viewport->screenCoordinates(
    Marble::GeoDataCoordinates(coords.getLonX(), coords.getLatY(), 0,
                               GeoDataCoordinates::Degree), x, y, hidden);
  return visible && !hidden;
}

bool CoordinateConverter::wToS(const Marble::GeoDataCoordinates& coords, int& x, int& y)
{
  qreal xr, yr;
  bool hidden = false;
  bool visible = viewport->screenCoordinates(coords, xr, yr, hidden);

  x = static_cast<int>(std::round(xr));
  y = static_cast<int>(std::round(yr));
  return visible && !hidden;
}

QPoint CoordinateConverter::wToS(const atools::geo::Pos& coords, bool *visible)
{
  int xr, yr;
  bool isVisible = wToS(coords, xr, yr);

  if(visible != nullptr)
  {
    *visible = isVisible;
    return QPoint(xr, yr);
  }
  else
  {
    if(isVisible)
      return QPoint(xr, yr);
    else
      return QPoint();
  }
}

QPoint CoordinateConverter::wToS(const Marble::GeoDataCoordinates& coords, bool *visible)
{
  int xr, yr;
  bool isVisible = wToS(coords, xr, yr);

  if(visible != nullptr)
  {
    *visible = isVisible;
    return QPoint(xr, yr);
  }
  else
  {
    if(isVisible)
      return QPoint(xr, yr);
    else
      return QPoint();
  }
}

bool CoordinateConverter::sToW(int x, int y, Marble::GeoDataCoordinates& coords)
{
  qreal lon, lat;

  bool visible = viewport->geoCoordinates(x, y, lon, lat, GeoDataCoordinates::Degree);

  coords.setLongitude(lon, GeoDataCoordinates::Degree);
  coords.setLatitude(lat, GeoDataCoordinates::Degree);
  return visible;
}