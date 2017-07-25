 TEMPLATE      = lib
 CONFIG       += plugin debug_and_release
 FORMS		   = 
 HEADERS       = plugin_interface.h chromy.h
 SOURCES       = plugin_interface.cpp chromy.cpp sqlite3.c
 TARGET		   = chromy
 
 win32 {
 	CONFIG -= embed_manifest_dll
	LIBS += shell32.lib
	INCLUDEPATH += "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\Include"
	LIBPATH += "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\Lib"
%	LIBS += user32.lib
%	LIBS += Gdi32.lib
%	LIBS += comctl32.lib
}
