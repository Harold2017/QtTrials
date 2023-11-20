#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "modules/video_capture/video_capture_factory.h"
#include "api/video/i420_buffer.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stopBtn->setEnabled(false);
    on_updateDeviceBtn_clicked();

    connect(this, &MainWindow::recvFrame, this, &MainWindow::onRecvFrame, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    CloseVideoCaptureDevice();
    delete ui;
}

void MainWindow::OnFrame(const webrtc::VideoFrame& frame)
{
    static int64_t last = 0;

    QMutexLocker locker(&m_mutex);
    m_i420_buffer = frame.video_frame_buffer()->ToI420();

    //qDebug() << Q_FUNC_INFO << ">>>>>>>frame: " << frame.width() << frame.height() << frame.size();
    if (frame.rotation() != webrtc::kVideoRotation_0)
        m_i420_buffer = webrtc::I420Buffer::Rotate(*m_i420_buffer, frame.rotation());

    qDebug() << "fps: " << 1000'000 / (frame.timestamp_us() - last);
    last = frame.timestamp_us();

    Q_EMIT recvFrame();
}

void MainWindow::on_startBtn_clicked()
{
    OpenVideoCaptureDevice();
}

void MainWindow::on_stopBtn_clicked()
{
    CloseVideoCaptureDevice();
}

void MainWindow::OpenVideoCaptureDevice()
{
    const size_t kWidth = 1920;
    const size_t kHeight = 1080;
    const size_t kFps = 60;
    const size_t kDeviceIndex = ui->deviceComBox->currentIndex();
    /*
    scoped_refptr<T>& operator=(T* p)
    {
    if (p)
        p->AddRef();  // input ref cnt + 1

    if (ptr_) ptr_->Release();  // self ref cnt - 1
        ptr_ = p;
        return *this;
    }

    scoped_refptr<T>& operator=(const scoped_refptr<T>& r)
    {
        return *this = r.ptr_;
    }
    */
    // according to the above source code, this expression will call the second copy assignment function
    // which means the reference count not decrease... it will cause memory leak...
    // so have to manually call `Release` in close function
    m_video_capturer = CameraCapturerTrackSource::Create(kWidth, kHeight, kFps, kDeviceIndex);
    if (m_video_capturer)
    {
        m_video_capturer->AddOrUpdateSink(this, rtc::VideoSinkWants());
        ui->startBtn->setEnabled(false);
        ui->stopBtn->setEnabled(true);
    }
    else
    {
        QMessageBox::warning(this, tr("OpenCamera"), tr("Open Video Capture Device Failed"), QMessageBox::Ok);
    }
}

void MainWindow::CloseVideoCaptureDevice()
{
    if (m_video_capturer)
    {
        m_video_capturer->RemoveSink(this);
        m_video_capturer->Release(); // reference count -1
        m_video_capturer.release();
        ui->startBtn->setEnabled(true);
        ui->stopBtn->setEnabled(false);
    }
}

void MainWindow::on_updateDeviceBtn_clicked()
{
    ui->deviceComBox->clear();
    // get device name
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) return;
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i)
    {
        const uint32_t kSize = 256;
        char name[kSize] = {0};
        char id[kSize] = {0};
        if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) ui->deviceComBox->addItem(name);
    }
}

void MainWindow::onRecvFrame()
{
    QMutexLocker locker(&m_mutex);
    webrtc::I420BufferInterface* buffer = m_i420_buffer.get();
    if (buffer)
    {
        ui->videoWidget->setFrameSize(QSize(buffer->width(), buffer->height()));
        ui->videoWidget->updateTextures(buffer->DataY(), buffer->DataU(), buffer->DataV(), buffer->StrideY(),
                                        buffer->StrideU(), buffer->StrideV());
    }
}