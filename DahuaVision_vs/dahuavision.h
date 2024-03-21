#pragma once

#include <QtWidgets/QWidget>
#include "ui_dahuavision.h"
#include "IMVApi.h"
#include <qtimer.h>
#include "camwidget.h"

class DahuaVision : public QWidget
{
    Q_OBJECT

public:
    DahuaVision(QWidget *parent = nullptr);
    ~DahuaVision();

private:
    Ui::DahuaVision ui;

    IMV_DeviceList m_deviceInfoList;	            // ���ֵ�����б�
    QTimer m_staticTimer;	                        // ��ʱ������ʱˢ��״̬����Ϣ


private slots:
    void btn_findDevice_clicked();                  // �����豸
    void btn_openDevice_clicked();                  // ���豸
    void btn_closeDevice_clicked();                 // �ر��豸
    void btn_startCapturing_clicked();              // ��ʼ�ɼ�
    void btn_pauseCapturing_clicked();              // ֹͣ�ɼ�
    void btn_saveImage_clicked();                   // ����ͼ��ΪJPG��BMP��ʽ�ļ�
    void cb_device_activated(int);                  // �����ǰ�豸
    void sb_expTime_valueChanged(int);              // ����ع�ʱ��
    void sb_gain_valueChanged(int);                 // �������
    void closeEvent(QCloseEvent* event) override;   // ��д�رմ����¼�
    void btn_softTrigger_clicked();                 // ����һ��
    void onTimerStreamStatistic();                  // ��ʱˢ��״̬����Ϣ

private:
    void initUi();                                  // ��ʼ��
};
