#pragma once

//全局变量以及不足以成一个类的成员都会放到这
namespace Global
{
extern QString appConfigPath;
extern QString appDataPath;
extern QString appTempPath;

extern void initOnce();

extern QString configPath();
extern QString dataPath();
extern QString tempPath();

extern bool deleteDir(const QString &path);
extern qint64 getDirSize(const QString &filePath);
extern void webImageDownloadToLocal(QString &webStr);
};
