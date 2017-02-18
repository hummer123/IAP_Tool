#include "mainwindow.h"
#include <QtWidgets>
#include <QtSerialPort/QSerialPortInfo>
#include <QScrollBar>
#include <QtCore/QDebug>
#include <QThread>


#define XMODEM_SOH  0x01
#define XMODEM_STX  0x02
#define XMODEM_EOT  0x04
#define XMODEM_ACK  0x06
#define XMODEM_NAK  0x15
#define XMODEM_CAN  0x18
#define XMODEM_CRC  0x43    // 'C'

QStringList oldPortList;   //全局变量用于保存PC可用串口
QString usePortName;       //当前正在使用的串口号
bool isOpenSerial = false; //当前串口打开状态


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *mainWidget = new QWidget;
    setCentralWidget(mainWidget);

    binSize = 0;
    binFilePath.clear();
    binFileData.clear();
    useSerialPort = new QSerialPort(this);
    binPackNum = 0;
    intValidator = new QIntValidator(0, 4000000, this);
    useTimer = new QTimer(this);
    useTimer->setInterval(100); //间隔100ms 用于缓冲接收数据, readData()中开启
    useTimer->stop();

    isOpenSerial = false;
    usePortName.clear();
    oldPortList.clear();

    UI_Init(mainWidget);

    PortCheck *portCheck = new PortCheck;
    QThread *secondThread = new QThread;
    sysTimer = new QTimer(this);

    connect(openFileBtn, &QPushButton::clicked, this, &MainWindow::openFileAPath);
    connect(saveFileBtn, &QPushButton::clicked, this, &MainWindow::saveFileTo);

    connect(baudCbx, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::checkCustomBaudRatePolicy);
    connect(openPortBtn, &QPushButton::clicked, this, &MainWindow::openSerialPort);
    connect(downLoadBtn, &QPushButton::clicked, this, &MainWindow::sendData);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearData);
    connect(useSerialPort, SIGNAL(readyRead()), this, SLOT(readyData()));
    connect(useTimer, SIGNAL(timeout()), this, SLOT(receiveData()));
    //线程连接
    connect(sysTimer, &QTimer::timeout, portCheck, &PortCheck::sysPortCheck);
    connect(portCheck, &PortCheck::stopCurPort, this, &MainWindow::openSerialPort);
    portCheck->moveToThread(secondThread);
    secondThread->start();
    sysTimer->start(1000);  //1s检测一次可用串口数


    fillPortsParameters();
    fillPortsInfo();

    filePathEdit->installEventFilter(this);
    portCbx->installEventFilter(this);

//    qDebug() << "main thread: " <<QThread::currentThreadId();

    setWindowTitle(tr("IAP DownLoad bin"));
    this->setFont(QFont("Consolas", 10));
    this->setWindowIcon(QIcon(":/images/window"));
    this->setFixedSize(sizeHint().width(), sizeHint().height());
}

MainWindow::~MainWindow()
{
    if(useSerialPort->isOpen())
        useSerialPort->close();
    useTimer->stop();
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == filePathEdit)
    {
        if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyAnyEvent = static_cast<QKeyEvent *>(event);
            if((keyAnyEvent->key() == Qt::Key_Left) || (keyAnyEvent->key() == Qt::Key_Right)
                    || ((keyAnyEvent->key() == Qt::Key_C)&&(keyAnyEvent->modifiers()&Qt::ControlModifier)))
            {
                //qDebug() << "Key_Left";
                return QObject::eventFilter(obj,event);
            }
            else
                return true;
        }
    }
    if(obj == portCbx)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            updatePortCbx();

            return QObject::eventFilter(obj,event);
        }
    }

    return QObject::eventFilter(obj,event);
}

void MainWindow::openSerialPort()
{
    if(isOpenSerial == false)
    {
        updateSettings();

        if(useSerialPort->open(QIODevice::ReadWrite))
        {
            openPortBtn->setText(tr("Close Serial Port"));
            openPortBtn->setIcon(QIcon(":/images/open"));
            commEdit->append(tr("Open Serial successful"));
            isOpenSerial = true;
//            qDebug() << "Open Serial";
        }
        else
        {
            QMessageBox::information(this, tr("Open Serial failed"), tr("Con't Open ") + portCbx->currentText(), QMessageBox::Ok);
            return;
        }
    }
    else
    {
        if(useSerialPort->isOpen())
            useSerialPort->close();
        openPortBtn->setText(tr("Open Serial Port"));
        openPortBtn->setIcon(QIcon(":/images/close"));
        commEdit->append(tr("Close Serial successful"));
        isOpenSerial = false;
//        qDebug() << "Close Serial";
    }
}

void MainWindow::clearData()
{
    commEdit->clear();
    progressBar->setValue(0);
}

void MainWindow::readyData()
{
    if(useSerialPort->bytesAvailable() > 0)
    {
        if(!useTimer->isActive())
        {
             useTimer->start();     //等待100ms 用于缓冲接收数据
        }
    }
}

void MainWindow::receiveData()
{
    useTimer->stop();

    if(isOpenSerial == true)
    {
        QByteArray revBuf = useSerialPort->readAll();

//        qDebug() << "Receive Data = " << revBuf;

        //设置竖直滚条在最下方，显示commEdit最新数据
        QTextCursor cursor = commEdit->textCursor();
        cursor.movePosition(QTextCursor::End);
        commEdit->setTextCursor(cursor);

        //commEdit->insertPlainText(sData);

        switch(revBuf[0])
        {
        case XMODEM_CRC: binPackNum = 0; commEdit->append(tr("Receive Signal 'C', validate 1k-Xmodem"));
                            xModemTransmitData(binPackNum); break;
        case XMODEM_ACK: ++binPackNum; commEdit->append(tr("MCU Correctly received data"));
                            xModemTransmitData(binPackNum); break;
        case XMODEM_NAK: commEdit->append(tr("MCU Received data is Error, Resend dataPack"));
                            xModemTransmitData(binPackNum); break;
        case XMODEM_CAN: commEdit->append(tr("MCU Abort transmission data")); break;
        default: break;
        }
    }
//    qDebug() << "Time Receive Data";
}


void MainWindow::sendData()
{
    commEdit->append(tr("The Function is Reserve"));
}

void MainWindow::UI_Init(QWidget *curWidget)
{
    //bin 文件操作
    binGBx = new QGroupBox(tr("Bin Path"));
    filePathEdit = new QLineEdit;
    filePathEdit->setStyleSheet("QLineEdit{background-color:rgba(255,255,255,20%)}");
    binSizeLbl = new QLabel(tr("bin File Size: 0 <b>B</b>"));
    binSizeLbl->setFrameStyle(QFrame::Panel | QFrame::Raised);
    openFileBtn = new QPushButton(tr("Open File"));
    openFileBtn->setFocus();
    saveFileBtn = new QPushButton(tr("Save to"));
    downLoadBtn = new QPushButton(tr("DownLoad"));
    QGridLayout *binLayout = new QGridLayout;
    binLayout->setMargin(2);
    binLayout->addWidget(filePathEdit, 0, 0, 1, 2);
    binLayout->addWidget(openFileBtn, 0, 2);
    binLayout->addWidget(binSizeLbl, 1,0);
    binLayout->addWidget(saveFileBtn, 1, 1);
    binLayout->addWidget(downLoadBtn, 1, 2);
    binGBx->setLayout(binLayout);


    //通信显示框
    commGBx = new QGroupBox(tr("Communication Area"));
    commEdit = new QTextEdit;
    commEdit->setReadOnly(true);
    commEdit->setFont(QFont("Consolas", 11));
    commEdit->setFixedWidth(340);
    commEdit->setStyleSheet("QTextEdit{background-color:rgba(255,228,181,15%)}");
    QVBoxLayout *commLayout = new QVBoxLayout;
    commLayout->setMargin(1);
    commLayout->addWidget(commEdit);
    //串口信息
    portCbx = new QComboBox;
    baudCbx = new QComboBox;
    dataCbx = new QComboBox;
    stopCbx = new QComboBox;
    parityCbx = new QComboBox;
    baudLbl = new QLabel(tr("BaudRate: "));
    stopLbl = new QLabel(tr("Stop bits: "));
    dataLbl = new QLabel(tr("Data bits: "));
    parityLbl = new QLabel(tr("Parity: "));
    openPortBtn = new QPushButton(tr("Open Serial Port"));
    openPortBtn->setIcon(QIcon(":/images/close"));
    clearBtn = new QPushButton(tr("Clear Information"));
    clearBtn->setIcon(QIcon(":/images/clear"));
    QGridLayout *portLayout = new QGridLayout;
    portLayout->addWidget(portCbx,0, 0, 1, 2);
    portLayout->addWidget(baudLbl, 1, 0);
    portLayout->addWidget(baudCbx, 1, 1);
    portLayout->addWidget(dataLbl, 2, 0);
    portLayout->addWidget(dataCbx, 2, 1);
    portLayout->addWidget(stopLbl, 3, 0);
    portLayout->addWidget(stopCbx, 3, 1);
    portLayout->addWidget(parityLbl, 4, 0);
    portLayout->addWidget(parityCbx, 4, 1);
    portLayout->addWidget(openPortBtn, 5, 0, 1, 2);
    portLayout->addWidget(clearBtn, 6, 0, 1, 2);
    QHBoxLayout *midLayout = new QHBoxLayout;
    midLayout->setMargin(1);
    midLayout->addLayout(commLayout);
    midLayout->addLayout(portLayout);
    commGBx->setLayout(midLayout);


    //进度条
    progressBar = new QProgressBar;
    progressBar->setValue(0);
    progressBar->setFixedHeight(15);

    //总布局 = binGBx + commGBx + progressBar
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setMargin(1);
    mainLayout->addWidget(binGBx);
    mainLayout->addWidget(commGBx);
    mainLayout->addWidget(progressBar);

    curWidget->setLayout(mainLayout);
}

void MainWindow::fillPortsParameters()
{
    baudCbx->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    baudCbx->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    baudCbx->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    baudCbx->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    baudCbx->addItem(QStringLiteral("Custom"));
    baudCbx->setCurrentIndex(3);

    dataCbx->addItem(QStringLiteral("5"), QSerialPort::Data5);
    dataCbx->addItem(QStringLiteral("6"), QSerialPort::Data6);
    dataCbx->addItem(QStringLiteral("7"), QSerialPort::Data7);
    dataCbx->addItem(QStringLiteral("8"), QSerialPort::Data8);
    dataCbx->setCurrentIndex(3);

    parityCbx->addItem(QStringLiteral("None"), QSerialPort::NoParity);
    parityCbx->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
    parityCbx->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);
    parityCbx->addItem(QStringLiteral("Mark"), QSerialPort::MarkParity);
    parityCbx->addItem(QStringLiteral("Space"), QSerialPort::SpaceParity);

    stopCbx->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_OS_WIN
    stopCbx->addItem(QStringLiteral("1.5"), QSerialPort::OneAndHalfStop);
#endif
    stopCbx->addItem(QStringLiteral("2"), QSerialPort::TwoStop);
}

void MainWindow::fillPortsInfo()
{
    portCbx->clear();

    foreach(const QSerialPortInfo info, QSerialPortInfo::availablePorts())
    {
        oldPortList << info.portName();
    }
    qSort(oldPortList.begin(), oldPortList.end());

    portCbx->addItems(oldPortList);
}

void MainWindow::checkCustomBaudRatePolicy(int idx)
{
    bool isCustomBaudRate = !baudCbx->itemData(idx).isValid();
    baudCbx->setEditable(isCustomBaudRate);
    if(isCustomBaudRate)
    {
        baudCbx->clearEditText();
        QLineEdit *edit = baudCbx->lineEdit();
        edit->setValidator(intValidator);
    }
//    qDebug() << "check Custom BaudRate";
}

void MainWindow::updateSettings()
{
    useSerialPort->setPortName(portCbx->currentText());
    usePortName = portCbx->currentText();   //记录当前使用的串口号
    if(baudCbx->currentIndex() == 4)
        useSerialPort->setBaudRate(baudCbx->currentText().toInt());
    else
        useSerialPort->setBaudRate(static_cast<QSerialPort::BaudRate>(baudCbx->itemData(baudCbx->currentIndex()).toInt()));
    useSerialPort->setDataBits(static_cast<QSerialPort::DataBits>(dataCbx->itemData(dataCbx->currentIndex()).toInt()));
    useSerialPort->setStopBits(static_cast<QSerialPort::StopBits>(stopCbx->itemData(stopCbx->currentIndex()).toInt()));
    useSerialPort->setParity(static_cast<QSerialPort::Parity>(parityCbx->itemData(parityCbx->currentIndex()).toInt()));
//    qDebug() << "port currentText " << portCbx->currentText();
}

void MainWindow::openFileAPath()
{
    binFilePath = QFileDialog::getOpenFileName(this, tr("Open File"), ".", tr("bin File (*.bin)"));
    if(binFilePath.isEmpty())
        return;
    filePathEdit->setText(binFilePath);
    QFileInfo *temp = new QFileInfo(binFilePath);

    binSize = temp->size();

    if(binSize < 1024)
        binSizeLbl->setText(tr("bin Data Size: %1<b>B</b>").arg(binSize));
    else if(binSize < 1024*1024)
        binSizeLbl->setText(tr("bin Data Size: %1 <b>K</b> %2 <b>B</b>").arg(binSize/1024).arg(binSize%1024));
    else
        binSizeLbl->setText(tr("bin Data Size: %1 <b>M</b> %2<b>K</b> %3 <b>B</b>").arg(binSize/1024/1024)
                            .arg(binSize/1024%1024).arg(binSize%1024));
    delete temp;
    QFile importfile(binFilePath);
    if(!importfile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
          QMessageBox::warning(this, tr("Open File Failed"), tr("Cannot Open file"), QMessageBox::Ok);
          importfile.close();
    }

    QDataStream in(&importfile);

    char *binByte = new char[binSize];
    in.readRawData(binByte, binSize);

    binFileData.clear();
    QByteArray tempByte = QByteArray(binByte, binSize);
    binFileData = tempByte.toHex().toUpper();

    //显示文件内容
    quint32 column = 0;
    for(qint64 i = 2; i < binFileData.size(); i += 3)
    {
        if(column == 15)
        {
            binFileData.insert(i, '\n');
            column = 0;
        }
        else
        {
            binFileData.insert(i, ' ');
            ++column;
        }
    }

    delete[] binByte;

    importfile.close();
}

void MainWindow::saveFileTo()
{
    if(binFileData.isEmpty())
    {
        QMessageBox::information(this, tr("Save File Failed"), tr("Data is Empty"),QMessageBox::Ok);
        return;
    }
    QString saveFilePath = QFileDialog::getSaveFileName(this, tr("Save as..."), ".", tr("Save to Files(*.txt);;Save As Files(*.)"));
    if(saveFilePath.isEmpty())
        return;
    QFile savefile(saveFilePath);
    if(!savefile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::information(this, tr("Save File Failed"), tr("Open file %1 failed, connot save file\n%2")
                                 .arg(saveFilePath).arg(savefile.errorString()), QMessageBox::Ok);
        return;
    }

    QTextStream out(&savefile);
    out << binFileData;

    savefile.close();
}

void MainWindow::updatePortCbx()
{
    portCbx->clear();
    portCbx->addItems(oldPortList);
//    qDebug() << "PortCbx clicked";
}


/*********************   XModem   **********************/
quint16 MainWindow::CRC16_Check(char *puData, qint16 Len)
{
    qint8 i;
    quint16 u16CRC = 0;

    while(--Len)
    {
        i = 8;
        u16CRC = u16CRC^(((quint16)*puData++) << 8);

        do
        {
            if(u16CRC & 0x8000)
                u16CRC = (u16CRC<<1)^0x1021;    //CRC-ITU
            else
                u16CRC = u16CRC<<1;
        }while(--i);
    }

    return (u16CRC&0xFFFF);
}

void MainWindow::xModemTransmitData(qint16 packNum)
{
    qint16 lastSize;
    quint16 CRC;
    quint8 temp;

    lastSize = binSize - packNum*1024;

    if(lastSize > 0)
    {
        QFile *binFile = new QFile(binFilePath);
        binFile->open(QIODevice::ReadOnly);
        binFile->seek(packNum*1024);

        memset(XmodemFrame, 0x1A, 1024);
        temp = (quint8)(packNum&0xFF);

        QByteArray binTransmit;
        binTransmit.resize(5);

        binTransmit[0] = XMODEM_STX;
        binTransmit[1] = temp;
        binTransmit[2] = ~temp;

        if(lastSize >= 1024)
        {
            binFile->read(XmodemFrame, 1024);
            int i = (1 - (float(lastSize)/float(binSize))) * 100;
            progressBar->setValue(i);
        }
        else
        {
            binFile->read(XmodemFrame, lastSize);
            progressBar->setValue(100);
        }

        CRC = CRC16_Check(XmodemFrame, 1024);
        binTransmit[3] = (quint8)((CRC>>8)&0xFF);
        binTransmit[4] = (quint8)(CRC&0xFF);
        binTransmit.insert(3, XmodemFrame, 1024);

        binFile->close();
        useSerialPort->write(binTransmit);
        commEdit->append(tr("Download 1K dataPack, PackNum = %1").arg(packNum));
    }
    else
    {
        qDebug() << "Send EOT";
        char chEOT = XMODEM_EOT;
        useSerialPort->write(&chEOT);   //发送文件结束符 EOT
        commEdit->append(tr("Download OK..."));
    }

//    qDebug() << "Xmodem PackNum = " << packNum;
}


/*********************  串口检测类   *********************/
void PortCheck::sysPortCheck()
{
    newPortList.clear();
    judgeSta = false;

    foreach(const QSerialPortInfo info, QSerialPortInfo::availablePorts())
    {
        newPortList << info.portName();
        if(usePortName == info.portName())
            judgeSta =  true;
    }

    qSort(newPortList.begin(), newPortList.end());

    if((isOpenSerial == true) && (judgeSta == false))   //串口处于打开状态并且当前未检测到使用的串口，则关闭串口
    {
        emit stopCurPort();
    }
    if(newPortList.size() != oldPortList.size())        //发送更新串口表
    {
        oldPortList = newPortList;
    }
//    qDebug() << "PortCheck thread: " << QThread::currentThreadId();
}






