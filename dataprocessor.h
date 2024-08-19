#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H
#include <bits/stdc++.h>
#include <QObject>
#include <fstream>
#include <QVector>

class DataProcessor : public QObject
{
    Q_OBJECT

public:
    explicit DataProcessor(std::ifstream& inputFile, QObject *parent = nullptr)
        : QObject(parent), inputFile(inputFile) {}

public slots:
    void processData()
    {
        std::string line;
        while (getline(inputFile, line)) {
            std::istringstream ss(line);
            int t;
            float v;
            ss >> t >> v;
            x[t] = t;
            y[t] = v;
            if (t == 999) {
                emit plotting();
            }
        }

        emit finished(); // Signal that processing is finished
    }

signals:
    void plotting();
    void finished();

private:
    std::ifstream& inputFile;
    QVector<double> x, y;
};

#endif // DATAPROCESSOR_H
