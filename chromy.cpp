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
#include <QTextStream>
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
    if(id->count() > 1)
    {
        CatItem item = id->first().getTopResult();
        /* bookmarks will have null .data, search engines will have their
         * url stored there. */
        if(item.id == HASH_chromy && item.data != NULL)
        {
            // This is a user search text, create an entry for it
            QString &text = id->last().getText();
            CatItem newItem(text + ".chromySearch", text, HASH_chromy, getIcon());
            newItem.data = item.data;
            results->push_front(newItem);
        }
    }
}



QString chromyPlugin::getIcon()
{
#ifdef Q_WS_WIN
    return qApp->applicationDirPath() + "/plugins/icons/chromy.png";
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
                items->push_back(CatItem(url, name, HASH_chromy, getIcon()));
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
    chromyContext context;
    context.chromy = this;
    context.items = items;
    // context.debug = &out;
    rc = sqlite3_open_v2(tmpFile.fileName().toUtf8().constData(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, 0);
    if(!rc){
        rc = sqlite3_exec(db, "select keyword, short_name, url from keywords", indexChromeCallback, (void*)&context, &zErrMsg);
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
    chromyContext *context = (chromyContext*)param;
    /* for search engines, the content being searched is the keyword,
     * and the short name is displayed.  The URL is kept around in
     * data for when the item is launched */
    QString fullName = QString(argv[0]) + " .chromy";
    QString shortName = QString(argv[1]);
    CatItem item(fullName, shortName,
                 context->chromy->HASH_chromy, context->chromy->getIcon());
    item.data = (void*)(new QString(argv[2]));
    context->items->push_back(item);
    return 0;
}

void chromyPlugin::launchItem(QList<InputData>* inputData, CatItem* item)
{
    // QFile outFile("D:\\code\\chromy\\jason.log");
    // outFile.open(QIODevice::WriteOnly);
    // QTextStream *out = new QTextStream(&outFile);
    // *out << "Launching with [" << inputData->count() << "] inputs:\n";
    // for(int i=0;i<inputData->count();i++) {
	// 	const QString txt = (*inputData)[i].getText();
    //     *out << " #" << i << " [" << txt << "]\n";
    // }
    // outFile.close();
    QString file = "";
    if(item->data == NULL) {
        // just a bookmark
        file = item->fullPath;
    } else {
        // extract the URL from data, and substitute their query
        file = *((QString*)(item->data));
        if(inputData->count() > 1) {
            QString repl = (*inputData)[1].getText();
            file.replace(QRegExp("\\{searchTerms\\}"), repl);
        }
    }
    QUrl url(file);
    runProgram(url.toString(), "");
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
    // untested, no idea if it'll work
    platformPath = "~/Library/Application Support/Google/Chrome/";
#endif
    return platformPath + "/User Data/Default/";
}

Q_EXPORT_PLUGIN2(chromy, chromyPlugin)
