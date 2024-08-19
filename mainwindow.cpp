#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <fstream>
#include <bits/stdc++.h>
#include <QDir>
#include <QThread>
#include <memory>
#include "filereader.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , x(3,QVector<double>(1000)), y(3,QVector<double>(1000))
    , m_paused(false)
//    , m_pminValueLine(nullptr)  // Initialize maxValueLine to nullptr
    ,m_checkBox(4,false)
    ,m_minValueLines(3,nullptr)
    ,m_f()
    ,isConnected(false)
    ,m_num_channel(-1)
{
    ui->setupUi(this);




    ui->customplot->addGraph();

    ui->customplot->graph(0)->setScatterStyle(QCPScatterStyle::ssCircle);

    ui->customplot->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->customplot->xAxis->setLabel("X");
    ui->customplot->yAxis->setLabel("Y");
    ui->customplot->xAxis->setRange(0,1000);
    ui->customplot->yAxis->setRange(-200,0);

    ui->customplot->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom |  QCP::iSelectPlottables);
    ui->customplot->axisRect()->setRangeZoom(Qt::Vertical);


    m_SleepDelay=ui->horizontalSlider->value();

//    m_pminValueLine = std::make_shared<QCPItemLine>(ui->customplot);
    connect(this, &MainWindow::plotting, this, [this](int i) { this->plot_update(i); } );
    connect(this, &MainWindow::eventReceived, this, &MainWindow::on_event_Received);
    connect(this, &MainWindow::numChannelRecieved, this, [this](int numChannels){ this->initiateGui(numChannels); });
    // connect(this, &MainWindow::plotting, this, &MainWindow::draw_min_value_line);

//     connect(this,  &MainWindow::on_plot, [this](QDir directory, int i){ on_multi_plotting(directory,i); });

}


MainWindow::~MainWindow()
{
    for( int i=0; i<m_f.size(); i++ )
    {
        fclose(m_f[i]);
    }
    delete ui;

}

void MainWindow::setNumberOfChannels(int num_channel){
    if( m_num_channel >= 0 ) return;
    emit initiateGui( num_channel );
    m_num_channel = num_channel;
    m_f = std::vector<FILE*>(m_num_channel, nullptr);
    for( int i=0; i<m_f.size(); i++ )
    {
        std::string out_directory = ui->Output_Directory->text().toStdString();
        std::string fileName = "Channel_";
        fileName+=('A'+i);
        fileName+=".txt";
        fileName = out_directory+"/"+fileName;
        if( m_f[i] == nullptr )
            m_f[i] = fopen(fileName.c_str(), "wb");
    }
}

void MainWindow::initiateGui(int numChannels) {
    // QDir directory("C:\\Users\\VECC_Trainee\\Documents\\FirstWidget\\Californium_Data");
     ui->customplot->clearGraphs(); // Clear existing graphs

     // Define a color generator function
     auto generateColor = [](int index) {
         // Generate a color based on the index
         int hue = index * 40 % 360; // Vary the hue based on the index
         return QColor::fromHsv(hue, 255, 255); // Convert HSV to QColor
     };

     for( int i=0; i<numChannels; i++ ) {
             ui->customplot->addGraph(); // Add a new graph for each channel
             // Customize each graph as needed
             ui->customplot->graph(i)->setScatterStyle(QCPScatterStyle::ssCircle);
             ui->customplot->graph(i)->setLineStyle(QCPGraph::lsLine);
             ui->customplot->graph(i)->setPen(QPen(generateColor(i))); // Set color using the generator function
             ui->customplot->graph(i)->setName(QString("Channel %1").arg(i + 1));
     }

     ui->customplot->legend->setVisible(true);

     // Set legend position and styling (optional)
     ui->customplot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignRight);
     ui->customplot->legend->setBrush(QColor(255, 255, 255, 150));
     ui->customplot->legend->setBorderPen(QPen(Qt::black, 1));
     ui->customplot->legend->setFont(QFont("Helvetica", 9));
}

void MainWindow::saveData(int channel){
    int x_size = x[channel].size();
    int y_size = y[channel].size();
    for( int i=0; i<std::min(x_size,y_size); i++ )
        fprintf(m_f[channel], "%lg %lg\n", x[channel][i], y[channel][i]);
}

void MainWindow::plot_update(int i)
{
    // draw_min_value_line(i);
    if( i >= m_num_channel ) return;
    this->saveData(i);
    if( m_checkBox[i] == true )
        ui->customplot->graph(i)->setData(x[i],y[i]);
    else
        ui->customplot->graph(i)->clearData();
//    ui->customplot->graph(1)->setData(xB,yB);
//    ui->customplot->rescaleAxes();
//    if(i>0 && m_checkBox[i])
//        draw_min_value_line(i);
//    else
//        {
//            if( m_minValueLines[i]!= nullptr )
//            {
//                auto temp = m_minValueLines[i];
//                ui->customplot->removeItem(temp);
//                m_minValueLines[i]=nullptr;
//            }
//        }
    ui->customplot->replot();

    // ui->customplot->update();

}

void MainWindow::draw_min_value_line(int i)
{
    qDebug() << "Drawing min value line for graph" << i;

    if (i < 0 || i >= y.size()) {
        qDebug() << "Invalid graph index:" << i;
        return;
    }

    // If the line for this graph doesn't exist, create it
    if (m_minValueLines[i] == nullptr) {
        m_minValueLines[i] = new QCPItemLine(ui->customplot);
        ui->customplot->addItem(m_minValueLines[i]);
        m_minValueLines[i]->setPen(QPen(Qt::red)); // Set color for the minimum value line
    }

    // Find the minimum value and its corresponding index
    auto minIter = std::min_element(y[i].begin(), y[i].end());
    if (minIter != y[i].end()) {
        int minIndex = std::distance(y[i].begin(), minIter);
        double minValue = *minIter;

        qDebug() << "Min value:" << minValue << "at index:" << minIndex;

        // Update the position of the line
        m_minValueLines[i]->start->setCoords(minIndex, ui->customplot->yAxis->range().lower);
        m_minValueLines[i]->end->setCoords(minIndex, ui->customplot->yAxis->range().upper);

        qDebug() << "Vertical line added at:" << minIndex;
    } else {
        qDebug() << "No min value found in y vector";
    }
}


void MainWindow::on_plotbutton_clicked()
{
    QString directoryPath = ui->Input_Directory->text();
    QDir directory(directoryPath);
    if(directoryPath==""){
        QMessageBox::critical(this, tr("Error"), tr("Please Enter Input Directory: "));
                return;
    }
    QString Out_DirectoryPath = ui->Output_Directory->text();
    if(Out_DirectoryPath==""){
        QMessageBox::critical(this, tr("Error"), tr("Please Enter Output Directory: "));
                return;
    }
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Do you want to restart oscilloscope?", (std::string("The files in directory: ")+ui->Output_Directory->text().toStdString()+std::string(" will be reset. Close all process relating to the files in the directory and restart them after pressing YES. Press NO if you want to continue.")).c_str(),
                                    QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) return;
    // Close the previous files list
    for( int i=0; i<m_f.size(); i++ )
    {
        fclose(m_f[i]);
        m_f[i] = nullptr;
    }
    m_num_channel = -1;
    ui->m_eventCountSpinBox->setValue(0);
    std::vector<QDir> dirs;
    dirs.push_back(directory);

    FileReader fileReader(dirs,this); // pass the this variable here FileReader fileReader(dirs, this);
//     m_sp_card_file.Set_MainWindow(this);
//     if (!m_sp_card_file.Initialise_Acquisition_FileSet(dirs[0].path().toStdString().c_str(), 0)) return;
// ABHIRUP: Commenting so that plot is updating when reading file
//    while( fileReader.hasMoreSignals() ){
//        if( !m_paused ) {
//            auto  s = fileReader.getNextSignal();
//            x =s.first;
//            y = s.second;
//            for( int i=0; i<3; i++ )
//            {
//               emit this->plotting(i);

//               // emit plotting(i);

//               QApplication::processEvents(); // Process UI events to maintain responsiveness
//            }


//            QThread::msleep(m_SleepDelay);
//        }else {
//            QApplication::processEvents();
//        }
//    }
// ABHIRUP: END

    // ui->customplot->addGraph(); // Add a new graph for each channel
//    emit on_plot(directoryA,0);
//    emit on_plot(directoryB,1);
     //emit on_plot(directoryC,0);
}

//void MainWindow::on_multi_plotting(QDir directory,int i)
//{
//   // QDir directory("C:\\Users\\VECC_Trainee\\Documents\\FirstWidget\\Californium_Data");
////    ui->customplot->clearGraphs(); // Clear existing graphs

//    // Define a color generator function
//    auto generateColor = [](int index) {
//        // Generate a color based on the index
//        int hue = index * 40 % 360; // Vary the hue based on the index
//        return QColor::fromHsv(hue, 255, 255); // Convert HSV to QColor
//    };


//        ui->customplot->addGraph(); // Add a new graph for each channel
//        // Customize each graph as needed
//        ui->customplot->graph(i)->setScatterStyle(QCPScatterStyle::ssCircle);
//        ui->customplot->graph(i)->setLineStyle(QCPGraph::lsLine);
//        ui->customplot->graph(i)->setPen(QPen(generateColor(i))); // Set color using the generator function
//        ui->customplot->graph(i)->setName(QString("Channel %1").arg(i + 1));

//    QStringList files = directory.entryList(QStringList() << "*.txt", QDir::Files);
//    foreach (const QString &file, files) {
//        QString fullPath = directory.absoluteFilePath(file);
////        fullPath.replace('/', QDir::separator());
//        qDebug() << fullPath;
//        std::ifstream inputFile(fullPath.toStdString());

//        if (!inputFile.is_open()) {
//            qDebug() << "Error opening the file!" << endl;
//            return;
//        }

//        std::string line;
//        const int batchSize = 1000; // Process data in batches of 1000
//        int processedCount = 0;
//        while (getline(inputFile, line)) {
//            std::istringstream ss(line);
//            int t;
//            float v;
//            ss >> t;
//            ss >> v;
//            x[i][t] = t;
//            y[i][t] = v;
//            if (++processedCount % batchSize == 0) {
//                QThread::msleep(m_SleepDelay);
//                 if(!m_paused)
//               {    emit plotting(i);
//                    QApplication::processEvents(); // Process UI events to maintain responsiveness
//               }
//            }

//            // Process any remaining data
//            if (processedCount % batchSize != 0) {
//                QApplication::processEvents(); // Process UI events
//            }
//        }

//        qDebug() << "Data processing completed. Processed Count: " << processedCount;

//        inputFile.close(); // Close the file

//        if(i==1||i==2)
//        draw_min_value_line(i);
//    }
//}


void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    m_SleepDelay = value;
}


void MainWindow::on_pausebutton_clicked()
{
    m_paused=!m_paused;
}


void MainWindow::on_pushButton_clicked()
{
    emit this->closeEvent(0);
    QApplication::quit();
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    event->accept();
    emit this->closeEvent(0);
    QApplication::quit();
}

void MainWindow::on_MainWindow_destroyed()
{
//    emit this->closeEvent(0);
//    QApplication::quit();
}

//void MainWindow::drawChannels(int numChannels)
//{

//    ui->customplot->clearGraphs(); // Clear existing graphs

//    // Define a color generator function
//    auto generateColor = [](int index) {
//        // Generate a color based on the index
//        int hue = index * 40 % 360; // Vary the hue based on the index
//        return QColor::fromHsv(hue, 255, 255); // Convert HSV to QColor
//    };


//        ui->customplot->addGraph(); // Add a new graph for each channel
//        // Customize each graph as needed
//        ui->customplot->graph(0)->setScatterStyle(QCPScatterStyle::ssCircle);
//        ui->customplot->graph(0)->setLineStyle(QCPGraph::lsLine);
//        ui->customplot->graph(0)->setPen(QPen(generateColor(0))); // Set color using the generator function
//        ui->customplot->graph(0)->setName(QString("Channel %1").arg(0 + 1));

//        // Generate data for each channel (you can replace this with your own data)
//        QDir directoryA("C:\\Users\\VECC_Trainee\\Documents\\FirstWidget\\Californium_Data\\Run1\\Chanel A");

//        QStringList filesA = directoryA.entryList(QStringList() << "*.txt", QDir::Files);
//        foreach (const QString &file, filesA) {
//            QString fullPath = directoryA.absoluteFilePath(file);
//    //        fullPath.replace('/', QDir::separator());
//            qDebug() << fullPath;
//            std::ifstream inputFile(fullPath.toStdString());

//            if (!inputFile.is_open()) {
//                qDebug() << "Error opening the file!" << endl;
//                return;
//            }

//            std::string line;
//            const int batchSize = 1000; // Process data in batches of 1000
//            int processedCount = 0;
//            while (getline(inputFile, line)) {
//                std::istringstream ss(line);
//                int t;
//                float v;
//                ss >> t;
//                ss >> v;
//                x[t] = t;
//                y[t] = v;
//                if (++processedCount % batchSize == 0) {
//                    QThread::msleep(m_SleepDelay);
//                     if(!m_paused)
//                   {    emit plotting();
//                        QApplication::processEvents(); // Process UI events to maintain responsiveness
//                   }
//                }

//                // Process any remaining data
//                if (processedCount % batchSize != 0) {
//                    QApplication::processEvents(); // Process UI events
//                }
//            }

//            qDebug() << "Data processing completed. Processed Count: " << processedCount;

//            inputFile.close(); // Close the file
////            draw_min_value_line();
//        }


//        ui->customplot->addGraph(); // Add a new graph for each channel
//        // Customize each graph as needed
//        ui->customplot->graph(1)->setScatterStyle(QCPScatterStyle::ssCircle);
//        ui->customplot->graph(1)->setLineStyle(QCPGraph::lsLine);
//        ui->customplot->graph(1)->setPen(QPen(generateColor(0))); // Set color using the generator function
//        ui->customplot->graph(1)->setName(QString("Channel %1").arg(0 + 1));

//        // Generate data for each channel (you can replace this with your own data)
//        QDir directoryB("C:\\Users\\VECC_Trainee\\Documents\\FirstWidget\\Californium_Data\\Run1\\Chanel B");

//        QStringList filesB = directoryB.entryList(QStringList() << "*.txt", QDir::Files);
//        foreach (const QString &file, filesB) {
//            QString fullPath = directoryB.absoluteFilePath(file);
//    //        fullPath.replace('/', QDir::separator());
//            qDebug() << fullPath;
//            std::ifstream inputFile(fullPath.toStdString());

//            if (!inputFile.is_open()) {
//                qDebug() << "Error opening the file!" << endl;
//                return;
//            }

//            std::string line;
//            const int batchSize = 1000; // Process data in batches of 1000
//            int processedCount = 0;
//            while (getline(inputFile, line)) {
//                std::istringstream ss(line);
//                int t;
//                float v;
//                ss >> t;
//                ss >> v;
//                x[t] = t;
//                y[t] = v;
//                if (++processedCount % batchSize == 0) {
//                    QThread::msleep(m_SleepDelay);
//                     if(!m_paused)
//                   {    emit plotting();
//                        QApplication::processEvents(); // Process UI events to maintain responsiveness
//                   }
//                }

//                // Process any remaining data
//                if (processedCount % batchSize != 0) {
//                    QApplication::processEvents(); // Process UI events
//                }
//            }

//            qDebug() << "Data processing completed. Processed Count: " << processedCount;

//            inputFile.close(); // Close the file
//            draw_min_value_line();
//        }


//        ui->customplot->addGraph(); // Add a new graph for each channel
//        // Customize each graph as needed
//        ui->customplot->graph(2)->setScatterStyle(QCPScatterStyle::ssCircle);
//        ui->customplot->graph(2)->setLineStyle(QCPGraph::lsLine);
//        ui->customplot->graph(2)->setPen(QPen(generateColor(0))); // Set color using the generator function
//        ui->customplot->graph(2)->setName(QString("Channel %1").arg(0 + 1));

//        // Generate data for each channel (you can replace this with your own data)
//        QDir directoryC("C:\\Users\\VECC_Trainee\\Documents\\FirstWidget\\Californium_Data\\Run1\\Chanel C");

//        QStringList filesC = directoryC.entryList(QStringList() << "*.txt", QDir::Files);
//        foreach (const QString &file, filesC) {
//            QString fullPath = directoryC.absoluteFilePath(file);
//    //        fullPath.replace('/', QDir::separator());
//            qDebug() << fullPath;
//            std::ifstream inputFile(fullPath.toStdString());

//            if (!inputFile.is_open()) {
//                qDebug() << "Error opening the file!" << endl;
//                return;
//            }

//            std::string line;
//            const int batchSize = 1000; // Process data in batches of 1000
//            int processedCount = 0;
//            while (getline(inputFile, line)) {
//                std::istringstream ss(line);
//                int t;
//                float v;
//                ss >> t;
//                ss >> v;
//                x[t] = t;
//                y[t] = v;
//                if (++processedCount % batchSize == 0) {
//                    QThread::msleep(m_SleepDelay);
//                     if(!m_paused)
//                   {    emit plotting();
//                        QApplication::processEvents(); // Process UI events to maintain responsiveness
//                   }
//                }

//                // Process any remaining data
//                if (processedCount % batchSize != 0) {
//                    QApplication::processEvents(); // Process UI events
//                }
//            }

//            qDebug() << "Data processing completed. Processed Count: " << processedCount;

//            inputFile.close(); // Close the file
//            draw_min_value_line();
//        }


//}

//void MainWindow::someFunction()
//{
//    int numChannels = 3; // Set the number of channels to 3
//    drawChannels(numChannels); // Call the drawChannels function
//}




void MainWindow::on_m_chanelA_checkBox_clicked(bool checked)
{
    m_checkBox[0] = checked;
}


void MainWindow::on_m_chanelA_checkBox_2_clicked(bool checked)
{
    m_checkBox[1] = checked;
}


void MainWindow::on_m_chanelA_checkBox_3_clicked(bool checked)
{
    m_checkBox[2] = checked;
}

void MainWindow::setx(std::vector<QVector<double>> x) { this->x = x; }
void MainWindow::sety(std::vector<QVector<double>> y) { this->y = y; }

bool MainWindow::isPaused(){ return this->m_paused; }
int MainWindow::getSleepDelay() { return this->m_SleepDelay; }

void MainWindow::on_event_Received() { ui->m_eventCountSpinBox->setValue( ui->m_eventCountSpinBox->value()+1 ); }





void MainWindow::on_pushButton_2_clicked()
{
    QString directoryPath1 = QFileDialog::getExistingDirectory(this, tr("Choose Directory with .bin file"),
                                                             "/home",
                                                             QFileDialog::ShowDirsOnly
                                                             | QFileDialog::DontResolveSymlinks);
    if (!directoryPath1.isEmpty()) {
        ui->Input_Directory->setText(directoryPath1);
    }
}





void MainWindow::on_pushButton_3_clicked()
{
    QString directoryPath2 = QFileDialog::getExistingDirectory(this, tr("Choose Directory with .bin file"),
                                                             "/home",
                                                             QFileDialog::ShowDirsOnly
                                                             | QFileDialog::DontResolveSymlinks);
    if (!directoryPath2.isEmpty()) {
        ui->Output_Directory->setText(directoryPath2);
    }
}



void MainWindow::on_m_chanelA_checkBox_4_clicked(bool checked)
{
    m_checkBox[3] = checked;
}

