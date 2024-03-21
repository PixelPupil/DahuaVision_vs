#pragma once

#include <QWidget>
#include "ui_camwidget.h"

#include <windows.h>
#include <process.h>
#include <QWidget>
#include "IMVApi.h"
#include "MessageQue.h"
#include <QElapsedTimer>
#include <QMutex>

// ״̬��ͳ����Ϣ
struct FrameStatInfo
{
    quint32     m_nFrameSize;       // ֡��С, ��λ: �ֽ�
    qint64      m_nPassTime;         // ���յ���֡ʱ������������
    FrameStatInfo(int nSize, qint64 nTime) : m_nFrameSize(nSize), m_nPassTime(nTime)
    {
    }
};

class CFrameInfo
{
public:
    CFrameInfo()
    {
        m_pImageBuf = NULL;
        m_nBufferSize = 0;
        m_nWidth = 0;
        m_nHeight = 0;
        m_ePixelType = gvspPixelMono8;
        m_nPaddingX = 0;
        m_nPaddingY = 0;
        m_nTimeStamp = 0;
    }

    ~CFrameInfo()
    {
    }

public:
    unsigned char* m_pImageBuf;
    int				m_nBufferSize;
    int				m_nWidth;
    int				m_nHeight;
    IMV_EPixelType	m_ePixelType;
    int				m_nPaddingX;
    int				m_nPaddingY;
    uint64_t		m_nTimeStamp;
};


class CamWidget : public QWidget
{
	Q_OBJECT

public:
	CamWidget(QWidget *parent = nullptr);
	~CamWidget();

    // ö�ٴ�����ʽ
    enum ETrigType
    {
        trigContinous = 0,	// ��������
        trigSoftware = 1,	// �������
        trigLine = 2,		// �ⲿ����
    };

    // �����
    bool CameraOpen(void);

    // �ر����
    bool CameraClose(void);

    // ��ʼ�ɼ�
    bool CameraStart(void);

    // ֹͣ�ɼ�
    bool CameraStop(void);

    // �л��ɼ���ʽ��������ʽ �������ɼ����ⲿ���������������
    bool CameraChangeTrig(ETrigType trigType = trigContinous);

    // ִ��һ������
    bool ExecuteSoftTrig(void);

    // �����ع�
    bool SetExposeTime(double dExposureTime);

    // ��������
    bool SetAdjustPlus(double dGainRaw);

    // ���õ�ǰ���
    void SetCamera(const QString& strKey);

    // ״̬��ͳ����Ϣ
    void recvNewFrame(quint32 frameSize);
    void resetStatistic();
    QString getStatistic();
    void updateStatistic();

    void display();

    // ��ȡ��ǰ�����е�ͼ��
    QImage getImg();


private:
    // ������ʾƵ�ʣ�Ĭ��һ������ʾ30֡
    void setDisplayFPS(int nFPS);

    // �����֡�Ƿ���ʾ
    bool isTimeToDisplay();

    // ���ڹر���Ӧ����
    void closeEvent(QCloseEvent* event);

private slots:
    // ��ʾһ֡ͼ��
    bool ShowImage(unsigned char* pRgbFrameBuf, int nWidth, int nHeight, uint64_t nPixelFormat);

signals:
    // ��ʾͼ����źţ���displayThread�з��͸��źţ������߳�����ʾ
    bool signalShowImage(unsigned char* pRgbFrameBuf, int nWidth, int nHeight, uint64_t nPixelFormat);

public:
    TMessageQue<CFrameInfo>				m_qDisplayFrameQueue;		// ��ʾ����

private:
    Ui::CamWidget ui;

    QString								m_currentCameraKey;			// ��ǰ���key
    IMV_HANDLE							m_devHandle;				// ������

    // ������ʾ֡��
    QMutex								m_mxTime;
    int									m_nDisplayInterval;         // ��ʾ���
    uint64_t							m_nFirstFrameTime;          // ��һ֡��ʱ���
    uint64_t							m_nLastFrameTime;           // ��һ֡��ʱ���
    QElapsedTimer						m_elapsedTimer;				// ������ʱ��������ʾ֡��

    // ״̬��ͳ����Ϣ
    typedef std::list<FrameStatInfo> FrameList;
    FrameList   m_listFrameStatInfo;
    QMutex      m_mxStatistic;
    quint64     m_nTotalFrameCount;		// �յ�����֡��
    QString     m_strStatistic;			// ͳ����Ϣ, ����������
    bool		m_bNeedUpdate;

    bool			m_isExitDisplayThread;
    HANDLE			m_threadHandle;

    QImage m_img;                       // ���浱ǰ�����е�ͼ��
};
