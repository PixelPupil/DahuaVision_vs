#include "dahuavision.h"
#include <qmessagebox.h>
#include <qfiledialog.h>

DahuaVision::DahuaVision(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

	// 定时变更统计值
    connect(&m_staticTimer, &QTimer::timeout, this, &DahuaVision::onTimerStreamStatistic);

    initUi();
}

DahuaVision::~DahuaVision()
{}

// 设定各按钮初始状态
void DahuaVision::initUi() {
	ui.gb_image->setEnabled(false);
	ui.gb_param->setEnabled(false);
	ui.gb_trigger->setEnabled(false);
	ui.lb_framecount->setText("");
	ui.cb_device->setEnabled(false);
	ui.btn_openDevice->setEnabled(false);
	ui.btn_closeDevice->setEnabled(false);
	ui.btn_startCapturing->setEnabled(false);
	ui.btn_pauseCapture->setEnabled(false);
	
	ui.pb_expTime->setValue(ui.sb_expTime->value());
	ui.pb_gain->setValue(ui.sb_gain->value());
	
}

// 变更当前设备
void DahuaVision::cb_device_activated(int nIndex) {
	ui.wid_cam->SetCamera(m_deviceInfoList.pDevInfo[nIndex].cameraKey);
}

// 查找设备
void DahuaVision::btn_findDevice_clicked() {
	int ret = IMV_OK;
	ret = IMV_EnumDevices(&m_deviceInfoList, interfaceTypeAll);

	if (IMV_OK != ret)
	{
		const QString warn_title = QString("Device not Found!");
		const QString warn_str = QString("Enumeration devices failed! ErrorCode:") + QString::number(ret);
		QMessageBox::warning(this, warn_title, warn_str);
		return;
	}

	if (m_deviceInfoList.nDevNum >= 1)
	{
		ui.cb_device->setEnabled(true);
		ui.btn_openDevice->setEnabled(true);

		for (unsigned int i = 0; i < m_deviceInfoList.nDevNum; i++)
		{
			ui.cb_device->addItem(m_deviceInfoList.pDevInfo[i].cameraKey);
		}

		ui.wid_cam->SetCamera(m_deviceInfoList.pDevInfo[0].cameraKey);
	}
	else {
		const QString warn_title = QString("Device not Found!");
		const QString warn_str = QString("Device not found, please check interface connection!");
		QMessageBox::warning(this, warn_title, warn_str);
		return;
	}
}

// 打开设备
void DahuaVision::btn_openDevice_clicked(){ 
	if (!ui.wid_cam->CameraOpen())
	{
		const QString warn_title = QString("Device Could Not Open!");
		const QString warn_str = QString("Device Could Not Open!");
		QMessageBox::warning(this, warn_title, warn_str);
		return;
	}

	ui.btn_findDevice->setEnabled(false);
	ui.btn_openDevice->setEnabled(false);
	ui.btn_closeDevice->setEnabled(true);
	ui.btn_startCapturing->setEnabled(true);
	ui.btn_pauseCapture->setEnabled(false);
	ui.cb_device->setEnabled(false);
	ui.gb_trigger->setEnabled(true);

	ui.wid_cam->resetStatistic();
	QString strStatic = ui.wid_cam->getStatistic();
	ui.lb_framecount->setText(strStatic);
}

// 关闭设备
void DahuaVision::btn_closeDevice_clicked() {
	// 暂停采集
	btn_pauseCapturing_clicked();

	ui.wid_cam->CameraClose();
	  
	ui.lb_framecount->setText("");
	  
	ui.btn_openDevice->setEnabled(true);
	ui.btn_closeDevice->setEnabled(false);
	ui.btn_startCapturing->setEnabled(false);
	ui.btn_pauseCapture->setEnabled(false);
	ui.cb_device->setEnabled(true);
	ui.gb_image->setEnabled(false);
	ui.gb_trigger->setEnabled(false);
}

// 开始采集
void DahuaVision::btn_startCapturing_clicked() {
	if (ui.rb_continous->isChecked())
	{
		ui.wid_cam->CameraChangeTrig(CamWidget::trigContinous);		// 连续采集
	}
	else
	{
		ui.wid_cam->CameraChangeTrig(CamWidget::trigLine);			// 外部触发
	}
	

	sb_expTime_valueChanged(ui.sb_expTime->value());
	sb_gain_valueChanged(ui.sb_gain->value());
	  
	ui.wid_cam->CameraStart();
	  
	ui.btn_startCapturing->setEnabled(false);
	ui.btn_pauseCapture->setEnabled(true);
	ui.gb_image->setEnabled(true);
	ui.gb_param->setEnabled(true);
	ui.rb_continous->setEnabled(false);
	ui.rb_trigLine->setEnabled(false);

	ui.wid_cam->resetStatistic();
	m_staticTimer.start(100);
}

// 暂停采集
void DahuaVision::btn_pauseCapturing_clicked() {
	m_staticTimer.stop();

	ui.wid_cam->CameraStop();

	ui.btn_startCapturing->setEnabled(true);
	ui.btn_pauseCapture->setEnabled(false);
	ui.gb_param->setEnabled(false);
	ui.rb_continous->setEnabled(true);
	ui.rb_trigLine->setEnabled(true);
}

// 保存图像
void DahuaVision::btn_saveImage_clicked() {
	btn_pauseCapturing_clicked();

	const QImage img = ui.wid_cam->getImg();
	const QString filePath = QFileDialog::getSaveFileName(
		this, "Save Image", QDir::homePath(), "JPEG Image(*.jpg);;BMP Image(*.bmp)");

	if (!filePath.isEmpty()) {
		bool saved = img.save(filePath);
		if (saved) {
			qDebug() << "Image saved successfully!";
		}
		else {
			qDebug() << "Failed to save image!";
		}
	}
	else {
		qDebug() << "Save operation canceled!";
	}
	btn_startCapturing_clicked();
}

// 变更曝光时间
void DahuaVision::sb_expTime_valueChanged(int dExposureTime) {
	const double value = ui.sb_expTime->value();
	ui.wid_cam->SetExposeTime(value);
	ui.pb_expTime->setValue(value);
}

// 变更增益
void DahuaVision::sb_gain_valueChanged(int dGain) {
	const double value = ui.sb_gain->value();
	ui.wid_cam->SetAdjustPlus(value);
	ui.pb_gain->setValue(value);
}

// 设置统计值
void DahuaVision::onTimerStreamStatistic() {
	QString strStatic = ui.wid_cam->getStatistic();
	ui.lb_framecount->setText(strStatic);
}

// 重写窗口关闭事件
void DahuaVision::closeEvent(QCloseEvent* event)  {
	btn_pauseCapturing_clicked();
	ui.wid_cam->CameraClose();
}

// 软触发一次
void DahuaVision::btn_softTrigger_clicked() {
	ui.wid_cam->ExecuteSoftTrig();
}