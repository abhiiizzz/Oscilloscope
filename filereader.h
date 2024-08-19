#ifndef FILEREADER_H
#define FILEREADER_H

#include <bits/stdc++.h>
#include <QDir>
#include <Common_sp_devices_read.h>
#include <mainwindow.h>

class FileReader
{
public:
    FileReader(std::vector<QDir> dirs, MainWindow *pMainWindow);
    bool hasMoreSignals();
    std::pair<std::vector<QVector<double>>,std::vector<QVector<double>>> getNextSignal();
private:
    int m_block;
    SP_Devices_DataRead_Class_v2 m_sp_card_file;
};

#endif // FILEREADER_H
