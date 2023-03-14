//
//  sigslot2.hpp
//  rcv
//
//  Created by Jackie Ou on 2023/03/06.
//  Copyright © 2023 RingCentral. All rights reserved.
//

#ifndef RTC_BASE_THIRD_PARTY_SIGSLOT2_SIGSLOT_H_
#define RTC_BASE_THIRD_PARTY_SIGSLOT2_SIGSLOT_H_

#include <cstring>
#include <list>
#include <future>
#include <mutex>
#include "rtc_base/thread.h"

#if defined(WIN32)
#define _SIGSLOT2_HAS_WIN32_THREADS
#include <minwinbase.h>
#elif defined(_ANDROID_) || defined(__APPLE__) || defined(__MACH__)
#define _SIGSLOT2_HAS_POSIX_THREADS
#include <pthread.h>
#else
#error Unsupported platform
#endif

#ifndef SIGSLOT2_DEFAULT_MT_POLICY
#define SIGSLOT2_DEFAULT_MT_POLICY MultiThreadedGlobal
#endif

#ifndef _ANNOTATE_ACCESS_SPECIFIER
#define _ANNOTATE_ACCESS_SPECIFIER(x)
#endif

#ifndef slots
#define slots _ANNOTATE_ACCESS_SPECIFIER(rcv_slot)
#endif

#ifndef signals
#define signals public _ANNOTATE_ACCESS_SPECIFIER(rcv_signal)
#endif

namespace sigslot2 {

const static std::string TAG("sigslot");

#ifdef _SIGSLOT2_HAS_WIN32_THREADS
class MultiThreadedGlobal {
public:
    MultiThreadedGlobal() {
        static bool isInitialised = false;
        if (!isInitialised) {
            InitializeCriticalSection(getCriticalSection());
            isInitialised = true;
        }
    }

    void lock() {
        EnterCriticalSection(getCriticalSection());
    }

    void unlock() {
        LeaveCriticalSection(getCriticalSection());
    }

private:
    CRITICAL_SECTION* getCriticalSection() {
        static CRITICAL_SECTION g_criticalSection;
        return &g_criticalSection;
    }
};

class MultiThreadedLocal {
public:
    MultiThreadedLocal() {
        InitializeCriticalSection(&m_criticalSection);
    }

    MultiThreadedLocal(const MultiThreadedLocal&) {
        InitializeCriticalSection(&m_criticalSection);
    }

    ~MultiThreadedLocal() {
        DeleteCriticalSection(&m_criticalSection);
    }

    void lock() {
        EnterCriticalSection(&m_criticalSection);
    }

    void unlock() {
        LeaveCriticalSection(&m_criticalSection);
    }

private:
    CRITICAL_SECTION m_criticalSection;
};
#endif  // _SIGSLOT2_HAS_WIN32_THREADS

#ifdef _SIGSLOT2_HAS_POSIX_THREADS
class MultiThreadedGlobal {
public:
    void lock() {
        pthread_mutex_lock(getMutex());
    }

    void unlock() {
        pthread_mutex_unlock(getMutex());
    }

private:
    static pthread_mutex_t* getMutex();
};

class MultiThreadedLocal {
public:
    MultiThreadedLocal() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    MultiThreadedLocal(const MultiThreadedLocal&) {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~MultiThreadedLocal() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex;
};
#endif  // _SIGSLOT2_HAS_POSIX_THREADS

template <class MTPolicy>
class LockBlock {
public:
    LockBlock(MTPolicy* mtx) : m_mutex(mtx) {
        m_mutex->lock();
    }

    ~LockBlock() {
        m_mutex->unlock();
    }

private:
    MTPolicy* m_mutex;
};

class ISignal;

class IHasSlots : public std::enable_shared_from_this<IHasSlots> {
    template <class MTPolicy>
    friend class SignalBase;

    template <class MTPolicy, typename... Args>
    friend class Signal;

private:
    typedef void (*ConnectCall)(IHasSlots* receiver, std::shared_ptr<ISignal> sender);

    typedef void (*DisconnectCall)(IHasSlots* receiver, ISignal* sender);

    typedef void (*DisconnectAllCall)(IHasSlots* receiver);

protected:
    IHasSlots(ConnectCall connectCall, DisconnectCall disconnectCall, DisconnectAllCall disconnectAllCall)
        : m_connectCall(connectCall)
        , m_disconnectCall(disconnectCall)
        , m_disconnectAllCall(disconnectAllCall) {}

    // Doesn't really need to be virtual,
    // but is for backwards compatibility (it was virtual in a previous version of sigslot).
    virtual ~IHasSlots() {}

private:
    void connect(std::shared_ptr<ISignal> sender) {
        m_connectCall(this, sender);
    }

    void disconnect(ISignal* sender) {
        m_disconnectCall(this, sender);
    }

    void disconnectAll() {
        m_disconnectAllCall(this);
    }

private:
    const ConnectCall m_connectCall;

    const DisconnectCall m_disconnectCall;

    const DisconnectAllCall m_disconnectAllCall;
};

class ISignal : public std::enable_shared_from_this<ISignal> {
private:
    typedef void (*DisconnectCall)(ISignal* sender, IHasSlots* receiver);

protected:
    ISignal(DisconnectCall disconnectCall)
    : m_disconnectCall(disconnectCall) {

    }

    virtual ~ISignal() {}

public:
    void disconnect(IHasSlots* receiver) {
        m_disconnectCall(this, receiver);
    }

private:
    const DisconnectCall m_disconnectCall;
};

enum ConnectionType {
    AutoConnection = 0,

    DirectConnection = 1,

    QueuedConnection = 2,

    BlockingQueuedConnection = 3,

    UniqueConnection = 0x80,

    SingleShotConnection = 0x100
};

class OpaqueConnection : public std::enable_shared_from_this<OpaqueConnection> {
protected:
    typedef void (*EmitT)(const OpaqueConnection*);

    template <typename FromT, typename ToT>
    union UnionCaster {
        FromT from;
        ToT to;
    };

public:
    OpaqueConnection(std::shared_ptr<IHasSlots> receiver, uint32_t type, rtc::Thread* thread, bool isFunc)
    : m_receiver(receiver)
    , m_type(type)
    , m_thread(thread)
    , m_isFunc(isFunc) {

    }

    virtual ~OpaqueConnection() {}

    inline std::shared_ptr<IHasSlots> receiver() const {
        return m_receiver.lock();
    }

    inline ConnectionType type() const {
        return static_cast<ConnectionType>(m_type);
    }

    inline bool isFunc() const {
        return m_isFunc;
    }

    inline bool isSingleShot() const {
        return m_isSingleShot;
    }

    inline void setEmitted(bool hasEmitted) {
        m_hasEmitted = hasEmitted;
    }

    inline bool hasEmitted() const {
        return m_hasEmitted;
    }

    inline void setFired(bool fired) {
        m_hasFired.store(fired);
    }

    inline bool hasFired() const {
        return m_hasFired.load();
    }

    inline bool expired() const {
        if (!isFunc()) {
            if (!isSingleShot()) {
                return receiver() == nullptr;
            } else {
                return receiver() == nullptr || hasFired();
            }
        } else {
            return false;
        }
    }

    // Just calls the stored "emitter" function pointer stored at construction time.
    template <typename... Args>
    void emit(Args... args) {
        if (expired()) {
            return;
        }

        typedef void (*EmitTTo)(const OpaqueConnection*, Args...);
        UnionCaster<EmitT, EmitTTo> caster;
        caster.from = m_emit;

        if (m_type == ConnectionType::AutoConnection) {
            if (!m_thread) {
                m_type = ConnectionType::DirectConnection;
            } else {
                m_type = m_thread->IsCurrent() ? ConnectionType::DirectConnection : ConnectionType::QueuedConnection;
            }
        }

        if (m_type == ConnectionType::DirectConnection) {
            (caster.to)(this, args...);
            if (isSingleShot() && !hasFired()) {
                setFired(true);
            }
        } else if (m_type == ConnectionType::QueuedConnection) {
            m_thread->PostTask([wself = std::weak_ptr<OpaqueConnection>(shared_from_this()), caster, args...]() mutable {
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                if (!self->expired()) {
                    (caster.to)(self.get(), args...);
                    if (self->isSingleShot() && !self->hasFired()) {
                        self->setFired(true);
                    }
                }
            });
        } else if (m_type == ConnectionType::BlockingQueuedConnection) {
            m_thread->BlockingCall([this, &caster, &args...](){
                (caster.to)(this, args...);
            });
            if (isSingleShot() && !hasFired()) {
                setFired(true);
            }
        }
    }

private:
    OpaqueConnection(const OpaqueConnection&) = delete;

    OpaqueConnection& operator=(const OpaqueConnection&) = delete;

    OpaqueConnection(OpaqueConnection&&) = delete;

    OpaqueConnection& operator=(OpaqueConnection&&) = delete;

protected:
    EmitT m_emit;

    std::weak_ptr<IHasSlots> m_receiver;

    uint32_t m_type = 0;

    rtc::Thread* m_thread;

    bool m_isFunc = false;

    bool m_isSingleShot = false;

    bool m_hasEmitted = false;

    std::atomic_bool m_hasFired = { false };
};

template <typename... Args>
class OpaqueConnectionF : public OpaqueConnection {
public:
    typedef std::function<void (Args...)> Func;

    OpaqueConnectionF(Func f, uint32_t type, rtc::Thread* thread)
    : OpaqueConnection(nullptr, type, thread, true)
    , m_func(std::move(f)) {
        assert(m_func);
        if (!m_func) {
            return;
        }

        typedef void (*EmitTFrom)(const OpaqueConnection* self, Args...);
        UnionCaster<EmitTFrom, EmitT> caster;
        caster.from = &OpaqueConnectionF::emitter;
        m_emit = caster.to;

        m_isSingleShot = m_type & ConnectionType::SingleShotConnection;
        m_type &= ~ConnectionType::SingleShotConnection;
    }

private:
    static void emitter(const OpaqueConnection* conn, Args... args) {
        auto self = static_cast<const OpaqueConnectionF*>(conn);
        if (const auto& func = self->m_func) {
            func(args...);
        }
    }

private:
    Func m_func;
};

template <typename Receiver, typename... Args>
class OpaqueConnectionMF : public OpaqueConnection {
public:
    typedef std::function<void (Args...)> Method;

    OpaqueConnectionMF(std::shared_ptr<Receiver> receiver, void (Receiver::*method)(Args...), uint32_t type, rtc::Thread* thread)
    : OpaqueConnection(receiver, type, thread, false) {
        assert(receiver);
        assert(method);
        if (!receiver || !method) {
            return;
        }

        m_method = [wobj = std::weak_ptr<Receiver>(receiver), method](auto&&... args) {
            if (auto obj = wobj.lock()) {
                (static_cast<Receiver*>(obj.get())->*(method))(std::forward<decltype(args)>(args)...);
            }
        };

        typedef void (*EmitTFrom)(const OpaqueConnection* self, Args...);
        UnionCaster<EmitTFrom, EmitT> caster;
        caster.from = &OpaqueConnectionMF::emitter;
        m_emit = caster.to;

        m_isSingleShot = m_type & ConnectionType::SingleShotConnection;
        m_type &= ~ConnectionType::SingleShotConnection;
    }

private:
    static void emitter(const OpaqueConnection* conn, Args... args) {
        auto self = static_cast<const OpaqueConnectionMF*>(conn);
        if (const auto& method = self->m_method) {
            method(args...);
        }
    }

private:
    Method m_method;
};

template <class MTPolicy>
class SignalBase : public ISignal, public MTPolicy {
protected:
    typedef std::list<std::shared_ptr<OpaqueConnection>> ConnectionsList;

    SignalBase()
    : ISignal(&SignalBase::doDisconnect)
    , m_currentIterator(m_connections.end()) {

    }

    virtual ~SignalBase() {

    }

public:
    bool isEmpty() {
        LockBlock<MTPolicy> lock(this);
        return m_connections.empty();
    }

    void disconnectAll() {
        LockBlock<MTPolicy> lock(this);
        while (!m_connections.empty()) {
            if (auto receiver = m_connections.front()->receiver()) {
                receiver->disconnect(this);
            }
            m_connections.pop_front();
        }
        // If disconnectAll is called while the signal is firing,
        // advance the current slot iterator to the end to avoid an invalidated iterator from being dereferenced.
        m_currentIterator = m_connections.end();
    }

#if !defined(NDEBUG)
    bool connected(std::shared_ptr<IHasSlots> receiver) {
        if (!receiver) {
            return false;
        }

        LockBlock<MTPolicy> lock(this);
        ConnectionsList::const_iterator it = m_connections.begin();
        ConnectionsList::const_iterator itEnd = m_connections.end();

        while (it != itEnd) {
            if ((*it)->receiver() == receiver) {
                return true;
            }
            ++it;
        }
        return false;
    }
#endif

    void disconnect(std::shared_ptr<IHasSlots> receiver) {
        if (!receiver) {
            return;
        }

        LockBlock<MTPolicy> lock(this);
        ConnectionsList::iterator it = m_connections.begin();
        ConnectionsList::iterator itEnd = m_connections.end();

        while (it != itEnd) {
            if ((*it)->receiver() == receiver) {
                // If we're currently using this iterator because the signal is firing,
                // advance it to avoid it being invalidated.
                if (m_currentIterator == it) {
                    m_currentIterator = m_connections.erase(it);
                } else {
                    m_connections.erase(it);
                }
                receiver->disconnect(this);
                return;
            }
            ++it;
        }
    }

    void disconnect(std::shared_ptr<OpaqueConnection> conn) {
        if (!conn) {
            return;
        }

        LockBlock<MTPolicy> lock(this);
        ConnectionsList::iterator it = m_connections.begin();
        ConnectionsList::iterator itEnd = m_connections.end();

        while (it != itEnd) {
            if ((*it) == conn) {
                // If we're currently using this iterator because the signal is firing,
                // advance it to avoid it being invalidated.
                if (m_currentIterator == it) {
                    m_currentIterator = m_connections.erase(it);
                } else {
                    m_connections.erase(it);
                }
                return;
            }
            ++it;
        }
    }

private:
    static void doDisconnect(ISignal* sender, IHasSlots* receiver) {
        auto self = static_cast<SignalBase*>(sender);
        if (!self) {
            return;
        }

        LockBlock<MTPolicy> lock(self);
        ConnectionsList::iterator it = self->m_connections.begin();
        ConnectionsList::iterator itEnd = self->m_connections.end();

        while (it != itEnd) {
            ConnectionsList::iterator itNext = it;
            ++itNext;
            auto r = (*it)->receiver();
            if ((*it)->expired() || r.get() == receiver) {
                // If we're currently using this iterator because the signal is firing,
                // advance it to avoid it being invalidated.
                if (self->m_currentIterator == it) {
                    self->m_currentIterator = self->m_connections.erase(it);
                } else {
                    self->m_connections.erase(it);
                }
            }
            it = itNext;
        }
    }

private:
    SignalBase(const SignalBase&) = delete;

    SignalBase& operator=(SignalBase const&) = delete;

    SignalBase(SignalBase&&) = delete;

    SignalBase& operator=(SignalBase&&) = delete;

protected:
    bool m_unique = false;

    ConnectionsList m_connections;

    // Used to handle a slot being disconnected while a signal is firing (iterating m_connections).
    ConnectionsList::iterator m_currentIterator;
};

template <typename Derived, typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
class HasSlots : public IHasSlots, public MTPolicy {
private:
    typedef std::list<std::weak_ptr<ISignal>> SenderList;
    typedef SenderList::const_iterator ConstIterator;

public:
    HasSlots()
    : IHasSlots(&HasSlots::doConnect, &HasSlots::doDisconnect, &HasSlots::doDisconnectAll) {

    }

    ~HasSlots() {
        doDisconnectAll(this);
    }

protected:
    std::shared_ptr<Derived> getSelf() {
        return std::dynamic_pointer_cast<Derived>(shared_from_this());
    }

private:
    HasSlots(const HasSlots&) = delete;

    HasSlots& operator=(HasSlots const&) = delete;

    HasSlots(HasSlots&&) = delete;

    HasSlots& operator=(HasSlots&&) = delete;

private:
    // UT
    const SenderList& senderList() {
        return m_senders;
    }

private:
    static void doConnect(IHasSlots* receiver, std::shared_ptr<ISignal> sender) {
        auto self = static_cast<HasSlots*>(receiver);
        if (!self) {
            return;
        }

        if (std::is_base_of<MultiThreadedLocal, HasSlots>::value) {
            LockBlock<MTPolicy> lock(self);
            self->m_senders.push_back(sender);
        } else {
            self->m_senders.push_back(sender);
        }
    }

    static void _doDisconnect(HasSlots* receiver, ISignal* sender) {
        if (!receiver) {
            return;
        }

        ConstIterator it = receiver->m_senders.begin();
        ConstIterator itEnd = receiver->m_senders.end();

        while (it != itEnd) {
            auto s = (*it).lock();
            if (s && s.get() == sender) {
                it = receiver->m_senders.erase(it);
            } else {
                ++it;
            }
        }
    }

    static void doDisconnect(IHasSlots* receiver, ISignal* sender) {
        auto self = static_cast<HasSlots*>(receiver);
        if (!self) {
            return;
        }

        if (std::is_base_of<MultiThreadedLocal, HasSlots>::value) {
            LockBlock<MTPolicy> lock(self);
            _doDisconnect(self, sender);
        } else {
            _doDisconnect(self, sender);
        }
    }

    static void _doDisconnectAll(HasSlots* receiver) {
        if (!receiver) {
            return;
        }

        while (!receiver->m_senders.empty()) {
            std::list<std::weak_ptr<ISignal>> senders;
            senders.swap(receiver->m_senders);
            auto it = senders.begin();
            auto itEnd = senders.end();

            while (it != itEnd) {
                std::weak_ptr<ISignal> s = *it;
                ++it;
                if (auto sender = s.lock()) {
                    sender->disconnect(receiver);
                }
            }
        }
    }

    static void doDisconnectAll(IHasSlots* receiver) {
        auto self = static_cast<HasSlots*>(receiver);
        if (!self) {
            return;
        }

        if (std::is_base_of<MultiThreadedLocal, HasSlots>::value) {
            LockBlock<MTPolicy> lock(self);
            _doDisconnectAll(self);
        } else {
            _doDisconnectAll(self);
        }
    }

protected:
    SenderList m_senders;
};

template <class MTPolicy, typename... Args>
class Signal : public SignalBase<MTPolicy> {
public:
    typedef typename SignalBase<MTPolicy>::ConnectionsList ConnectionsList;

public:
    Signal() {}

    ~Signal() {}

    std::shared_ptr<OpaqueConnection> connect(std::function<void (Args...)> func,
                                              uint32_t type = ConnectionType::AutoConnection,
                                              rtc::Thread* thread = nullptr) {
        LockBlock<MTPolicy> lock(this);

        // UniqueConnection do not work for lambdas, non-member functions and functors; they only apply to connecting to member functions.
        bool unique = (type & ConnectionType::UniqueConnection) || this->m_unique;
        if (unique) {
            return nullptr;
        }

        uint32_t _type = type;
        _type &= ~ConnectionType::UniqueConnection;
        _type &= ~ConnectionType::SingleShotConnection;

        bool async = (_type == ConnectionType::QueuedConnection) || (_type == ConnectionType::BlockingQueuedConnection);
        if (async && !thread) {
            return nullptr;
        }

        type &= ~ConnectionType::UniqueConnection;
        this->m_unique = unique;
        auto conn = std::make_shared<OpaqueConnectionF<Args...>>(std::move(func), type, thread);
        this->m_connections.push_back(conn);

        return conn;
    }

    template <class Receiver>
    bool connect(std::shared_ptr<Receiver> receiver,
                 void (Receiver::*pMethod)(Args...),
                 uint32_t type = ConnectionType::AutoConnection,
                 rtc::Thread* thread = nullptr) {
        assert(receiver);
        assert(pMethod);
        if (!receiver || !pMethod) {
            return false;
        }

        LockBlock<MTPolicy> lock(this);
        bool unique = (type & ConnectionType::UniqueConnection) || this->m_unique;
        if (unique && this->m_connections.size() > 0) {
            return false;
        }

        uint32_t _type = type;
        _type &= ~ConnectionType::UniqueConnection;
        _type &= ~ConnectionType::SingleShotConnection;

        bool async = (_type == ConnectionType::QueuedConnection) || (_type == ConnectionType::BlockingQueuedConnection);
        if (async && !thread) {
            return false;
        }

        type &= ~ConnectionType::UniqueConnection;
        this->m_unique = unique;
        this->m_connections.push_back(std::make_shared<OpaqueConnectionMF<Receiver, Args...>>(receiver, pMethod, type, thread));

        receiver->connect(ISignal::shared_from_this());

        return true;
    }

    template <typename... Params>
    void emit(Params&&... args) {
        LockBlock<MTPolicy> lock(this);
        this->m_currentIterator = this->m_connections.begin();

        while (this->m_currentIterator != this->m_connections.end()) {
            auto& conn = *this->m_currentIterator;
            ++(this->m_currentIterator);
            if (conn->isSingleShot() && conn->hasEmitted()) {
                continue;
            }
            conn->template emit<Params...>(std::forward<Params>(args)...);
            if (conn->isSingleShot() && !conn->hasEmitted()) {
                conn->setEmitted(true);
            }
        }
    }

protected:
    const ConnectionsList& connections() {
        return this->m_connections;
    }
};

template <class MTPolicy, typename... Args>
class SignalWithThreadPolicy {
    using SIGNAL = Signal<MTPolicy, Args...>;

public:
    SignalWithThreadPolicy()
    : m_signal(std::make_shared<SIGNAL>()) {

    }

    ~SignalWithThreadPolicy() {
        disconnectAll();
    }

    std::shared_ptr<OpaqueConnection> connect(std::function<void (Args...)> func,
                                              uint32_t type = ConnectionType::AutoConnection,
                                              rtc::Thread* thread = nullptr) {
        assert(func);
        return m_signal->connect(std::move(func), type, thread);
    }

    template <class Receiver>
    bool connect(std::shared_ptr<Receiver> receiver,
                 void (Receiver::*pMethod)(Args...),
                 uint32_t type = ConnectionType::AutoConnection,
                 rtc::Thread* thread = nullptr) {
        assert(receiver);
        assert(pMethod);
        return m_signal->connect(receiver, pMethod, type, thread);
    }

    template <typename... Params>
    inline void operator()(Params&&... args) {
        m_signal->emit(std::forward<Params>(args)...);
    }

    template <typename... Params>
    inline void emit(Params&&... args) {
        m_signal->emit(std::forward<Params>(args)...);
    }

    inline bool isEmpty() {
        return m_signal->isEmpty();
    }

    inline void disconnect(std::shared_ptr<IHasSlots> receiver) {
        m_signal->disconnect(receiver);
    }

    inline void disconnect(std::shared_ptr<OpaqueConnection> conn) {
        m_signal->disconnect(conn);
    }

    inline void disconnectAll() {
        m_signal->disconnectAll();
    }

private:
    SignalWithThreadPolicy(const SignalWithThreadPolicy& that) = delete;

    SignalWithThreadPolicy& operator=(const SignalWithThreadPolicy&) = delete;

    SignalWithThreadPolicy(SignalWithThreadPolicy&&) = delete;

    SignalWithThreadPolicy& operator=(SignalWithThreadPolicy&&) = delete;

private:
    // UT
    inline const typename SIGNAL::ConnectionsList& connections() {
        return m_signal->connections();
    }

private:
    std::shared_ptr<SIGNAL> m_signal;
};

template <typename... Args>
using signal = SignalWithThreadPolicy<Args...>;

template <typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal0 = SignalWithThreadPolicy<MTPolicy>;

template <typename A1, typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal1 = SignalWithThreadPolicy<MTPolicy, A1>;

template <typename A1,
          typename A2,
          typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal2 = SignalWithThreadPolicy<MTPolicy, A1, A2>;

template <typename A1,
          typename A2,
          typename A3,
          typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal3 = SignalWithThreadPolicy<MTPolicy, A1, A2, A3>;

template <typename A1,
          typename A2,
          typename A3,
          typename A4,
          typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal4 = SignalWithThreadPolicy<MTPolicy, A1, A2, A3, A4>;

template <typename A1,
          typename A2,
          typename A3,
          typename A4,
          typename A5,
          typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal5 = SignalWithThreadPolicy<MTPolicy, A1, A2, A3, A4, A5>;

template <typename A1,
          typename A2,
          typename A3,
          typename A4,
          typename A5,
          typename A6,
          typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal6 = SignalWithThreadPolicy<MTPolicy, A1, A2, A3, A4, A5, A6>;

template <typename A1,
          typename A2,
          typename A3,
          typename A4,
          typename A5,
          typename A6,
          typename A7,
          typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal7 = SignalWithThreadPolicy<MTPolicy, A1, A2, A3, A4, A5, A6, A7>;

template <typename A1,
          typename A2,
          typename A3,
          typename A4,
          typename A5,
          typename A6,
          typename A7,
          typename A8,
          typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal8 = SignalWithThreadPolicy<MTPolicy, A1, A2, A3, A4, A5, A6, A7, A8>;

}  // namespace rcv

#endif /* RTC_BASE_THIRD_PARTY_SIGSLOT2_SIGSLOT_H_ */
