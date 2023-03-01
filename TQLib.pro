TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += \
    $$PWD/ \
    $$PWD/abseil/include

SOURCES += \
    api/task_queue/pending_task_safety_flag.cc \
    api/task_queue/task_queue_base.cc \
    api/units/data_rate.cc \
    api/units/data_size.cc \
    api/units/frequency.cc \
    api/units/time_delta.cc \
    api/units/timestamp.cc \
    main.cpp \
    rtc_base/async_resolver.cc \
    rtc_base/async_resolver_interface.cc \
    rtc_base/checks.cc \
    rtc_base/deprecated/recursive_critical_section.cc \
    rtc_base/event.cc \
    rtc_base/event_tracer.cc \
    rtc_base/file_rotating_stream.cc \
    rtc_base/internal/default_socket_server.cc \
    rtc_base/ip_address.cc \
    rtc_base/log_sinks.cc \
    rtc_base/logging.cc \
    rtc_base/net_helpers.cc \
    rtc_base/network_constants.cc \
    rtc_base/network_monitor.cc \
    rtc_base/null_socket_server.cc \
    rtc_base/physical_socket_server.cc \
    rtc_base/platform_thread.cc \
    rtc_base/platform_thread_types.cc \
    rtc_base/socket.cc \
    rtc_base/socket_address.cc \
    rtc_base/string_encode.cc \
    rtc_base/string_to_number.cc \
    rtc_base/string_utils.cc \
    #rtc_base/strings/json.cc \
    rtc_base/strings/string_builder.cc \
    rtc_base/strings/string_format.cc \
    rtc_base/synchronization/sequence_checker_internal.cc \
    rtc_base/synchronization/yield.cc \
    rtc_base/synchronization/yield_policy.cc \
    rtc_base/system/file_wrapper.cc \
    rtc_base/system_time.cc \
    rtc_base/task_queue.cc \
    rtc_base/task_queue_stdlib.cc \
    rtc_base/task_utils/repeating_task.cc \
    rtc_base/thread.cc \
    rtc_base/time_utils.cc

HEADERS += \
    api/function_view.h \
    api/scoped_refptr.h \
    api/sequence_checker.h \
    api/task_queue/default_task_queue_factory.h \
    api/task_queue/pending_task_safety_flag.h \
    api/task_queue/task_queue_base.h \
    api/task_queue/task_queue_factory.h \
    api/units/data_rate.h \
    api/units/data_size.h \
    api/units/frequency.h \
    api/units/time_delta.h \
    api/units/timestamp.h \
    rtc_base/async_resolver.h \
    rtc_base/async_resolver_interface.h \
    rtc_base/checks.h \
    rtc_base/deprecated/recursive_critical_section.h \
    rtc_base/event.h \
    rtc_base/event_tracer.h \
    rtc_base/file_rotating_stream.h \
    rtc_base/internal/default_socket_server.h \
    rtc_base/ip_address.h \
    rtc_base/log_sinks.h \
    rtc_base/logging.h \
    rtc_base/net_helpers.h \
    rtc_base/network_constants.h \
    rtc_base/network_monitor.h \
    rtc_base/null_socket_server.h \
    rtc_base/physical_socket_server.h \
    rtc_base/platform_thread.h \
    rtc_base/platform_thread_types.h \
    rtc_base/ref_count.h \
    rtc_base/ref_counter.h \
    rtc_base/socket.h \
    rtc_base/socket_address.h \
    rtc_base/socket_factory.h \
    rtc_base/socket_server.h \
    rtc_base/string_encode.h \
    rtc_base/string_to_number.h \
    rtc_base/string_utils.h \
    #rtc_base/strings/json.h \
    rtc_base/strings/string_builder.h \
    rtc_base/strings/string_format.h \
    rtc_base/synchronization/mutex.h \
    rtc_base/synchronization/mutex_abseil.h \
    rtc_base/synchronization/mutex_critical_section.h \
    rtc_base/synchronization/mutex_pthread.h \
    rtc_base/synchronization/sequence_checker_internal.h \
    rtc_base/synchronization/yield.h \
    rtc_base/synchronization/yield_policy.h \
    rtc_base/system/arch.h \
    rtc_base/system/asm_defines.h \
    rtc_base/system/assume.h \
    rtc_base/system/cocoa_threading.h \
    rtc_base/system/file_wrapper.h \
    rtc_base/system/ignore_warnings.h \
    rtc_base/system/inline.h \
    rtc_base/system/no_cfi_icall.h \
    rtc_base/system/no_unique_address.h \
    rtc_base/system/rtc_export.h \
    rtc_base/system/rtc_export_template.h \
    rtc_base/system/unused.h \
    rtc_base/system_time.h \
    rtc_base/task_queue.h \
    rtc_base/task_queue_stdlib.h \
    rtc_base/task_utils/repeating_task.h \
    rtc_base/thread.h \
    rtc_base/thread_annotations.h \
    rtc_base/time_utils.h \
    rtc_base/trace_event.h \
    rtc_base/type_traits.h

mac {
    DEFINES += WEBRTC_MAC WEBRTC_POSIX

    LIBS += -L$$PWD/abseil/lib/ -labsl -framework Foundation

    HEADERS += \
    rtc_base/system/gcd_helpers.h \
    rtc_base/task_queue_gcd.h

    SOURCES += \
    api/task_queue/default_task_queue_factory_gcd.cc \
    rtc_base/system/gcd_helpers.m \
    rtc_base/system/cocoa_threading.mm \
    rtc_base/task_queue_gcd.cc
}

win32 {
    DEFINES += WEBRTC_WIN WIN32_LEAN_AND_MEAN NOMINMAX

    LIBS += -L'C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22000.0/um/x64/' -lWinMM -lUser32 -lWS2_32 -lAdvAPI32 -L'$$PWD/abseil/lib/' -labsl

    HEADERS += \
    rtc_base/win/windows_version.h \
    rtc_base/win32.h \
    rtc_base/win32_socket_init.h \
    rtc_base/task_queue_win.h

    SOURCES += \
    rtc_base/win/windows_version.cc \
    rtc_base/win32.cc \
    rtc_base/task_queue_win.cc \
    api/task_queue/default_task_queue_factory_win.cc
}

linux {
    DEFINES += WEBRTC_LINUX WEBRTC_POSIX

    LIBS += -L$$PWD/abseil/lib/ -labsl -lpthread

    SOURCES += \
    api/task_queue/default_task_queue_factory_stdlib.cc
}
