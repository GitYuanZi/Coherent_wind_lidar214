#include "portdialog.h"
#include "ui_portdialog.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QDebug>
#include <QMessageBox>
#include <QString>

portDialog::portDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::portDialog)
{
	ui->setupUi(this);

	timer1= new QTimer(this);
	connect(timer1,SIGNAL(timeout()),this,SLOT(DemandPX()));

	connect(&thread_port,SIGNAL(response(QString)),this,SLOT(receive_response(QString)));
    connect(&thread_port,SIGNAL(S_PortNotOpen(bool)),this,SLOT(portError_OR_timeout(bool)));
    connect(&thread_port,SIGNAL(timeout(bool)),this,SLOT(portError_OR_timeout(bool)));
    connect(&thread_port,SIGNAL(last_return()),this,SLOT(port_Close()));
	handle_PX = false;
}

portDialog::~portDialog()
{
	delete ui;
}

bool portDialog::get_returnMotor_connect()					//返回连接电机bool值给主程序
{
	Set_MotorConnect = ui->checkBox_motor_connected->isChecked();
	return Set_MotorConnect;
}

void portDialog::search_set_port(int Sp)
{
	portDia_status = false;
	retSP = Sp;												//对话框接收SP值
	ui->pushButton_auto_searchPort->setEnabled(false);

	//搜索串口函数
	portname.clear();
	QSerialPort my_serial;
	foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
	{
		my_serial.close();
		my_serial.setPortName(info.portName());
		if(!my_serial.open(QIODevice::ReadWrite))
			return;
		my_serial.setBaudRate(QSerialPort::Baud19200);
		my_serial.setDataBits(QSerialPort::Data8);
		my_serial.setStopBits(QSerialPort::OneStop);
		my_serial.setFlowControl(QSerialPort::NoFlowControl);

		QString test("VR;");
		QByteArray testData = test.toLocal8Bit();
		my_serial.write(testData);
        if(my_serial.waitForBytesWritten(300))
		{
            if(my_serial.waitForReadyRead(300))
			{
				QByteArray testResponse = my_serial.readAll();
                while(my_serial.waitForReadyRead(300))
					testResponse += my_serial.readAll();
				QString response(testResponse);
				if(response.left(10) == "VR;Whistle")
				{
					portname = info.portName();
					ui->lineEdit_serialportName->setText(portname);
					break;
				}
			}
		}
	}
	my_serial.close();

	if(portname.left(3) == "COM")
	{
        qDebug() << "port found";
        ui->pushButton_open_motor->setEnabled(true);
		OpenMotor();										//打开电机
	}
	else
	{
        qDebug() << "no port found";
        status_value = 1;                                   //串口未连接
        emit this->Motor_connect_status(status_value);      //主界面状态栏显示电机连接状态
		ui->pushButton_auto_searchPort->setEnabled(true);	//未检测到串口，按钮设为使能状态
        ui->pushButton_open_motor->setEnabled(false);
	}
}

void portDialog::close_order()
{
    qDebug() << "Send closePort";
    Order_str = "closePort";
    thread_port.transaction(portname,Order_str);
}

void portDialog::initial_data( bool c, quint16 d,bool e)	//初始数据
{
	Set_MotorConnect = c;									//连接电机的bool值
	StepAngle = d;											//步进角
	Not_collect = e;										//正在采集的bool值
	portDia_status = false;

	ui->lineEdit_serialportName->setText(portname);			//串口名
	ui->lineEdit_SP->setText(QString::number(retSP));		//速度
	ui->lineEdit_AC->setText("180");						//加速度
	ui->lineEdit_DC->setText("180");						//减速度
	ui->radioButton_CW->setChecked(true);					//顺时针
	ui->lineEdit_PR->setText("0");							//移动距离
	ui->lineEdit_PA->setText("0");							//绝对距离
    ui->lineEdit_PX->setText(QString::number(PX_data));     //当前位置
	ui->checkBox_motor_connected->setChecked(Set_MotorConnect);	//连接电机

    ui->pushButton_setPXis0->setEnabled(false);
    if(portname.left(3) != "COM")
    {
        ui->pushButton_auto_searchPort->setEnabled(true);
        ui->pushButton_open_motor->setEnabled(false);
    }
    else
    {
        ui->pushButton_auto_searchPort->setEnabled(false);
        ui->pushButton_open_motor->setEnabled(true);
    }

    if(status_value == 3)
        ui->pushButton_open_motor->setText(QString::fromLocal8Bit("关闭电机"));
    else
        ui->pushButton_open_motor->setText(QString::fromLocal8Bit("打开电机"));

	if(Not_collect  == false)								//若正在采集，界面除取消键，其他均为非使能状态
	{
		ui->groupBox->setEnabled(false);					//通信设置box
		ui->groupBox_2->setEnabled(false);					//转动参数设定box
		ui->groupBox_3->setEnabled(false);					//直接控制box
		ui->groupBox_4->setEnabled(false);					//当前位置设定box
		ui->groupBox_motor->setEnabled(false);				//单方向采集box
		ui->pushButton_default->setEnabled(false);			//默认键
		ui->pushButton_sure->setEnabled(false);				//确定键
	}
	if(StepAngle == 0)										//单方向采集，box为使能状态
		ui->groupBox_motor->setEnabled(true);
	else
		ui->groupBox_motor->setEnabled(false);
	connect(ui->lineEdit_SP,&QLineEdit::textChanged,this,&portDialog::SPchanged);
}

void portDialog::SPchanged()
{
	if(ui->lineEdit_SP->text().toInt() > 90)
		ui->lineEdit_SP->setText("90");
	else
		retSP = ui->lineEdit_SP->text().toInt();
}

void portDialog::on_pushButton_open_motor_clicked()
{
    portDia_status = true;
    handle_PX = false;
    OpenMotor();
}

void portDialog::on_pushButton_default_clicked()			//默认键
{
	portDia_status = false;
	ui->lineEdit_SP->setText(QString::number(retSP));		//速度
	ui->lineEdit_AC->setText("180");						//加速度
	ui->lineEdit_DC->setText("180");						//减速度
    ui->radioButton_CW->setChecked(true);					//顺时针
	ui->lineEdit_PR->setText("0");							//移动距离
	ui->lineEdit_PA->setText("0");							//绝对距离
    ui->checkBox_motor_connected->setChecked(Set_MotorConnect);

	if(StepAngle == 0)										//单方向采集，box为使能状态
		ui->groupBox_motor->setEnabled(true);
	else
		ui->groupBox_motor->setEnabled(false);
}

void portDialog::on_pushButton_sure_clicked()				//确定键
{
	accept();
}

void portDialog::on_pushButton_cancel_clicked()				//取消键
{
	reject();
}

void portDialog::on_pushButton_auto_searchPort_clicked()	//自动检测键
{
	search_set_port(retSP);
}

void portDialog::on_pushButton_relative_clicked()			//相对转动键
{
    ui->pushButton_auto_searchPort->setEnabled(false);
    ui->pushButton_open_motor->setEnabled(false);
	ui->pushButton_relative->setEnabled(false);
	ui->pushButton_absolute->setEnabled(false);
	ui->pushButton_setPXis0->setEnabled(false);
	ui->pushButton_default->setEnabled(false);
	ui->pushButton_sure->setEnabled(false);
	ui->pushButton_cancel->setEnabled(false);
	portDia_status = true;
	if(ui->radioButton_CW->isChecked())
		CW_Rotate(ui->lineEdit_PR->text().toInt());
	else
		CCW_Rotate(ui->lineEdit_PR->text().toInt());
}

void portDialog::on_pushButton_absolute_clicked()			//绝对转动键
{
    ui->pushButton_auto_searchPort->setEnabled(false);
    ui->pushButton_open_motor->setEnabled(false);
	ui->pushButton_relative->setEnabled(false);
	ui->pushButton_absolute->setEnabled(false);
	ui->pushButton_setPXis0->setEnabled(false);
	ui->pushButton_default->setEnabled(false);
	ui->pushButton_sure->setEnabled(false);
	ui->pushButton_cancel->setEnabled(false);
	portDia_status = true;
	ABS_Rotate(ui->lineEdit_PA->text().toInt());
}

void portDialog::on_pushButton_setPXis0_clicked()			//设置当前位置为0键
{
	ui->pushButton_auto_searchPort->setEnabled(false);
	ui->pushButton_relative->setEnabled(false);
	ui->pushButton_absolute->setEnabled(false);
	ui->pushButton_setPXis0->setEnabled(false);
	ui->pushButton_default->setEnabled(false);
	ui->pushButton_sure->setEnabled(false);
	ui->pushButton_cancel->setEnabled(false);
	portDia_status = true;
	SetPX(ui->lineEdit_PX->text().toInt());
}

void portDialog::show_PX(float px_show)
{
    ui->lineEdit_PX->setText(QString::number(px_show,'f',2));
}

void portDialog::update_status()
{
    ui->pushButton_auto_searchPort->setEnabled(false);
    ui->pushButton_open_motor->setEnabled(true);
	ui->pushButton_relative->setEnabled(true);
	ui->pushButton_absolute->setEnabled(true);
    ui->pushButton_setPXis0->setEnabled(false);
	ui->pushButton_default->setEnabled(true);
	ui->pushButton_sure->setEnabled(true);
	ui->pushButton_cancel->setEnabled(true);
	portDia_status = false;
}

void portDialog::ABS_Rotate(int Pa)
{
    qDebug() << "ABS  PX_data = " << PX_data;
    qDebug() << "ABS  pa      = " << Pa;
    PX_data = Pa;
    Order_str = "PA="+QString::number(Pa*MOTOR_RESOLUTION/360)+";";
    order_value = true;
	thread_port.transaction(portname,Order_str);
}

void portDialog::CW_Rotate(int Pr)
{
    qDebug() << "CW PX_data = " << PX_data;
    qDebug() << "CW pa      = " << Pr;
    PX_data = PX_data + Pr;
    Order_str = "PR="+QString::number(Pr*MOTOR_RESOLUTION/360)+";";
    order_value = false;
	thread_port.transaction(portname,Order_str);
}

void portDialog::CCW_Rotate(int Pr)
{
    PX_data = PX_data - Pr;
    Order_str = "PR=-"+QString::number(Pr*MOTOR_RESOLUTION/360)+";";
    order_value = false;
	thread_port.transaction(portname,Order_str);
}

void portDialog::SetPX(int Px)
{
	Order_str = "MO=0;PX="+QString::number(Px*MOTOR_RESOLUTION/360)+";MO=1;";
	thread_port.transaction(portname,Order_str);
}

void portDialog::SetSP(int Sp)
{
    qDebug() << "send SP=" << Sp;
    Order_str = "SP="+QString::number(Sp*MOTOR_RESOLUTION/360)+";";
	thread_port.transaction(portname,Order_str);
}

void portDialog::SetAC()
{
    qDebug() << "send AC=10^6";
    Order_str = "AC=1000000;";
    thread_port.transaction(portname,Order_str);
}

void portDialog::SetDC()
{
    qDebug() << "send DC=10^6";
    Order_str = "DC=1000000;";
    thread_port.transaction(portname,Order_str);
}

void portDialog::OpenMotor()
{
    if(status_value == 3)
    {
        qDebug() << "send MO=0";
        Order_str = "MO=0;";
    }
    else
    {
        qDebug() << "send MO=1";
        Order_str = "MO=1;";
    }
	thread_port.transaction(portname,Order_str);
}

void portDialog::check_MOvalue()
{
    qDebug() << "send MO;";
    Order_str = "MO;";
    thread_port.transaction(portname,Order_str);
}

void portDialog::Send_BG()
{
    qDebug() << "send BG;";
    Order_str = "BG;";
    thread_port.transaction(portname,Order_str);
}

void portDialog::Send_MS()
{
    qDebug() << "send MS;";
    Order_str = "MS;";
    thread_port.transaction(portname,Order_str);
}

void portDialog::Find_PX()
{
    qDebug() << "Find PX;";
    initial_PX = true;
    Order_str = "PX;";
    thread_port.transaction(portname,Order_str);
}

void portDialog::DemandPX()
{
	if(handle_PX == false)
	{
		handle_PX = true;
        Order_str = "PX;";
		thread_port.transaction(portname,Order_str);
	}
}

void portDialog::receive_response(const QString &s)
{
	QString res = s;

    qDebug() << "res = " << res;
    if(res.left(3) == "MO=")
    {
        check_MOvalue();                                    //检查电机打开状态
//        if(res.left(4) == "MO=1")                         //MO=1,打开电机
//            check_MOvalue();
        if((res.left(4) == "MO=0")&&portDia_status)         //MO=0,设置当前位置后，更新对话框中的按钮
            update_status();
    }
    else
        if(res.left(3) == "MO;")
        {
            if(res.left(4) == "MO;0")                       //电机打开失败
            {
                status_value = 2;
                ui->pushButton_open_motor->setText(QString::fromLocal8Bit("打开电机"));
                ui->pushButton_absolute->setEnabled(false);
                ui->pushButton_relative->setEnabled(false);
                ui->pushButton_auto_searchPort->setEnabled(false);
                emit this->Motor_connect_status(status_value);//主界面状态栏显示电机连接状态
            }
            else
                if(res.left(4) == "MO;1")                   //电机打开成功
                {
                    status_value = 3;
                    ui->pushButton_open_motor->setText(QString::fromLocal8Bit("关闭电机"));
                    ui->pushButton_absolute->setEnabled(true);
                    ui->pushButton_relative->setEnabled(true);
                    ui->pushButton_auto_searchPort->setEnabled(false);
                    emit this->Motor_connect_status(status_value);          //主界面状态栏显示电机连接状态
                    SetSP(retSP);                           //设置速度
                }
        }
        else
            if(res.left(3) == "SP=")
                SetAC();                                    //设置加速度
            else
                if(res.left(3) == "AC=")
                    SetDC();                                //设置减速度
                else
                    if(res.left(3) == "DC=")
                        Find_PX();
                    else
                        if((res.left(3) == "PA=")||(res.left(3) == "PR="))
                            Send_BG();                          //电机转动
                        else
                            if(res.left(3) == "BG;")
                                timer1->start(60);              //打开定时器，定时发送PX;
                            else
                                if(res.left(3) == "PX;")
                                {
                                    handle_PX = true;
                                    QStringList list = res.split(";");
                                    QString PxStr = list.at(1).toLocal8Bit().data();

                                    float PX_float = PxStr.toFloat()*360/MOTOR_RESOLUTION;
                                    qDebug() << "PX_float = " << PX_float;
                                    emit this->SendPX(PX_float);//主界面左侧圆盘显示电机位置

                                    if(portDia_status)
                                        show_PX(PX_float);      //串口对话框显示电机位置数值

                                    int PX_int = PX_float + 0.5;

                                    if(initial_PX)
                                    {
                                        PX_data = PX_int;
                                        initial_PX = false;
                                    }
                                    else
                                    {
                                        qDebug() << "PX_data  = " << PX_data;
                                        if(((PX_data-1)<=PX_int)&&(PX_int<=(PX_data+1)))
                                        {
                                            timer1->stop();      //若到达预定位置，关闭定时器，并发送MS
                                            Send_MS();
                                        }
                                    }

                                    handle_PX = false;

                                }
                                else
                                    if(res.left(3) == "MS;")
                                    {
                                        QStringList list = res.split(";");
                                        QString BGStr = list.at(1).toLocal8Bit().data();
                                        if(BGStr == "0")        //电机停止转动
                                        {
                                            if(portDia_status)
                                                update_status();//更新串口对话框界面
                                            emit this->Position_success();
                                        }
                                    }
}

void portDialog::portError_OR_timeout(bool a)				//串口未能打开或连接超时
{
	timer1->stop();											//关闭定时器
    if(a)
        status_value = 1;
    else
        status_value = 2;
    emit this->Motor_connect_status(status_value);          //主界面状态栏显示电机未能正确连接
	if(portDia_status)
		update_status();									//更新对话框按钮
}

void portDialog::port_Close()
{
    qDebug() << "port_close";
    portname.clear();
    emit this->Motor_Port_close();
}
