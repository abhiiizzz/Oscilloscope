#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QCoreApplication>
#include <fstream>
#include <QVector>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class DataProcessor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void drawChannels(int numChannels);
    void someFunction();
    void setx(std::vector<QVector<double>> x);
    void sety(std::vector<QVector<double>> y);
    void saveData(int channel);
    void setNumberOfChannels(int num_channel);
    bool isPaused();
    int getSleepDelay();
    bool isConnected;

protected:
    void closeEvent(QCloseEvent *event) override; // Override the close event handler

private slots:
    void on_plotbutton_clicked();
    void plot_update(int i);

    void on_horizontalSlider_valueChanged(int value);

    void on_pausebutton_clicked();

    void draw_min_value_line(int i);  // Add this line

    void on_pushButton_clicked();

    void on_MainWindow_destroyed();

    void on_m_chanelA_checkBox_clicked(bool checked);

    void on_m_chanelA_checkBox_2_clicked(bool checked);

    void on_m_chanelA_checkBox_3_clicked(bool checked);

    void on_m_chanelA_checkBox_4_clicked(bool checked);

    void on_event_Received();

    void initiateGui(int numChannels);


//    void on_multi_plotting(QDir dir,int i);




//    void on_m_connectButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();



signals:
    void plotting(int i);
    void on_plot(QDir directory, int i);
    void eventReceived();
    void numChannelRecieved(int numChannels);
//    void plotA();
//    void plotB();
//    void plotC();

private:

    Ui::MainWindow *ui;
    std::vector<QVector<double>> x,y;
    int m_SleepDelay;
    bool m_paused;
//    std::shared_ptr<QCPItemLine> m_pminValueLine;  // Add this line
    std::vector<bool> m_checkBox;
    QVector<QCPItemLine*> m_minValueLines;
    std::vector<FILE*> m_f;
    int m_num_channel;
};
#endif // MAINWINDOW_H
