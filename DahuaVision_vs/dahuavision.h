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

    IMV_DeviceList m_deviceInfoList;	            // 发现的相机列表
    QTimer m_staticTimer;	                        // 定时器，定时刷新状态栏信息


private slots:
    void btn_findDevice_clicked();                  // 查找设备
    void btn_openDevice_clicked();                  // 打开设备
    void btn_closeDevice_clicked();                 // 关闭设备
    void btn_startCapturing_clicked();              // 开始采集
    void btn_pauseCapturing_clicked();              // 停止采集
    void btn_saveImage_clicked();                   // 保存图像为JPG或BMP格式文件
    void cb_device_activated(int);                  // 变更当前设备
    void sb_expTime_valueChanged(int);              // 变更曝光时间
    void sb_gain_valueChanged(int);                 // 变更增益
    void closeEvent(QCloseEvent* event) override;   // 重写关闭窗口事件
    void btn_softTrigger_clicked();                 // 软触发一次
    void onTimerStreamStatistic();                  // 定时刷新状态栏信息

private:
    void initUi();                                  // 初始化
};
