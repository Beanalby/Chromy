/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <QtGui>
#include <QUrl>
#include <QFile>
#include <QRegExp>
#include <QTextCodec>
#include "sqlite3.h"

#ifdef Q_WS_WIN
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>

QString GetShellDirectory(int type)
{
    wchar_t buffer[_MAX_PATH];
    SHGetFolderPath(NULL, type, NULL, 0, buffer);
    return QString::fromUtf16(buffer);
}
#endif

#include "chromy.h"

chromyPlugin* gchromyInstance = NULL;

void chromyPlugin::getID(uint* id)
{
    *id = HASH_chromy;
}

void chromyPlugin::getName(QString* str)
{
    *str = PLUGIN_NAME;
}

void chromyPlugin::init()
{
}



void chromyPlugin::getLabels(QList<InputData>* id)
{
}

void chromyPlugin::getResults(QList<InputData>* id, QList<CatItem>* results)
{
}



QString chromyPlugin::getIcon()
{
#ifdef Q_WS_WIN
    return qApp->applicationDirPath() + "/plugins/icons/chromy.ico";
#endif
}

void chromyPlugin::getCatalog(QList<CatItem>* items)
{
    QString path = getChromePath();

    // do the bookmarks first
    QFile inputFile(path + "Bookmarks");
    if(!inputFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QRegExp nameRx("\"name\": \"([^\"]+)\"");
    QRegExp urlRx("\"url\": \"([^\"]+)\"");
    QString line, name, url;

    while(true)
    {
        line = inputFile.readLine();
        if(line == 0)
            break;
        if(nameRx.indexIn(line) != -1)
        {
            name = nameRx.cap(1);
            continue;
        }
        if(urlRx.indexIn(line) != -1)
        {
            url = urlRx.cap(1);
            if(name != 0 && url != 0 && !url.isEmpty() && !name.isEmpty())
                items->push_back(CatItem(url, name, 0, getIcon()));
        }
    }
    inputFile.close();

    // pull the search engines from the sqlite3 database.
    QFile tmpFile(path + "chromy.tmp.db");
    QFile webData(path + "Web Data");
    // Chrome holds "Web Data" open, make a copy for us to read
    webData.copy(tmpFile.fileName());
    sqlite3 *db;
    int rc;
    char *zErrMsg = 0;
    rc = sqlite3_open_v2(tmpFile.fileName().toUtf8().constData(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, 0);
    if(!rc){
        rc = sqlite3_exec(db, "select short_name, keyword, url from keywords", indexChromeCallback, (void*)items, &zErrMsg);
        if(rc!=SQLITE_OK) {
            // error
            // out << "Error on sql statement: " << zErrMsg << "\n";
            // sqlite3_free(zErrMsg);
        }
    }
    tmpFile.close();
    tmpFile.remove();
    if(db) {
        sqlite3_close(db);
    }
}

static int indexChromeCallback(void* param, int argc, char **argv, char **azColName){
    QList<CatItem>* items = (QList<CatItem>*)param;
    // add as a search engine, not a bookmark
    // items->push_back(CatItem(QString(argv[1]), QString(argv[2])));
    return 0;
}

void chromyPlugin::launchItem(QList<InputData>* id, CatItem* item)
{
}

void chromyPlugin::doDialog(QWidget* parent, QWidget** newDlg)
{
}

void chromyPlugin::endDialog(bool accept)
{
}

int chromyPlugin::msg(int msgId, void* wParam, void* lParam)
{
    bool handled = false;
    switch (msgId)
    {
        case MSG_INIT:
            init();
            handled = true;
            break;
        case MSG_GET_LABELS:
            getLabels((QList<InputData>*) wParam);
            handled = true;
            break;
        case MSG_GET_ID:
            getID((uint*) wParam);
            handled = true;
            break;
        case MSG_GET_NAME:
            getName((QString*) wParam);
            handled = true;
            break;
        case MSG_GET_RESULTS:
            getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
            handled = true;
            break;
        case MSG_GET_CATALOG:
            getCatalog((QList<CatItem>*) wParam);
            handled = true;
            break;
        case MSG_LAUNCH_ITEM:
            launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
            handled = true;
            break;
        case MSG_HAS_DIALOG:
            // Set to true if you provide a gui
            handled = false;
            break;
        case MSG_DO_DIALOG:
            // This isn't called unless you return true to MSG_HAS_DIALOG
            doDialog((QWidget*) wParam, (QWidget**) lParam);
            break;
        case MSG_END_DIALOG:
            // This isn't called unless you return true to MSG_HAS_DIALOG
            endDialog((bool) wParam);
            break;

        default:
            break;
    }
    return handled;
}

QString chromyPlugin::getChromePath()
{
    QString platformPath;
#ifdef Q_WS_WIN
    platformPath  = GetShellDirectory(CSIDL_LOCAL_APPDATA) + "/Google/Chrome/";
#endif

#ifdef Q_WS_X11
    // todo - what's the proper path?
#endif

#ifdef Q_WS_MAC
    // todo - what's the proper path?
#endif
    return platformPath + "/User Data/Default/";
}

Q_EXPORT_PLUGIN2(chromy, chromyPlugin)
