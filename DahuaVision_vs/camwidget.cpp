#include "camwidget.h"
#include "ui_camwidget.h"

#define DEFAULT_SHOW_RATE (30) // Ĭ����ʾ֡��
#define DEFAULT_ERROR_STRING ("N/A") 
#define MAX_FRAME_STAT_NUM (50) 
#define MIN_LEFT_LIST_NUM (2) 
#define MAX_STATISTIC_INTERVAL (5000000000) // List�ĸ���ʱ������һ֡��ʱ�������

// ȡ���ص�����
static void FrameCallback(IMV_Frame* pFrame, void* pUser)
{
	CamWidget* pCammerWidget = (CamWidget*)pUser;
	if (!pCammerWidget)
	{
		printf("pCammerWidget is NULL!\n");
		return;
	}

	CFrameInfo frameInfo;
	frameInfo.m_nWidth = (int)pFrame->frameInfo.width;
	frameInfo.m_nHeight = (int)pFrame->frameInfo.height;
	frameInfo.m_nBufferSize = (int)pFrame->frameInfo.size;
	frameInfo.m_nPaddingX = (int)pFrame->frameInfo.paddingX;
	frameInfo.m_nPaddingY = (int)pFrame->frameInfo.paddingY;
	frameInfo.m_ePixelType = pFrame->frameInfo.pixelFormat;
	frameInfo.m_pImageBuf = (unsigned char*)malloc(sizeof(unsigned char) * frameInfo.m_nBufferSize);
	frameInfo.m_nTimeStamp = pFrame->frameInfo.timeStamp;

	// �ڴ�����ʧ�ܣ�ֱ�ӷ���
	if (frameInfo.m_pImageBuf != NULL)
	{
		memcpy(frameInfo.m_pImageBuf, pFrame->pData, frameInfo.m_nBufferSize);

		if (pCammerWidget->m_qDisplayFrameQueue.size() > 16)
		{
			CFrameInfo frameOld;
			if (pCammerWidget->m_qDisplayFrameQueue.get(frameOld))
			{
				free(frameOld.m_pImageBuf);
				frameOld.m_pImageBuf = NULL;
			}
		}

		pCammerWidget->m_qDisplayFrameQueue.push_back(frameInfo);
	}

	pCammerWidget->recvNewFrame(pFrame->frameInfo.size);
}

// ��ʾ�߳�
static unsigned int __stdcall displayThread(void* pUser)
{
	CamWidget* pCammerWidget = (CamWidget*)pUser;
	if (!pCammerWidget)
	{
		printf("pCammerWidget is NULL!\n");
		return -1;
	}

	pCammerWidget->display();

	return 0;
}

CamWidget::CamWidget(QWidget* parent) :
	QWidget(parent)
	, m_currentCameraKey("")
	, m_devHandle(NULL)
	, m_nDisplayInterval(0)
	, m_nFirstFrameTime(0)
	, m_nLastFrameTime(0)
	, m_bNeedUpdate(true)
	, m_nTotalFrameCount(0)
	, m_isExitDisplayThread(false)
	, m_threadHandle(NULL)
{
	ui.setupUi(this);

	qRegisterMetaType<uint64_t>("uint64_t");
	connect(this, SIGNAL(signalShowImage(unsigned char*, int, int, uint64_t)), this, SLOT(ShowImage(unsigned char*, int, int, uint64_t)));

	// Ĭ����ʾ30֡
	setDisplayFPS(DEFAULT_SHOW_RATE);

	m_elapsedTimer.start();

	// ������ʾ�߳�
	m_threadHandle = (HANDLE)_beginthreadex(NULL,
		0,
		displayThread,
		this,
		CREATE_SUSPENDED,
		NULL);

	if (!m_threadHandle)
	{
		printf("Failed to create display thread!\n");
	}
	else
	{
		ResumeThread(m_threadHandle);

		m_isExitDisplayThread = false;
	}
}

CamWidget::~CamWidget()
{
	// �ر���ʾ�߳�
	m_isExitDisplayThread = true;
	WaitForSingleObject(m_threadHandle, INFINITE);
	CloseHandle(m_threadHandle);

}

// �����ع�
bool CamWidget::SetExposeTime(double dExposureTime)
{
	int ret = IMV_OK;

	ret = IMV_SetDoubleFeatureValue(m_devHandle, "ExposureTime", dExposureTime);
	if (IMV_OK != ret)
	{
		printf("set ExposureTime value = %0.2f fail, ErrorCode[%d]\n", dExposureTime, ret);
		return false;
	}

	return true;
}

// ��������
bool CamWidget::SetAdjustPlus(double dGainRaw)
{
	int ret = IMV_OK;

	ret = IMV_SetDoubleFeatureValue(m_devHandle, "GainRaw", dGainRaw);
	if (IMV_OK != ret)
	{
		printf("set GainRaw value = %0.2f fail, ErrorCode[%d]\n", dGainRaw, ret);
		return false;
	}

	return true;
}

// �����
bool CamWidget::CameraOpen(void)
{
	int ret = IMV_OK;

	if (m_currentCameraKey.length() == 0)
	{
		printf("open camera fail. No camera.\n");
		return false;
	}

	if (m_devHandle)
	{
		printf("m_devHandle is already been create!\n");
		return false;
	}
	QByteArray cameraKeyArray = m_currentCameraKey.toLocal8Bit();
	char* cameraKey = cameraKeyArray.data();

	ret = IMV_CreateHandle(&m_devHandle, modeByCameraKey, (void*)cameraKey);
	if (IMV_OK != ret)
	{
		printf("create devHandle failed! cameraKey[%s], ErrorCode[%d]\n", cameraKey, ret);
		return false;
	}

	// �����
	ret = IMV_Open(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("open camera failed! ErrorCode[%d]\n", ret);
		return false;
	}

	return true;
}

// �ر����
bool CamWidget::CameraClose(void)
{
	int ret = IMV_OK;

	if (!m_devHandle)
	{
		printf("close camera fail. No camera.\n");
		return false;
	}

	if (false == IMV_IsOpen(m_devHandle))
	{
		printf("camera is already close.\n");
		return false;
	}

	ret = IMV_Close(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("close camera failed! ErrorCode[%d]\n", ret);
		return false;
	}

	ret = IMV_DestroyHandle(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("destroy devHandle failed! ErrorCode[%d]\n", ret);
		return false;
	}

	m_devHandle = NULL;

	return true;
}

// ��ʼ�ɼ�
bool CamWidget::CameraStart()
{
	int ret = IMV_OK;

	if (IMV_IsGrabbing(m_devHandle))
	{
		printf("camera is already grebbing.\n");
		return false;
	}


	ret = IMV_AttachGrabbing(m_devHandle, FrameCallback, this);
	if (IMV_OK != ret)
	{
		printf("Attach grabbing failed! ErrorCode[%d]\n", ret);
		return false;
	}

	ret = IMV_StartGrabbing(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("start grabbing failed! ErrorCode[%d]\n", ret);
		return false;
	}

	return true;
}

// ֹͣ�ɼ�
bool CamWidget::CameraStop()
{
	int ret = IMV_OK;
	if (!IMV_IsGrabbing(m_devHandle))
	{
		printf("camera is already stop grebbing.\n");
		return false;
	}

	ret = IMV_StopGrabbing(m_devHandle);
	if (IMV_OK != ret)
	{
		printf("Stop grabbing failed! ErrorCode[%d]\n", ret);
		return false;
	}

	// �����ʾ����
	CFrameInfo frameOld;
	while (m_qDisplayFrameQueue.get(frameOld))
	{
		free(frameOld.m_pImageBuf);
		frameOld.m_pImageBuf = NULL;
	}

	m_qDisplayFrameQueue.clear();

	return true;
}

// �л��ɼ���ʽ��������ʽ �������ɼ����ⲿ���������������
bool CamWidget::CameraChangeTrig(ETrigType trigType)
{
	int ret = IMV_OK;

	if (trigContinous == trigType)
	{
		// ���ô���ģʽ
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "Off");
		if (IMV_OK != ret)
		{
			printf("set TriggerMode value = Off fail, ErrorCode[%d]\n", ret);
			return false;
		}
	}
	else if (trigSoftware == trigType)
	{
		// ���ô�����
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSelector", "FrameStart");
		if (IMV_OK != ret)
		{
			printf("set TriggerSelector value = FrameStart fail, ErrorCode[%d]\n", ret);
			return false;
		}

		// ���ô���ģʽ
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "On");
		if (IMV_OK != ret)
		{
			printf("set TriggerMode value = On fail, ErrorCode[%d]\n", ret);
			return false;
		}

		// ���ô���ԴΪ����
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSource", "Software");
		if (IMV_OK != ret)
		{
			printf("set TriggerSource value = Software fail, ErrorCode[%d]\n", ret);
			return false;
		}
	}
	else if (trigLine == trigType)
	{
		// ���ô�����
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSelector", "FrameStart");
		if (IMV_OK != ret)
		{
			printf("set TriggerSelector value = FrameStart fail, ErrorCode[%d]\n", ret);
			return false;
		}

		// ���ô���ģʽ
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "On");
		if (IMV_OK != ret)
		{
			printf("set TriggerMode value = On fail, ErrorCode[%d]\n", ret);
			return false;
		}

		// ���ô���ԴΪLine1����
		ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSource", "Line1");
		if (IMV_OK != ret)
		{
			printf("set TriggerSource value = Line1 fail, ErrorCode[%d]\n", ret);
			return false;
		}
	}

	return true;
}

// ִ��һ������
bool CamWidget::ExecuteSoftTrig(void)
{
	int ret = IMV_OK;

	ret = IMV_ExecuteCommandFeature(m_devHandle, "TriggerSoftware");
	if (IMV_OK != ret)
	{
		printf("ExecuteSoftTrig fail, ErrorCode[%d]\n", ret);
		return false;
	}

	printf("ExecuteSoftTrig success.\n");
	return true;
}

// ���õ�ǰ���
void CamWidget::SetCamera(const QString& strKey)
{
	m_currentCameraKey = strKey;
}

// ��ʾ
bool CamWidget::ShowImage(unsigned char* pRgbFrameBuf, int nWidth, int nHeight, uint64_t nPixelFormat)
{
	QImage image;
	if (NULL == pRgbFrameBuf ||
		nWidth == 0 ||
		nHeight == 0)
	{
		printf("%s image is invalid.\n", __FUNCTION__);
		return false;
	}

	if (gvspPixelMono8 == nPixelFormat)
	{
		image = QImage(pRgbFrameBuf, nWidth, nHeight, QImage::Format_Grayscale8);
	}
	else
	{
		image = QImage(pRgbFrameBuf, nWidth, nHeight, QImage::Format_RGB888);
	}
	
	m_img = image.copy();

	// ��QImage�Ĵ�С���������죬��label�Ĵ�С����һ�¡�����label������ʾ������ͼƬ
	QImage imageScale = image.scaled(QSize(ui.lb_cam->width(), ui.lb_cam->height()));
	QPixmap pixmap = QPixmap::fromImage(imageScale);
	ui.lb_cam->setPixmap(pixmap);
	free(pRgbFrameBuf);

	return true;
}

// ��ʾ�߳�
void CamWidget::display()
{
	while (!m_isExitDisplayThread)
	{
		CFrameInfo frameInfo;

		if (false == m_qDisplayFrameQueue.get(frameInfo))
		{
			Sleep(1);
			continue;
		}

		// �ж��Ƿ�Ҫ��ʾ��������ʾ���ޣ�30֡�����Ͳ���ת�롢��ʾ����
		if (!isTimeToDisplay())
		{
			// �ͷ��ڴ�
			free(frameInfo.m_pImageBuf);
			continue;
		}

		// mono8��ʽ�ɲ���ת�룬ֱ����ʾ��������ʽ��Ҫ����ת�������ʾ
		if (gvspPixelMono8 == frameInfo.m_ePixelType)
		{
			// ��ʾ�߳��з�����ʾ�źţ������߳�����ʾͼ��
			emit signalShowImage(frameInfo.m_pImageBuf, (int)frameInfo.m_nWidth, (int)frameInfo.m_nHeight, (uint64_t)frameInfo.m_ePixelType);
		}
		else
		{
			// ת�� 
			unsigned char* pRGBbuffer = NULL;
			int nRgbBufferSize = 0;
			nRgbBufferSize = frameInfo.m_nWidth * frameInfo.m_nHeight * 3;
			pRGBbuffer = (unsigned char*)malloc(nRgbBufferSize);
			if (pRGBbuffer == NULL)
			{
				// �ͷ��ڴ�
				free(frameInfo.m_pImageBuf);
				printf("RGBbuffer malloc failed.\n");
				continue;
			}

			IMV_PixelConvertParam stPixelConvertParam;
			stPixelConvertParam.nWidth = frameInfo.m_nWidth;
			stPixelConvertParam.nHeight = frameInfo.m_nHeight;
			stPixelConvertParam.ePixelFormat = frameInfo.m_ePixelType;
			stPixelConvertParam.pSrcData = frameInfo.m_pImageBuf;
			stPixelConvertParam.nSrcDataLen = frameInfo.m_nBufferSize;
			stPixelConvertParam.nPaddingX = frameInfo.m_nPaddingX;
			stPixelConvertParam.nPaddingY = frameInfo.m_nPaddingY;
			stPixelConvertParam.eBayerDemosaic = demosaicNearestNeighbor;
			stPixelConvertParam.eDstPixelFormat = gvspPixelRGB8;
			stPixelConvertParam.pDstBuf = pRGBbuffer;
			stPixelConvertParam.nDstBufSize = nRgbBufferSize;

			int ret = IMV_PixelConvert(m_devHandle, &stPixelConvertParam);
			if (IMV_OK != ret)
			{
				// �ͷ��ڴ�
				printf("image convert to RGB failed! ErrorCode[%d]\n", ret);
				free(frameInfo.m_pImageBuf);
				free(pRGBbuffer);
				continue;
			}

			// �ͷ��ڴ�
			free(frameInfo.m_pImageBuf);

			// ��ʾ�߳��з�����ʾ�źţ������߳�����ʾͼ��
			emit signalShowImage(pRGBbuffer, (int)stPixelConvertParam.nWidth, (int)stPixelConvertParam.nHeight, (uint64_t)stPixelConvertParam.eDstPixelFormat);
		}
	}
}

bool CamWidget::isTimeToDisplay()
{
	m_mxTime.lock();

	// ����ʾ
	if (m_nDisplayInterval <= 0)
	{
		m_mxTime.unlock();
		return false;
	}

	// ��һ֡������ʾ
	if (m_nFirstFrameTime == 0 || m_nLastFrameTime == 0)
	{
		m_nFirstFrameTime = m_elapsedTimer.nsecsElapsed();
		m_nLastFrameTime = m_nFirstFrameTime;

		m_mxTime.unlock();
		return true;
	}

	// ��ǰ֡����һ֡�ļ�����������ʾ�������ʾ
	uint64_t nCurTimeTmp = m_elapsedTimer.nsecsElapsed();
	uint64_t nAcquisitionInterval = nCurTimeTmp - m_nLastFrameTime;
	if (nAcquisitionInterval > m_nDisplayInterval)
	{
		m_nLastFrameTime = nCurTimeTmp;
		m_mxTime.unlock();
		return true;
	}

	// ��ǰ֡����ڵ�һ֡��ʱ����
	uint64_t nPre = (m_nLastFrameTime - m_nFirstFrameTime) % m_nDisplayInterval;
	if (nPre + nAcquisitionInterval > m_nDisplayInterval)
	{
		m_nLastFrameTime = nCurTimeTmp;
		m_mxTime.unlock();
		return true;
	}

	m_mxTime.unlock();
	return false;
}

// ������ʾƵ��
void CamWidget::setDisplayFPS(int nFPS)
{
	m_mxTime.lock();
	if (nFPS > 0)
	{
		m_nDisplayInterval = 1000 * 1000 * 1000.0 / nFPS;
	}
	else
	{
		m_nDisplayInterval = 0;
	}
	m_mxTime.unlock();
}

// ���ڹر���Ӧ����
void CamWidget::closeEvent(QCloseEvent* event)
{
	IMV_DestroyHandle(m_devHandle);
	m_devHandle = NULL;
}

// ״̬��ͳ����Ϣ ��ʼ
void CamWidget::resetStatistic()
{
	QMutexLocker locker(&m_mxStatistic);
	m_nTotalFrameCount = 0;
	m_listFrameStatInfo.clear();
	m_bNeedUpdate = true;
}
QString CamWidget::getStatistic()
{
	if (m_mxStatistic.tryLock(30))
	{
		if (m_bNeedUpdate)
		{
			updateStatistic();
		}

		m_mxStatistic.unlock();
		return m_strStatistic;
	}
	return "";
}
void CamWidget::updateStatistic()
{
	size_t nFrameCount = m_listFrameStatInfo.size();  // ֡��
	QString strFPS = DEFAULT_ERROR_STRING;  // ֡��
	QString strSpeed = DEFAULT_ERROR_STRING;

	if (nFrameCount > 1)
	{
		quint64 nTotalSize = 0;
		FrameList::const_iterator it = m_listFrameStatInfo.begin();

		if (m_listFrameStatInfo.size() == 2)
		{
			nTotalSize = m_listFrameStatInfo.back().m_nFrameSize;
		}
		else
		{
			for (++it; it != m_listFrameStatInfo.end(); ++it)
			{
				nTotalSize += it->m_nFrameSize;
			}
		}

		const FrameStatInfo& first = m_listFrameStatInfo.front();
		const FrameStatInfo& last = m_listFrameStatInfo.back();

		qint64 nsecs = last.m_nPassTime - first.m_nPassTime;

		if (nsecs > 0)
		{
			double dFPS = (nFrameCount - 1) * ((double)1000000000.0 / nsecs);
			double dSpeed = nTotalSize * ((double)1000000000.0 / nsecs) / (1000.0) / (1000.0) * (8.0);
			strFPS = QString::number(dFPS, 'f', 2);
			strSpeed = QString::number(dSpeed, 'f', 2);
		}
	}

	m_strStatistic = QString("Stream: %1 images   %2 FPS   %3 Mbps")
		.arg(m_nTotalFrameCount)
		.arg(strFPS)
		.arg(strSpeed);
	m_bNeedUpdate = false;
}

void CamWidget::recvNewFrame(quint32 frameSize)
{
	QMutexLocker locker(&m_mxStatistic);
	if (m_listFrameStatInfo.size() >= MAX_FRAME_STAT_NUM)
	{
		m_listFrameStatInfo.pop_front();
	}
	m_listFrameStatInfo.push_back(FrameStatInfo(frameSize, m_elapsedTimer.nsecsElapsed()));
	++m_nTotalFrameCount;

	if (m_listFrameStatInfo.size() > MIN_LEFT_LIST_NUM)
	{
		FrameStatInfo infoFirst = m_listFrameStatInfo.front();
		FrameStatInfo infoLast = m_listFrameStatInfo.back();
		while (m_listFrameStatInfo.size() > MIN_LEFT_LIST_NUM && infoLast.m_nPassTime - infoFirst.m_nPassTime > MAX_STATISTIC_INTERVAL)
		{
			m_listFrameStatInfo.pop_front();
			infoFirst = m_listFrameStatInfo.front();
		}
	}

	m_bNeedUpdate = true;
}
// ״̬��ͳ����Ϣ end 

// ��ȡ��ǰͼ��
QImage CamWidget::getImg() {
	return m_img;
}
