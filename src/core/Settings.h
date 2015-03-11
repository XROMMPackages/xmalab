#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>
#include <vector>

#define UI_VERSION 0

namespace xma{
	class Settings{

	public:
		static Settings * getInstance();
		virtual ~Settings();

		bool getBoolSetting(QString name);
		int getIntSetting(QString name);
		float getFloatSetting(QString name);
		QString getQStringSetting(QString name);

		void set(QString name, bool value);
		void set(QString name, int value);
		void set(QString name, float value);
		void set(QString name, QString value);

		//Special Settings
		//Filename
		void setLastUsedDirectory(QString Filename, bool directory = false);
		QString getLastUsedDirectory();

		//UI QByteArray
		void setUIGeometry(QString windowTitle, QByteArray geometry);
		QByteArray getUIGeometry(QString windowTitle);
		void setUIState(QString windowTitle, QByteArray state);
		QByteArray getUIState(QString windowTitle);

	private:
		Settings();
		static Settings * instance;

		std::vector < std::pair <QString, bool> > booleanSettings;
		std::vector < std::pair <QString, int> > intSettings;
		std::vector < std::pair <QString, float> > floatSettings;
		std::vector < std::pair <QString, QString> > qstringSettings;

		void addBoolSetting(QString name, bool defaultValue);
		void addIntSetting(QString name, int defaultValue);
		void addFloatSetting(QString name, float defaultValue);
		void addQStringSetting(QString name, QString defaultValue);
	};
}

	

#endif  // SETTINGS_H
