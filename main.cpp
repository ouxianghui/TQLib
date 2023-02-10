#include <iostream>
#include <thread>
#include "api/task_queue/default_task_queue_factory.h"
#include "rtc_base/event.h"
#include "api/units/time_delta.h"
#include "rtc_base/thread.h"
#include "rtc_base/win32_socket_init.h"
#include "rtc_base/physical_socket_server.h"


using namespace std;

int main()
{
    cout << "Hello World!" << endl;

    cout << "main thread: " << std::this_thread::get_id() << endl;

    rtc::WinsockInitializer winsockInit;
    rtc::PhysicalSocketServer pss;
    rtc::AutoSocketServerThread mainThread(&pss);

    auto main = rtc::ThreadManager::Instance()->CurrentThread();

    auto tqf = webrtc::CreateDefaultTaskQueueFactory();
    auto tq = tqf->CreateTaskQueue("my-queue", webrtc::TaskQueueFactory::Priority::NORMAL);

    rtc::Event event;

    tq->PostTask([&event](){
        cout << "call in queue thread: " << std::this_thread::get_id() << endl;
        event.Set();
    });

    event.Wait(webrtc::TimeDelta::Seconds(10));

    main->PostTask([&main](){
        cout << "call in main thread: " << std::this_thread::get_id() << endl;
        main->Stop();
    });

    mainThread.Run();

    return 0;
}
