#include <iostream>
#include <thread>
#include "api/task_queue/default_task_queue_factory.h"
#include "rtc_base/event.h"
#include "api/units/time_delta.h"
#include "rtc_base/thread.h"
#ifdef WEBRTC_WIN
#include "rtc_base/win32_socket_init.h"
#endif
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/third_party/sigslot/sigslot2.h"

using namespace std;

class Controller {
public:
    sigslot2::signal0<> _click;
};

class Worker : public sigslot2::HasSlots<>{
public:
    void onClicked() {
        std::cout << "Worker::onClicked(): " << std::this_thread::get_id() << std::endl;
    }
};

int main()
{
    cout << "Hello World!" << endl;

    cout << "main thread: " << std::this_thread::get_id() << endl;

#ifdef WEBRTC_WIN
    rtc::WinsockInitializer winsockInit;
    rtc::PhysicalSocketServer pss;
    rtc::AutoSocketServerThread mainThread(&pss);
#endif

    //auto main = rtc::ThreadManager::Instance()->CurrentThread();

    auto tqf = webrtc::CreateDefaultTaskQueueFactory();
    auto tq = tqf->CreateTaskQueue("my-queue", webrtc::TaskQueueFactory::Priority::NORMAL);

    rtc::Event event;

    tq->PostTask([&event](){
        cout << "call in queue thread: " << std::this_thread::get_id() << endl;
        event.Set();
    });

    event.Wait(webrtc::TimeDelta::Seconds(10));

    //main->PostTask([&main](){
    //    cout << "call in main thread: " << std::this_thread::get_id() << endl;
    //    main->Stop();
    //});
    //main->Run();

    Controller ctrl;
    Worker worker;
    ctrl._click.connect(&worker, &Worker::onClicked, sigslot2::DirectConnection | sigslot2::SingleShotConnection, &mainThread);

    ctrl._click();
    ctrl._click();

#ifdef WEBRTC_WIN
    mainThread.Run();
#endif

    return 0;
}
