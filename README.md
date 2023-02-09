# TQLib - Task Queue Library

从 webrtc m111 拆出来的强大的线程库, 所有平台都可以选择使用libevent，也可以选择iOS/macOS使用GCD，Windows使用win32线程，Linux/Android使用c++ stdlib线程。
支持串行任务、延迟任务

依赖：
abseil-cpp

基础用法：
----
```C++

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

    q->PostTask([&event](){
        cout << "Hello World!" << endl;
        event.Set();
    });

    event.Wait(webrtc::TimeDelta::Seconds(10));

    return 0;
}

```
