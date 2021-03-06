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

#include "infocontroller.h"

#include "atools.h"
#include "common/constants.h"
#include "common/htmlinfobuilder.h"
#include "gui/mainwindow.h"
#include "gui/widgetstate.h"
#include "mapgui/mapquery.h"
#include "route/routecontroller.h"
#include "settings/settings.h"
#include "ui_mainwindow.h"
#include "util/htmlbuilder.h"
#include "options/optiondata.h"

#include <QDebug>
#include <QScrollBar>
#include <QUrlQuery>

using atools::util::HtmlBuilder;

InfoController::InfoController(MainWindow *parent, MapQuery *mapDbQuery, InfoQuery *infoDbQuery) :
  QObject(parent), mainWindow(parent), mapQuery(mapDbQuery), infoQuery(infoDbQuery)
{
  iconBackColor = QApplication::palette().color(QPalette::Active, QPalette::Base);

  info = new HtmlInfoBuilder(mapQuery, infoQuery, true);

  Ui::MainWindow *ui = mainWindow->getUi();
  infoFontPtSize = static_cast<float>(ui->textBrowserAirportInfo->font().pointSizeF());
  simInfoFontPtSize = static_cast<float>(ui->textBrowserAircraftInfo->font().pointSizeF());

  // Set search path to silence text browser warnings
  QStringList paths({QApplication::applicationDirPath()});
  ui->textBrowserAirportInfo->setSearchPaths(paths);
  ui->textBrowserRunwayInfo->setSearchPaths(paths);
  ui->textBrowserComInfo->setSearchPaths(paths);
  ui->textBrowserApproachInfo->setSearchPaths(paths);
  ui->textBrowserNavaidInfo->setSearchPaths(paths);
  ui->textBrowserAircraftInfo->setSearchPaths(paths);
  ui->textBrowserAircraftProgressInfo->setSearchPaths(paths);

  // Create connections for "Map" links in text browsers
  connect(ui->textBrowserAirportInfo, &QTextBrowser::anchorClicked, this, &InfoController::anchorClicked);
  connect(ui->textBrowserRunwayInfo, &QTextBrowser::anchorClicked, this, &InfoController::anchorClicked);
  connect(ui->textBrowserComInfo, &QTextBrowser::anchorClicked, this, &InfoController::anchorClicked);
  connect(ui->textBrowserApproachInfo, &QTextBrowser::anchorClicked, this, &InfoController::anchorClicked);
  connect(ui->textBrowserNavaidInfo, &QTextBrowser::anchorClicked, this, &InfoController::anchorClicked);

  connect(ui->textBrowserAircraftInfo, &QTextBrowser::anchorClicked, this, &InfoController::anchorClicked);
  connect(ui->textBrowserAircraftProgressInfo, &QTextBrowser::anchorClicked, this,
          &InfoController::anchorClicked);
}

InfoController::~InfoController()
{
  delete info;
}

/* User clicked on "Map" link in text browsers */
void InfoController::anchorClicked(const QUrl& url)
{
  qDebug() << "InfoController::anchorClicked" << url;

  if(url.scheme() == "lnm" && url.host() == "show")
  {
    QUrlQuery query(url);

    if(query.hasQueryItem("lonx") && query.hasQueryItem("laty"))
    {
      emit showPos(atools::geo::Pos(query.queryItemValue("lonx").toFloat(),
                                    query.queryItemValue("laty").toFloat()), -1);
    }
    else if(query.hasQueryItem("id") && query.hasQueryItem("type"))
    {
      // Only airport used for id variable
      maptypes::MapAirport airport;
      mapQuery->getAirportById(airport, query.queryItemValue("id").toInt());
      emit showRect(airport.bounding);
    }
  }
}

void InfoController::saveState()
{
  Ui::MainWindow *ui = mainWindow->getUi();
  atools::gui::WidgetState(lnm::INFOWINDOW_WIDGET).save({ui->tabWidgetInformation, ui->tabWidgetAircraft});

  // Store currently shown map objects in a string list containing id and type
  maptypes::MapObjectRefList refs;
  for(const maptypes::MapAirport& airport  : currentSearchResult.airports)
    refs.append({airport.id, maptypes::AIRPORT});
  for(const maptypes::MapVor& vor : currentSearchResult.vors)
    refs.append({vor.id, maptypes::VOR});
  for(const maptypes::MapNdb& ndb : currentSearchResult.ndbs)
    refs.append({ndb.id, maptypes::NDB});
  for(const maptypes::MapWaypoint& waypoint : currentSearchResult.waypoints)
    refs.append({waypoint.id, maptypes::WAYPOINT});
  for(const maptypes::MapAirway& airway : currentSearchResult.airways)
    refs.append({airway.id, maptypes::AIRWAY});

  QStringList refList;
  for(const maptypes::MapObjectRef& ref : refs)
    refList.append(QString("%1;%2").arg(ref.id).arg(ref.type));
  atools::settings::Settings::instance().setValue(lnm::INFOWINDOW_CURRENTMAPOBJECTS, refList.join(";"));
}

void InfoController::restoreState()
{
  QString refsStr = atools::settings::Settings::instance().valueStr(lnm::INFOWINDOW_CURRENTMAPOBJECTS);
  QStringList refsStrList = refsStr.split(";", QString::SkipEmptyParts);

  // Go through the string and collect all objects in the MapSearchResult
  maptypes::MapSearchResult res;
  for(int i = 0; i < refsStrList.size(); i += 2)
    mapQuery->getMapObjectById(res,
                               maptypes::MapObjectTypes(refsStrList.at(i + 1).toInt()),
                               refsStrList.at(i).toInt());

  updateTextEditFontSizes();
  showInformation(res);

  Ui::MainWindow *ui = mainWindow->getUi();
  atools::gui::WidgetState(lnm::INFOWINDOW_WIDGET).restore({ui->tabWidgetInformation, ui->tabWidgetAircraft});
}

void InfoController::updateAirport()
{
  if(databaseLoadStatus)
    return;

  qDebug() << "InfoController::updateAirport";

  if(!currentSearchResult.airports.isEmpty())
  {
    HtmlBuilder html(true);
    maptypes::MapAirport ap;
    mapQuery->getAirportById(ap, currentSearchResult.airports.first().id);

    info->airportText(ap, html,
                      &mainWindow->getRouteController()->getRouteMapObjects(),
                      mainWindow->getWeatherReporter(), iconBackColor);
    mainWindow->getUi()->textBrowserAirportInfo->setText(html.getHtml());
  }
}

void InfoController::clearInfoTextBrowsers()
{
  Ui::MainWindow *ui = mainWindow->getUi();

  ui->textBrowserAirportInfo->clear();
  ui->textBrowserRunwayInfo->clear();
  ui->textBrowserComInfo->clear();
  ui->textBrowserApproachInfo->clear();
  ui->textBrowserNavaidInfo->clear();
}

void InfoController::showInformation(maptypes::MapSearchResult result)
{
  qDebug() << "InfoController::showInformation";

  HtmlBuilder html(true);

  Ui::MainWindow *ui = mainWindow->getUi();
  int idx = ui->tabWidgetInformation->currentIndex();

  if(!result.airports.isEmpty())
  {
    // Only one airport shown
    const maptypes::MapAirport& airport = result.airports.first();

    currentSearchResult.airports.clear();
    currentSearchResult.airportIds.clear();
    // Remember one airport
    currentSearchResult.airports.append(airport);

    // If no airport related tab is shown bring airport tab to front
    if(idx != ic::AIRPORT && idx != ic::RUNWAYS && idx != ic::COM && idx != ic::APPROACHES)
      ui->tabWidgetInformation->setCurrentIndex(ic::AIRPORT);

    updateAirport();

    html.clear();
    info->runwayText(airport, html, iconBackColor);
    ui->textBrowserRunwayInfo->setText(html.getHtml());

    html.clear();
    info->comText(airport, html, iconBackColor);
    ui->textBrowserComInfo->setText(html.getHtml());

    html.clear();
    info->approachText(airport, html, iconBackColor);
    ui->textBrowserApproachInfo->setText(html.getHtml());
  }

  if(!result.vors.isEmpty() || !result.ndbs.isEmpty() || !result.waypoints.isEmpty() ||
     !result.airways.isEmpty())
  {
    // if any navaids are to be shown clear search result before
    currentSearchResult.vors.clear();
    currentSearchResult.vorIds.clear();
    currentSearchResult.ndbs.clear();
    currentSearchResult.ndbIds.clear();
    currentSearchResult.waypoints.clear();
    currentSearchResult.waypointIds.clear();
    currentSearchResult.airways.clear();
  }

  html.clear();
  for(const maptypes::MapVor& vor : result.vors)
  {
    currentSearchResult.vors.append(vor);

    if(result.airports.isEmpty())
      // If the result does not contain an airport bring navaid tab to front
      ui->tabWidgetInformation->setCurrentIndex(ic::NAVAID);

    info->vorText(vor, html, iconBackColor);
    ui->textBrowserNavaidInfo->setText(html.getHtml());
  }

  for(const maptypes::MapNdb& ndb : result.ndbs)
  {
    currentSearchResult.ndbs.append(ndb);

    if(result.airports.isEmpty())
      ui->tabWidgetInformation->setCurrentIndex(ic::NAVAID);
    info->ndbText(ndb, html, iconBackColor);
    ui->textBrowserNavaidInfo->setText(html.getHtml());
  }

  for(const maptypes::MapWaypoint& waypoint : result.waypoints)
  {
    currentSearchResult.waypoints.append(waypoint);

    if(result.airports.isEmpty())
      ui->tabWidgetInformation->setCurrentIndex(ic::NAVAID);
    info->waypointText(waypoint, html, iconBackColor);
    ui->textBrowserNavaidInfo->setText(html.getHtml());
  }

  for(const maptypes::MapAirway& airway : result.airways)
  {
    currentSearchResult.airways.append(airway);

    if(result.airports.isEmpty())
      ui->tabWidgetInformation->setCurrentIndex(ic::NAVAID);
    info->airwayText(airway, html);
    ui->textBrowserNavaidInfo->setText(html.getHtml());
  }

  idx = ui->tabWidgetInformation->currentIndex();
  if(idx == ic::NAVAID)
    mainWindow->setStatusMessage(tr("Showing information for navaid."));
  else if(idx == ic::AIRPORT || idx == ic::RUNWAYS || idx == ic::COM || idx == ic::APPROACHES)
    mainWindow->setStatusMessage(tr("Showing information for airport."));
}

void InfoController::preDatabaseLoad()
{
  // Clear current airport and navaids result
  currentSearchResult = maptypes::MapSearchResult();
  databaseLoadStatus = true;
  clearInfoTextBrowsers();
}

void InfoController::postDatabaseLoad()
{
  databaseLoadStatus = false;
}

void InfoController::dataPacketReceived(atools::fs::sc::SimConnectData data)
{
  Ui::MainWindow *ui = mainWindow->getUi();

  if(!databaseLoadStatus)
  {
    if(!lastSimData.getPosition().isValid() ||
       // !lastSimData.getPosition().fuzzyEqual(data.getPosition(), atools::geo::Pos::POS_EPSILON_10M) ||
       atools::almostNotEqual(QDateTime::currentDateTime().toMSecsSinceEpoch(),
                              lastSimUpdate, static_cast<qint64>(MIN_SIM_UPDATE_TIME_MS)))
    {
      // Last update was more than 500 ms ago
      HtmlBuilder html(true /* has background color */);

      if(ui->dockWidgetAircraft->isVisible())
      {
        if(canTextEditUpdate(ui->textBrowserAircraftInfo))
        {
          // ok - scrollbars not pressed
          info->aircraftText(data, html);
          updateTextEdit(ui->textBrowserAircraftInfo, html.getHtml());
        }

        if(canTextEditUpdate(ui->textBrowserAircraftProgressInfo))
        {
          // ok - scrollbars not pressed
          html.clear();
          info->aircraftProgressText(data, html, mainWindow->getRouteController()->getRouteMapObjects());
          updateTextEdit(ui->textBrowserAircraftProgressInfo, html.getHtml());
        }
      }
      lastSimData = data;
      lastSimUpdate = QDateTime::currentDateTime().toMSecsSinceEpoch();
    }
  }
}

/* @return true if no scrollbar is pressed in the text edit */
bool InfoController::canTextEditUpdate(const QTextEdit *textEdit)
{
  // Do not update if scrollbar is clicked
  return !textEdit->verticalScrollBar()->isSliderDown() &&
         !textEdit->horizontalScrollBar()->isSliderDown();
}

/* Update text edit and keep selection and scrollbar position */
void InfoController::updateTextEdit(QTextEdit *textEdit, const QString& text)
{
  // Remember cursor position
  QTextCursor cursor = textEdit->textCursor();
  int pos = cursor.position();
  int anchor = cursor.anchor();

  // Remember scrollbar position
  int vScrollPos = textEdit->verticalScrollBar()->value();
  int hScrollPos = textEdit->horizontalScrollBar()->value();
  textEdit->setText(text);

  if(anchor != pos)
  {
    // There is a selection - Reset cursor
    int maxPos = textEdit->document()->characterCount() - 1;

    // Probably the document changed its size
    anchor = std::min(maxPos, anchor);
    pos = std::min(maxPos, pos);

    // Create selection again
    cursor.setPosition(anchor, QTextCursor::MoveAnchor);
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    textEdit->setTextCursor(cursor);
  }

  // Reset scroll bars
  textEdit->verticalScrollBar()->setValue(vScrollPos);
  textEdit->horizontalScrollBar()->setValue(hScrollPos);
}

void InfoController::connectedToSimulator()
{
  Ui::MainWindow *ui = mainWindow->getUi();
  ui->textBrowserAircraftInfo->setText(tr("Connected. Waiting for update."));
  ui->textBrowserAircraftProgressInfo->setText(tr("Connected. Waiting for update."));
}

void InfoController::disconnectedFromSimulator()
{
  Ui::MainWindow *ui = mainWindow->getUi();
  ui->textBrowserAircraftInfo->setText(tr("Disconnected."));
  ui->textBrowserAircraftProgressInfo->setText(tr("Disconnected."));
}

void InfoController::optionsChanged()
{
  updateTextEditFontSizes();
  showInformation(currentSearchResult);
}

/* Update font size in text browsers if options have changed */
void InfoController::updateTextEditFontSizes()
{
  Ui::MainWindow *ui = mainWindow->getUi();

  int sizePercent = OptionData::instance().getGuiInfoTextSize();
  setTextEditFontSize(ui->textBrowserAirportInfo, infoFontPtSize, sizePercent);
  setTextEditFontSize(ui->textBrowserRunwayInfo, infoFontPtSize, sizePercent);
  setTextEditFontSize(ui->textBrowserComInfo, infoFontPtSize, sizePercent);
  setTextEditFontSize(ui->textBrowserApproachInfo, infoFontPtSize, sizePercent);
  setTextEditFontSize(ui->textBrowserNavaidInfo, infoFontPtSize, sizePercent);

  sizePercent = OptionData::instance().getGuiInfoSimSize();
  setTextEditFontSize(ui->textBrowserAircraftInfo, simInfoFontPtSize, sizePercent);
  setTextEditFontSize(ui->textBrowserAircraftProgressInfo, simInfoFontPtSize, sizePercent);
}

/* Set font size in text edit based on percent of original size */
void InfoController::setTextEditFontSize(QTextEdit *textEdit, float origSize, int percent)
{
  QFont f = textEdit->font();
  float newSize = origSize * percent / 100.f;
  if(newSize > 0.1f)
  {
    f.setPointSizeF(newSize);
    textEdit->setFont(f);
  }
}
