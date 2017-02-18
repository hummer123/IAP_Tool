#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QTimer>


QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QComboBox;
class QGroupBox;
class QTextEdit;
class QLineEdit;
class QIntValidator;
class QProgressBar;
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *, QEvent *);

private:
    void UI_Init(QWidget *);
    void fillPortsInfo();
    void fillPortsParameters();
    void updateSettings();
    void updatePortCbx();

    quint16 CRC16_Check(char *, qint16);
    void xModemTransmitData(qint16);


private slots:
    void openFileAPath();
    void saveFileTo();
    void checkCustomBaudRatePolicy(int idx);
    void openSerialPort();
    void clearData();
    void readyData();
    void receiveData();
    void sendData();

private:
    QGroupBox *binGBx;
    QGroupBox *commGBx;
    QTextEdit *commEdit;

    QLineEdit *filePathEdit;
    QLabel *otherInfoLbl;
    QLabel *binSizeLbl;
    QLabel *baudLbl;
    QLabel *stopLbl;
    QLabel *dataLbl;
    QLabel *parityLbl;
    QComboBox *portCbx;
    QComboBox *baudCbx;
    QComboBox *stopCbx;
    QComboBox *dataCbx;
    QComboBox *parityCbx;
    QPushButton *openPortBtn;
    QPushButton *downLoadBtn;
    QPushButton *openFileBtn;
    QPushButton *saveFileBtn;
    QPushButton *clearBtn;
    QProgressBar *progressBar;

private:
    quint32 binSize;
    QString binFilePath;
    QString binFileData;
    char XmodemFrame[1024];

    QTimer *useTimer;
    QTimer *sysTimer;
    QSerialPort *useSerialPort;
    quint16 binPackNum;

    QIntValidator *intValidator;
};

/*************   串口检测类   **************/
class PortCheck : public QObject
{
    Q_OBJECT

signals:
    void stopCurPort();

public slots:
    void sysPortCheck();

private:
    QStringList newPortList;
    bool judgeSta = false;
};



#endif // MAINWINDOW_H
