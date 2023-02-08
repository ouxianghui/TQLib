#include <iostream>
#include "api/task_queue/default_task_queue_factory.h"
#include "rtc_base/event.h"
#include "api/units/time_delta.h"

using namespace std;

int main()
{
    auto tqf = webrtc::CreateDefaultTaskQueueFactory();
    auto q = tqf->CreateTaskQueue("my-queue", webrtc::TaskQueueFactory::Priority::NORMAL);

    rtc::Event event;

    for (int i = 0; i < 99999; ++i) {
        q->PostTask([&event, i](){
            cout << "Hello World: i: " << i << endl;
            if (i == 99998) {
                event.Set();
            }
        });
    }

    event.Wait(webrtc::TimeDelta::Seconds(10));

    return 0;
}
