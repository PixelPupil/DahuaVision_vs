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

// 状态栏统计信息
struct FrameStatInfo
{
    quint32     m_nFrameSize;       // 帧大小, 单位: 字节
    qint64      m_nPassTime;         // 接收到该帧时经过的纳秒数
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

    // 枚举触发方式
    enum ETrigType
    {
        trigContinous = 0,	// 连续拉流
        trigSoftware = 1,	// 软件触发
        trigLine = 2,		// 外部触发
    };

    // 打开相机
    bool CameraOpen(void);

    // 关闭相机
    bool CameraClose(void);

    // 开始采集
    bool CameraStart(void);

    // 停止采集
    bool CameraStop(void);

    // 切换采集方式、触发方式 （连续采集、外部触发、软件触发）
    bool CameraChangeTrig(ETrigType trigType = trigContinous);

    // 执行一次软触发
    bool ExecuteSoftTrig(void);

    // 设置曝光
    bool SetExposeTime(double dExposureTime);

    // 设置增益
    bool SetAdjustPlus(double dGainRaw);

    // 设置当前相机
    void SetCamera(const QString& strKey);

    // 状态栏统计信息
    void recvNewFrame(quint32 frameSize);
    void resetStatistic();
    QString getStatistic();
    void updateStatistic();

    void display();

    // 获取当前画面中的图像
    QImage getImg();


private:
    // 设置显示频率，默认一秒钟显示30帧
    void setDisplayFPS(int nFPS);

    // 计算该帧是否显示
    bool isTimeToDisplay();

    // 窗口关闭响应函数
    void closeEvent(QCloseEvent* event);

private slots:
    // 显示一帧图像
    bool ShowImage(unsigned char* pRgbFrameBuf, int nWidth, int nHeight, uint64_t nPixelFormat);

signals:
    // 显示图像的信号，在displayThread中发送该信号，在主线程中显示
    bool signalShowImage(unsigned char* pRgbFrameBuf, int nWidth, int nHeight, uint64_t nPixelFormat);

public:
    TMessageQue<CFrameInfo>				m_qDisplayFrameQueue;		// 显示队列

private:
    Ui::CamWidget ui;

    QString								m_currentCameraKey;			// 当前相机key
    IMV_HANDLE							m_devHandle;				// 相机句柄

    // 控制显示帧率
    QMutex								m_mxTime;
    int									m_nDisplayInterval;         // 显示间隔
    uint64_t							m_nFirstFrameTime;          // 第一帧的时间戳
    uint64_t							m_nLastFrameTime;           // 上一帧的时间戳
    QElapsedTimer						m_elapsedTimer;				// 用来计时，控制显示帧率

    // 状态栏统计信息
    typedef std::list<FrameStatInfo> FrameList;
    FrameList   m_listFrameStatInfo;
    QMutex      m_mxStatistic;
    quint64     m_nTotalFrameCount;		// 收到的总帧数
    QString     m_strStatistic;			// 统计信息, 不包括错误
    bool		m_bNeedUpdate;

    bool			m_isExitDisplayThread;
    HANDLE			m_threadHandle;

    QImage m_img;                       // 储存当前画面中的图像
};
