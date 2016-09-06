//  ----------------------------------
//  XMALab -- Copyright © 2015, Brown University, Providence, RI.
//  
//  All Rights Reserved
//   
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3 
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided 
//  that this copyright notice appear in all copies and that the name of Brown University not be used in 
//  advertising or publicity pertaining to the use or distribution of the software without specific written 
//  prior permission from Brown University.
//  
//  See license.txt for further information.
//  
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS 
//  PROVIDED “AS IS”, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY 
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING 
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION 
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
//  ----------------------------------
//  
///\file Settings.h
///\author Benjamin Knorlein
///\date 11/20/2015

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>
#include <vector>

#define UI_VERSION 0

namespace xma
{
	class Settings
	{
	public:
		static Settings* getInstance();
		virtual ~Settings();

		bool getBoolSetting(QString name);
		int getIntSetting(QString name);
		float getFloatSetting(QString name);
		QString getQStringSetting(QString name);
		QStringList Settings::getQStringListSetting(QString name);

		void set(QString name, bool value);
		void set(QString name, int value);
		void set(QString name, float value);
		void set(QString name, QString value);
		void set(QString name, QStringList value);

		//Special Settings
		//Filename
		void setLastUsedDirectory(QString Filename, bool directory = false);
		QString getLastUsedDirectory();

		void addToRecentFiles(QString filename);

		//UI QByteArray
		void setUIGeometry(QString windowTitle, QByteArray geometry);
		QByteArray getUIGeometry(QString windowTitle);
		void setUIState(QString windowTitle, QByteArray state);
		QByteArray getUIState(QString windowTitle);

	private:
		Settings();
		static Settings* instance;

		std::vector<std::pair<QString, bool> > booleanSettings;
		std::vector<std::pair<QString, int> > intSettings;
		std::vector<std::pair<QString, float> > floatSettings;
		std::vector<std::pair<QString, QString> > qstringSettings;
		std::vector<std::pair<QString, QStringList> > qstringListSettings;

		void addBoolSetting(QString name, bool defaultValue);
		void addIntSetting(QString name, int defaultValue);
		void addFloatSetting(QString name, float defaultValue);
		void addQStringSetting(QString name, QString defaultValue);
		void addQStringListSetting(QString name, QStringList defaultValue);
	};
}


#endif // SETTINGS_H


