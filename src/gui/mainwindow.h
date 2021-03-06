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

#ifndef LITTLENAVMAP_MAINWINDOW_H
#define LITTLENAVMAP_MAINWINDOW_H

#include "common/maptypes.h"
#include "fs/fspaths.h"

#include <QMainWindow>
#include <QUrl>
#include <marble/MarbleGlobal.h>

class SearchController;
class RouteController;
class QComboBox;
class QLabel;
class SearchBase;
class DatabaseManager;
class WeatherReporter;
class ConnectClient;
class ProfileWidget;
class InfoController;
class OptionsDialog;
class QActionGroup;

namespace Marble {
class LegendWidget;
class MarbleAboutDialog;
class ElevationModel;
}

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace gui {
class Dialog;
class ErrorHandler;
class HelpHandler;
class FileHistoryHandler;
}

}

namespace Ui {
class MainWindow;
}

class MapWidget;
class MapQuery;
class InfoQuery;

/*
 * Main window contains all instances of controllers, widgets and managment classes.
 */
class MainWindow :
  public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  virtual ~MainWindow();

  /* Get main user interface instance */
  Ui::MainWindow *getUi() const
  {
    return ui;
  }

  MapWidget *getMapWidget() const
  {
    return mapWidget;
  }

  RouteController *getRouteController() const
  {
    return routeController;
  }

  const Marble::ElevationModel *getElevationModel();

  WeatherReporter *getWeatherReporter() const
  {
    return weatherReporter;
  }

  ConnectClient *getConnectClient() const
  {
    return connectClient;
  }

  /* Update the window title after switching simulators, flight plan name or change status. */
  void updateWindowTitle();

  /* Sets the text and tooltip of the statusbar label that indicates what objects are shown on the map */
  void setMapObjectsShownMessageText(const QString& text = QString(), const QString& tooltipText = QString());

  /* Sets a general status bar message which is shared with all widgets/actions status text */
  void setStatusMessage(const QString& message);

  void setDetailLabelText(const QString& text);

  atools::fs::FsPaths::SimulatorType getCurrentSimulator();

  atools::sql::SqlDatabase *getDatabase() const;

signals:
  /* Emitted when window is shown the first time */
  void windowShown();

private:
  void connectAllSlots();
  void mainWindowShown();

  void writeSettings();
  void readSettings();
  void updateActionStates();
  void setupUi();

  void options();
  void preDatabaseLoad();
  void postDatabaseLoad(atools::fs::FsPaths::SimulatorType type);

  void updateMapHistoryActions(int minIndex, int curIndex, int maxIndex);

  void updateMapObjectsShown();

  void searchSelectionChanged(const SearchBase *source, int selected, int visible, int total);
  void routeSelectionChanged(int selected, int total);

  void routeNew();
  void routeOpen();
  void routeAppend();
  void routeOpenRecent(const QString& routeFile);
  bool routeSave();
  bool routeSaveAs();
  void routeCenter();
  void renderStatusChanged(Marble::RenderStatus status);
  void resultTruncated(maptypes::MapObjectTypes type, int truncatedTo);
  bool routeCheckForChanges();
  bool routeValidate();
  void loadNavmapLegend();
  void showNavmapLegend();
  void showMapLegend();
  void resetMessages();
  void showDatabaseFiles();

  void kmlOpenRecent(const QString& kmlFile);
  void kmlOpen();
  void kmlClear();

  void legendAnchorClicked(const QUrl& url);

  void changeMapProjection(int index);
  void changeMapTheme(int index);
  void scaleToolbar(QToolBar *toolbar, float scale);

  /* Work on the close event that also catches clicking the close button
   * in the window frame */
  virtual void closeEvent(QCloseEvent *event) override;

  /* Emit a signal windowShown after first appearance */
  virtual void showEvent(QShowEvent *event) override;

  /* Original unchanged window title */
  QString mainWindowTitle;
  SearchController *searchController = nullptr;
  RouteController *routeController = nullptr;
  atools::gui::FileHistoryHandler *routeFileHistory = nullptr, *kmlFileHistory = nullptr;
  /* URL of the inline nav map legend */
  QUrl legendUrl;

  /* Combo boxes that are added to the toolbar */
  QComboBox *mapThemeComboBox = nullptr, *mapProjectionComboBox = nullptr;

  Ui::MainWindow *ui;
  MapWidget *mapWidget = nullptr;
  ProfileWidget *profileWidget = nullptr;

  /* Status bar labels */
  QLabel *mapDistanceLabel, *mapPosLabel, *renderStatusLabel, *detailLabel, *messageLabel;

  /* List of status bar messages (currently only one) */
  QStringList statusMessages;

  /* true if database is currently switched off (i.e. the scenery library loading is open) */
  bool hasDatabaseLoadStatus = false;

  /* Dialog classes and helper classes */
  Marble::LegendWidget *legendWidget = nullptr;
  Marble::MarbleAboutDialog *marbleAbout = nullptr;
  OptionsDialog *optionsDialog = nullptr;
  atools::gui::Dialog *dialog = nullptr;
  atools::gui::ErrorHandler *errorHandler = nullptr;
  atools::gui::HelpHandler *helpHandler = nullptr;

  /* Managment and controller classes */
  DatabaseManager *databaseManager = nullptr;
  WeatherReporter *weatherReporter = nullptr;
  ConnectClient *connectClient = nullptr;
  InfoController *infoController = nullptr;

  /* Action  groups for main menu */
  QActionGroup *actionGroupMapProjection = nullptr, *actionGroupMapTheme = nullptr;

  /* Database query helpers and caches */
  MapQuery *mapQuery = nullptr;
  InfoQuery *infoQuery = nullptr;

  bool firstStart = true /* emit window shown only once after startup */,
       firstApplicationStart = false /* first starup on a system after installation */;
};

#endif // LITTLENAVMAP_MAINWINDOW_H
