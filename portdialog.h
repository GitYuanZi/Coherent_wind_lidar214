#ifndef PORTDIALOG_H
#define PORTDIALOG_H

#include <QDialog>
#include "serialportthread.h"
#include <QTimer>

const int MOTOR_RESOLUTION = 524288;			//电机分辨率

namespace Ui {
class portDialog;
}

class portDialog : public QDialog
{
	Q_OBJECT

public:
	explicit portDialog(QWidget *parent = 0);
	~portDialog();

    void initial_data(quint16 a,bool c,quint16 d,bool e);//默认的设置参数
	void search_set_port(int Sp);
	bool get_returnMotor_connect();			//返回连接电机的bool值给主程序
    void show_PX(float px_show);			//显示当前位置
	void update_status();					//更新按钮状态
    void close_order();                     //关闭电机和串口

	void ABS_Rotate(int Pa);				//绝对转动
	void CW_Rotate(int Pr);					//相对正转
	void CCW_Rotate(int Pr);				//相对反转
    void SetPX_1(int Px);					//设置当前位置PX，发送MO=0;
    void SetPX_2();                         //设置当前位置px，发送PX=n;
	void SetSP(int Sp);						//设置电机速度SP
    void SetAC();                           //设置电机加速度
    void SetDC();                           //设置电机减速度
	void OpenMotor();						//打开电机
    void check_MOvalue();                   //发送MO;查询电机是否成功商店
    void Send_BG();                         //发送BG;电机转动到预定位置
    void Send_MS();                         //发送MS;查询电机当前状态
    void Find_PX();                         //发送PX;查询电机当前位置

signals:
    void portdlg_send(const QString &re);           //对话框返回字符串到主程序
    void SendPX(float a);                           //PX值发送给主程序
    void Position_success();                        //电机到达预定位置
    void Position_Error();                          //电机未达到预定位置
    void Motor_connect_status(int m_status_value);	//电机连接状态
    void Motor_Port_close();                        //电机和串口关闭


private slots:
    void on_pushButton_open_motor_clicked();
	void on_pushButton_default_clicked();	//默认键
	void on_pushButton_sure_clicked();		//确定键
	void on_pushButton_cancel_clicked();	//取消键
	void on_pushButton_auto_searchPort_clicked();
	void on_pushButton_relative_clicked();	//相对转动键
	void on_pushButton_absolute_clicked();	//绝对转动键
	void on_pushButton_setPXis0_clicked();	//设置当前位置为0键
	void SPchanged();						//SP值改变

	void receive_response(const QString &s);
    void portError_OR_timeout(bool a);		//串口未连接或接收命令超时
    void port_Close();                      //串口关闭
    void DemandPX();						//查询PX

private:
	Ui::portDialog *ui;
	QString portname;			//串口名
	SerialPortThread thread_port;
	int retSP;					//电机的SP值
	bool Set_MotorConnect;		//是否需要连接电机
	quint16 StepAngle;			//采集的组数
	bool Not_collect;			//是否正在采集
	int PX_data;				//PX的int类型
    int status_value;           //电机实际连接状态
    bool order_value;           //电机转动类型
    bool initial_PX;            //系统初始时的PX


	QString Order_str;			//待发送给电机的字符串命令
	QTimer *timer1;				//定时器，用于定时检查当前位置
	bool portDia_status;		//串口对话框打开状态
	bool handle_PX;				//正在处理PX值

    bool rotate_judge;          //两次位置判断
    float  odd_num;             //2n+1次的PX值
    float  even_num;            //2n次的PX值
    int px_mainwindow;          //主程序采集完成后的位置
    bool click_OC_motor;        //点击了打开或关闭电机按钮
    bool over_set_PX;           //采集结束后调整当前位置px
};

#endif // PORTDIALOG_H
