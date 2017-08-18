#ifndef KINECT_DEVICE_H_
#define KINECT_DEVICE_H_

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#pragma warning( disable: 4251 )

#ifdef KINECTVIDEODEVICE_EXPORTS
#define KINECTDEVICE_API __declspec(dllexport)
#else
#define KINECTDEVICE_API __declspec(dllimport)
#endif

namespace kinectdevice{
    class KinectDeviceWin;

    // Data for each kinect video frame.
    struct KINECTDEVICE_API FrameData {
        // w and h are frame width and height.
        FrameData(int w, int h);
        ~FrameData();

        // Color BGR frame buffer, 3 bytes per pixel and stride = 3* width in bytes.
        unsigned char* BGR_buf = 0;
        // Segmentation mask buffer, 1 byte per pixel and stride = width in bytes.
        // Data is in range of [0, 255].
        unsigned char* mask_buf = 0;
        // Frame width and height.
        unsigned int width;
        unsigned int height;

        // Frame capture time and suggested render time.
        std::chrono::time_point<std::chrono::system_clock> capture_time;
        std::chrono::time_point<std::chrono::system_clock> render_time;

        // Index for frame. The index is from 0 and increases with captured time.
        uint64_t frame_index;
    };

    struct KINECTDEVICE_API Configurations {
        // If playback == true, the frame data will be loaded from playback_path instead of aquiring from kinect sensor.
        bool playback = false;
        std::string playback_path = "C:\\TestDataSet\\playback15\\";

        // If record == true and is not in playback mode, the frame data will be saved to record_path.
        // If use_raw == false, the raw date will be preprocessed before it is recorded to disc.
        bool record = false;
        bool use_raw = false;
        std::string record_path = "C:\\temp\\record\\";

        // If cache_data == true, in record mode, multi frames will be stored to memory cache first
        // and saved to disc when cache is full.
        // In playback mode, multi frames will be loaded to memory cache and then frames are processed
        // one by one.
        bool cache_data = false;
        int max_frames_in_cache = 200;

        // If async_process == false, a single thread does frame capture and process.
        // otherwise, a thread is for frame capture and process_threads_number threads are for image process.
        bool async_process = true;
        int process_threads_number = 3;

        // Time and status log.
        bool process_time_measure_log = false;
        bool process_status_log = false;
        std::string log_path = "c:\\temp\\";

        // OpenCv face detection model path.
        std::string face_model_path =
            "C:\\TestDataSet\\haarcascade_frontalface_default.xml";
    };

    class KINECTDEVICE_API KinectDevice {
    public:
        KinectDevice(std::function<void(std::unique_ptr<FrameData>)> callback);
        ~KinectDevice();

        bool Start();
        bool Stop();
        bool Pause();
        bool Resume();

        // Return true if the sensor is started.
        bool IsStarted();
        // Return true if the sensor is capturing and processing frame.
        bool IsPlaying();

        // Record a frame and associate image processing debug information.
        void Screenshot();
        void GetFrameSize(int& width, int& height);

        // Set and get configurations.
        // The configuration need set before Start() to take effective.
        Configurations GetConfig();
        void SetConfig(Configurations config);

    private:
        std::unique_ptr<KinectDeviceWin> device_;
    };

} // namespace kinectdevice

#endif KINECT_DEVICE_H_
