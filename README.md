# IAP_Tool
IAP Tool use MCU IAP download

程序是基于Qt开发的串口IAP下载上位机,主要实现如下功能：
> * 串口IAP下载(基于1K-xModem协议实现);
> * 实现bin文件的读取并可选择另存为txt格式文件
> * 使用Qt自带的串口类，并尝试在线程中实现端口号的实时自动扫描;

**主界面** <br>
![image](https://github.com/hummer123/IAP_Tool/raw/master/README-PIC/IAP.png)

# 串口的串口实时自动扫描
在这里我写了一个PortCheck类，类中实现了一个槽函数sysPortCheck用于检测当前PC机可用端口并刷新在界面上.
主程序创建一个定时器，定时1s去触发PortCheck中的sysPortCheck槽函数，同时把PortCheck类移到线程中;
'''C++
connect(sysTimer, &QTimer::timeout, portCheck, &PortCheck::sysPortCheck);
connect(portCheck, &PortCheck::stopCurPort, this, &MainWindow::openSerialPort);
portCheck->moveToThread(secondThread);
secondThread->start();
sysTimer->start(1000);  //1s检测一次可用串口数
'''

# 串口的接收
这里串口的数据接收采用定时接收，当串口缓冲区中有数据可读时，定时100ms在去读，保证完整地读取一帧数据

# 效果图
![image](https://github.com/hummer123/IAP_Tool/raw/master/README-PIC/Use.png)
> * (注：上面的图是我基于LPC1788的串口的IAP下载，
[具体的LPC1788 IAP移植及下载可以看我在NXP论坛上的一篇帖子](http://www.nxpic.org/module/forum/thread-608158-1-1.html))
