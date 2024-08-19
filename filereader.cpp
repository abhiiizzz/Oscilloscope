#include "filereader.h"

#include <QDebug>

#define NUM_CHANELS 4

FileReader::FileReader(std::vector<QDir> dirs, MainWindow *pMainWindow):
  m_block(0)
  ,m_sp_card_file(new char[MONSTER_RAWDATA_BLOCKSIZE], new long long[4000000])
{
//    for( int i=0; i<dirs.size(); i++ ) {
//        QStringList files = dirs[i].entryList(QStringList() << "*.txt", QDir::Files);

//        foreach (const QString &file, files) {
//            QString fullPath = dirs[i].absoluteFilePath(file);
//            m_filePaths[i].append(fullPath);
//        }
//    }
    if( dirs.size() ==  0 ) return;
//    char *fulliteration = new char[MONSTER_RAWDATA_BLOCKSIZE];
//    long long *timestamps = new long long[4000000];
//    SP_Devices_DataRead_Class_v2 sp_card_file(fulliteration, timestamps);
    m_sp_card_file.Set_MainWindow(pMainWindow);
    if (!m_sp_card_file.Initialise_Acquisition_FileSet(dirs[0].path().toStdString().c_str(), 0)) return;
}

bool FileReader::hasMoreSignals(){
//    QStringList files = m_filePaths[i].entryList(QStringList() << "*.txt", QDir::Files);
    const unsigned int Nblocks = m_sp_card_file.GetNIterations();
    return m_block < Nblocks;
}

std::pair<std::vector<QVector<double>>,std::vector<QVector<double>>> FileReader::getNextSignal(){
    std::vector<QVector<double>> x(NUM_CHANELS),y(NUM_CHANELS);
    if( !hasMoreSignals() ) return {x,y};

    const unsigned int Nblocks = m_sp_card_file.GetNIterations();
    printf("Number of blocks available: %d\nBlock requested: %d\n", Nblocks, m_block);

    const char *rawdatabuffer = m_sp_card_file.GetPointerIteration(m_block);
    const SP_Devices_DataBlock_Information *_sp_devices = (SP_Devices_DataBlock_Information *)(&rawdatabuffer[22]);
    const unsigned int nof_records = *(unsigned int *)&rawdatabuffer[22 + sizeof(SP_Devices_DataBlock_Information)];
    const unsigned int n=0;
    for( int channel=0; channel < NUM_CHANELS; channel++ )
    {
        const int offset = _sp_devices->iOffset[channel]; // Offset in mV
        const int delay = _sp_devices->firmware[2]=='D'? _sp_devices->iFWDAQ_Delay : _sp_devices->iFWPD_LEW[channel]; // Delay (ns)

        unsigned int buffer_npoints;
        const short *stream_of_data = (short *)m_sp_card_file.GetSegment(channel, n, buffer_npoints);

        const double tomV = m_sp_card_file.RawTomV(channel);

        for(int i=0; i<buffer_npoints; i++) {
            // i + delay, stream_of_data[i] * tomV - offset

            x[channel].push_back(i+delay);
            y[channel].push_back(stream_of_data[i]*tomV-offset);
        }
    }
    m_block++;
    return {x,y};
}
