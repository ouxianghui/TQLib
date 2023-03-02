// sigslot.h: Signal/Slot classes
//
// Written by Sarah Thompson (sarah@telergy.com) 2002.
//
// License: Public domain. You are free to use this code however you like, with
// the proviso that the author takes on no responsibility or liability for any
// use.
//
// QUICK DOCUMENTATION
//
//        (see also the full documentation at http://sigslot.sourceforge.net/)
//
//    #define switches
//      SIGSLOT2_PURE_ISO:
//        Define this to force ISO C++ compliance. This also disables all of
//        the thread safety support on platforms where it is available.
//
//      SIGSLOT2_USE_POSIX_THREADS:
//        Force use of Posix threads when using a C++ compiler other than gcc
//        on a platform that supports Posix threads. (When using gcc, this is
//        the default - use SIGSLOT2_PURE_ISO to disable this if necessary)
//
//      SIGSLOT2_DEFAULT_MT_POLICY:
//        Where thread support is enabled, this defaults to
//        MultiThreadedGlobal. Otherwise, the default is SingleThreaded.
//        #define this yourself to override the default. In pure ISO mode,
//        anything other than SingleThreaded will cause a compiler error.
//
//    PLATFORM NOTES
//
//      Win32:
//        On Win32, the WEBRTC_WIN symbol must be #defined. Most mainstream
//        compilers do this by default, but you may need to define it yourself
//        if your build environment is less standard. This causes the Win32
//        thread support to be compiled in and used automatically.
//
//      Unix/Linux/BSD, etc.:
//        If you're using gcc, it is assumed that you have Posix threads
//        available, so they are used automatically. You can override this (as
//        under Windows) with the SIGSLOT2_PURE_ISO switch. If you're using
//        something other than gcc but still want to use Posix threads, you
//        need to #define SIGSLOT2_USE_POSIX_THREADS.
//
//      ISO C++:
//        If none of the supported platforms are detected, or if
//        SIGSLOT2_PURE_ISO is defined, all multithreading support is turned
//        off, along with any code that might cause a pure ISO C++ environment
//        to complain. Before you ask, gcc -ansi -pedantic won't compile this
//        library, but gcc -ansi is fine. Pedantic mode seems to throw a lot of
//        errors that aren't really there. If you feel like investigating this,
//        please contact the author.
//
//
//    THREADING MODES
//
//      SingleThreaded:
//        Your program is assumed to be single threaded from the point of view
//        of signal/slot usage (i.e. all objects using signals and slots are
//        created and destroyed from a single thread). Behaviour if objects are
//        destroyed concurrently is undefined (i.e. you'll get the occasional
//        segmentation fault/memory exception).
//
//      MultiThreadedGlobal:
//        Your program is assumed to be multi threaded. Objects using signals
//        and slots can be safely created and destroyed from any thread, even
//        when connections exist. In MultiThreadedGlobal mode, this is
//        achieved by a single global mutex (actually a critical section on
//        Windows because they are faster). This option uses less OS resources,
//        but results in more opportunities for contention, possibly resulting
//        in more context switches than are strictly necessary.
//
//      MultiThreadedLocal:
//        Behaviour in this mode is essentially the same as
//        MultiThreadedGlobal, except that each signal, and each object that
//        inherits HasSlots, all have their own mutex/critical section. In
//        practice, this means that mutex collisions (and hence context
//        switches) only happen if they are absolutely essential. However, on
//        some platforms, creating a lot of mutexes can slow down the whole OS,
//        so use this option with care.
//
//    USING THE LIBRARY
//
//      See the full documentation at http://sigslot.sourceforge.net/
//
// Libjingle specific:
//
// This file has been modified such that HasSlots and signalx do not have to be
// using the same threading requirements. E.g. it is possible to connect a
// HasSlots<SingleThreaded> and signal0<MultiThreadedLocal> or
// HasSlots<MultiThreadedLocal> and signal0<SingleThreaded>.
// If HasSlots is single threaded the user must ensure that it is not trying
// to connect or disconnect to signalx concurrently or data race may occur.
// If signalx is single threaded the user must ensure that disconnect, connect
// or signal is not happening concurrently or data race may occur.

#ifndef RTC_BASE_THIRD_PARTY_SIGSLOT2_SIGSLOT_H_
#define RTC_BASE_THIRD_PARTY_SIGSLOT2_SIGSLOT_H_

#include <cstring>
#include <list>
#include <set>
#include "rtc_base/thread.h"

// On our copy of sigslot.h, we set single threading as default.
#define SIGSLOT2_DEFAULT_MT_POLICY MultiThreadedLocal

#if defined(SIGSLOT2_PURE_ISO) ||                   \
    (!defined(WEBRTC_WIN) && !defined(__GNUG__) && \
    !defined(SIGSLOT2_USE_POSIX_THREADS))
#define _SIGSLOT2_SINGLE_THREADED
#elif defined(WEBRTC_WIN)
#define _SIGSLOT2_HAS_WIN32_THREADS
#include "windows.h"
#elif defined(__GNUG__) || defined(SIGSLOT2_USE_POSIX_THREADS)
#define _SIGSLOT2_HAS_POSIX_THREADS
#include <pthread.h>
#else
#define _SIGSLOT2_SINGLE_THREADED
#endif

#ifndef SIGSLOT2_DEFAULT_MT_POLICY
#ifdef _SIGSLOT2_SINGLE_THREADED
#define SIGSLOT2_DEFAULT_MT_POLICY SingleThreaded
#else
#define SIGSLOT2_DEFAULT_MT_POLICY MultiThreadedLocal
#endif
#endif

// TODO: change this namespace to rtc?
namespace sigslot2 {

class SingleThreaded {
public:
    void lock() {}
    void unlock() {}
};

#ifdef _SIGSLOT2_HAS_WIN32_THREADS
// The multi threading policies only get compiled in if they are enabled.
class MultiThreadedGlobal {
public:
    MultiThreadedGlobal() {
        static bool isInitialised = false;

        if (!isInitialised) {
            InitializeCriticalSection(getCritsec());
            isInitialised = true;
        }
    }

    void lock() { EnterCriticalSection(getCritsec()); }

    void unlock() { LeaveCriticalSection(getCritsec()); }

private:
    CRITICAL_SECTION* getCritsec() {
        static CRITICAL_SECTION g_critsec;
        return &g_critsec;
    }
};

class MultiThreadedLocal {
public:
    MultiThreadedLocal() { InitializeCriticalSection(&m_critsec); }

    MultiThreadedLocal(const MultiThreadedLocal&) {
        InitializeCriticalSection(&m_critsec);
    }

    ~MultiThreadedLocal() { DeleteCriticalSection(&m_critsec); }

    void lock() { EnterCriticalSection(&m_critsec); }

    void unlock() { LeaveCriticalSection(&m_critsec); }

private:
    CRITICAL_SECTION m_critsec;
};
#endif  // _SIGSLOT2_HAS_WIN32_THREADS

#ifdef _SIGSLOT2_HAS_POSIX_THREADS
// The multi threading policies only get compiled in if they are enabled.
class MultiThreadedGlobal {
public:
    void lock() { pthread_mutex_lock(getMutex()); }

    void unlock() { pthread_mutex_unlock(getMutex()); }

private:
    static pthread_mutex_t* getMutex();
};

class MultiThreadedLocal {
public:
    MultiThreadedLocal() { pthread_mutex_init(&m_mutex, nullptr); }

    MultiThreadedLocal(const MultiThreadedLocal&) {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~MultiThreadedLocal() { pthread_mutex_destroy(&m_mutex); }

    void lock() { pthread_mutex_lock(&m_mutex); }

    void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
    pthread_mutex_t m_mutex;
};
#endif  // _SIGSLOT2_HAS_POSIX_THREADS

template <class MTPolicy>
class LockBlock {
public:
    MTPolicy* m_mutex;

    LockBlock(MTPolicy* mtx) : m_mutex(mtx) { m_mutex->lock(); }

    ~LockBlock() { m_mutex->unlock(); }
};

class ISignalBase;

class IHasSlots {
private:
    typedef void (*SignalConnectT)(IHasSlots* self, ISignalBase* sender);

    typedef void (*SignalDisconnectT)(IHasSlots* self, ISignalBase* sender);

    typedef void (*DisconnectAllT)(IHasSlots* self);

protected:
    IHasSlots(SignalConnectT conn, SignalDisconnectT disc, DisconnectAllT discAll)
        : m_signalConnect(conn)
        , m_signalDisconnect(disc)
        , m_disconnectAll(discAll) {}

    // Doesn't really need to be virtual, but is for backwards compatibility
    // (it was virtual in a previous version of sigslot).
    virtual ~IHasSlots() {}

public:
    void signalConnect(ISignalBase* sender) {
        m_signalConnect(this, sender);
    }

    void signalDisconnect(ISignalBase* sender) {
        m_signalDisconnect(this, sender);
    }

    void disconnectAll() { m_disconnectAll(this); }

private:
    const SignalConnectT m_signalConnect;

    const SignalDisconnectT m_signalDisconnect;

    const DisconnectAllT m_disconnectAll;
};

class ISignalBase {
private:
    typedef void (*SlotDisconnectT)(ISignalBase* self, IHasSlots* pSlot);

    typedef void (*SlotDuplicateT)(ISignalBase* self, const IHasSlots* pOldSlot, IHasSlots* pNewSlot);

protected:
    ISignalBase(SlotDisconnectT disc, SlotDuplicateT dupl)
        : m_slotDisconnect(disc), m_slotDuplicate(dupl) {}

    ~ISignalBase() {}

public:
    void slotDisconnect(IHasSlots* pSlot) {
        m_slotDisconnect(this, pSlot);
    }

    void slotDuplicate(const IHasSlots* pOldSlot, IHasSlots* pNewSlot) {
        m_slotDuplicate(this, pOldSlot, pNewSlot);
    }

private:
    const SlotDisconnectT m_slotDisconnect;

    const SlotDuplicateT m_slotDuplicate;
};

enum ConnectionType {
    AutoConnection = 0,
    DirectConnection = 1,
    QueuedConnection = 2,
    BlockingQueuedConnection = 3,
    UniqueConnection = 0x80,
    SingleShotConnection = 0x100
};

class OpaqueConnection {
private:
    typedef void (*EmitT)(const OpaqueConnection*);

    template <typename FromT, typename ToT>
    union UnionCaster {
        FromT from;
        ToT to;
    };

public:
    template <typename DestT, typename... Args>
    OpaqueConnection(DestT* pd, void (DestT::*pm)(Args...), uint32_t type, rtc::Thread* thread)
        : m_dest(pd)
        , m_type(type)
        , m_thread(thread) {
        typedef void (DestT::*PMT)(Args...);
        static_assert(sizeof(PMT) <= sizeof(pMethod), "Size of slot function pointer too large.");
        std::memcpy(pMethod, &pm, sizeof(PMT));
        typedef void (*EmT)(const OpaqueConnection* self, Args...);
        UnionCaster<EmT, EmitT> caster2;
        caster2.from = &OpaqueConnection::emitter<DestT, Args...>;
        m_emit = caster2.to;

        m_singleShot = m_type & ConnectionType::SingleShotConnection;
        m_type &= ~ConnectionType::SingleShotConnection;
    }

    IHasSlots* getDest() const { return m_dest; }

    bool isSingleShot() const { return m_singleShot; }

    OpaqueConnection duplicate(IHasSlots* newTarget) const {
        OpaqueConnection res = *this;
        res.m_dest = newTarget;
        return res;
    }

    // Just calls the stored "emitter" function pointer stored at construction
    // time.
    template <typename... Args>
    void emit(Args... args) const {
        typedef void (*EmT)(const OpaqueConnection*, Args...);
        UnionCaster<EmitT, EmT> caster;
        caster.from = m_emit;
        uint32_t type = m_type;
        if (m_type == ConnectionType::AutoConnection) {
            if (!m_thread) {
                type = ConnectionType::DirectConnection;
            } else {
                type = m_thread->IsCurrent() ? ConnectionType::DirectConnection : ConnectionType::QueuedConnection;
            }
        }
        if (type == ConnectionType::DirectConnection) {
            (caster.to)(this, args...);
        } else if (type == ConnectionType::QueuedConnection) {
            m_thread->PostTask([this, caster, args...](){
                (caster.to)(this, args...);
            });
        } else if (type == ConnectionType::BlockingQueuedConnection) {
            m_thread->BlockingCall([this, &caster, args...](){
                (caster.to)(this, args...);
            });
        }
    }

private:
    template <typename DestT, typename... Args>
    static void emitter(const OpaqueConnection* self, Args... args) {
        typedef void (DestT::*PMT)(Args...);
        PMT pm;
        static_assert(sizeof(PMT) <= sizeof(pMethod), "Size of slot function pointer too large.");
        std::memcpy(&pm, self->pMethod, sizeof(PMT));
        (static_cast<DestT*>(self->m_dest)->*(pm))(args...);
    }

private:
    EmitT m_emit;

    IHasSlots* m_dest = nullptr;

    uint32_t m_type = 0;

    rtc::Thread* m_thread = nullptr;

    bool m_singleShot = false;

    // Pointers to member functions may be up to 16 bytes (24 bytes for MSVC)
    // for virtual classes, so make sure we have enough space to store it.
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned char pMethod[24];
#else
    unsigned char pMethod[16];
#endif
};

template <class MTPolicy>
class SignalBase : public ISignalBase, public MTPolicy {
protected:
    typedef std::list<OpaqueConnection> ConnectionsList;

    SignalBase()
        : ISignalBase(&SignalBase::doSlotDisconnect, &SignalBase::doSlotDuplicate)
        , m_currentIterator(m_connectedSlots.end()) {}

    ~SignalBase() { disconnectAll(); }

private:
    SignalBase& operator=(SignalBase const& that);

public:
    SignalBase(const SignalBase& o)
        : ISignalBase(&SignalBase::doSlotDisconnect, &SignalBase::doSlotDuplicate)
        , m_currentIterator(m_connectedSlots.end()) {
        LockBlock<MTPolicy> lock(this);
        for (const auto& connection : o.m_connectedSlots) {
            connection.getDest()->signalConnect(this);
            m_connectedSlots.push_back(connection);
        }
    }

    bool isEmpty() {
        LockBlock<MTPolicy> lock(this);
        return m_connectedSlots.empty();
    }

    void disconnectAll() {
        LockBlock<MTPolicy> lock(this);
        while (!m_connectedSlots.empty()) {
            IHasSlots* pDest = m_connectedSlots.front().getDest();
            m_connectedSlots.pop_front();
            pDest->signalDisconnect(static_cast<ISignalBase*>(this));
        }
        // If disconnectAll is called while the signal is firing, advance the
        // current slot iterator to the end to avoid an invalidated iterator from
        // being dereferenced.
        m_currentIterator = m_connectedSlots.end();
    }

#if !defined(NDEBUG)
    bool connected(IHasSlots* pClass) {
        LockBlock<MTPolicy> lock(this);
        ConnectionsList::const_iterator it = m_connectedSlots.begin();
        ConnectionsList::const_iterator itEnd = m_connectedSlots.end();
        while (it != itEnd) {
            if (it->getDest() == pClass)
                return true;
            ++it;
        }
        return false;
    }
#endif

    void disconnect(IHasSlots* pClass) {
        LockBlock<MTPolicy> lock(this);
        ConnectionsList::iterator it = m_connectedSlots.begin();
        ConnectionsList::iterator itEnd = m_connectedSlots.end();
        while (it != itEnd) {
            if (it->getDest() == pClass) {
                // If we're currently using this iterator because the signal is firing,
                // advance it to avoid it being invalidated.
                if (m_currentIterator == it) {
                    m_currentIterator = m_connectedSlots.erase(it);
                } else {
                    m_connectedSlots.erase(it);
                }
                pClass->signalDisconnect(static_cast<ISignalBase*>(this));
                return;
            }
            ++it;
        }
    }

private:
    static void doSlotDisconnect(ISignalBase* p, IHasSlots* pSlot) {
        SignalBase* const self = static_cast<SignalBase*>(p);
        LockBlock<MTPolicy> lock(self);
        ConnectionsList::iterator it = self->m_connectedSlots.begin();
        ConnectionsList::iterator itEnd = self->m_connectedSlots.end();
        while (it != itEnd) {
            ConnectionsList::iterator itNext = it;
            ++itNext;
            if (it->getDest() == pSlot) {
                // If we're currently using this iterator because the signal is firing,
                // advance it to avoid it being invalidated.
                if (self->m_currentIterator == it) {
                    self->m_currentIterator = self->m_connectedSlots.erase(it);
                } else {
                    self->m_connectedSlots.erase(it);
                }
            }
            it = itNext;
        }
    }

    static void doSlotDuplicate(ISignalBase* p, const IHasSlots* oldTarget, IHasSlots* newTarget) {
        SignalBase* const self = static_cast<SignalBase*>(p);
        LockBlock<MTPolicy> lock(self);
        ConnectionsList::iterator it = self->m_connectedSlots.begin();
        ConnectionsList::iterator itEnd = self->m_connectedSlots.end();
        while (it != itEnd) {
            if (it->getDest() == oldTarget) {
                self->m_connectedSlots.push_back(it->duplicate(newTarget));
            }
            ++it;
        }
    }

protected:
    bool m_unique = false;

    ConnectionsList m_connectedSlots;

    // Used to handle a slot being disconnected while a signal is
    // firing (iterating m_connectedSlots).
    ConnectionsList::iterator m_currentIterator;

    bool m_eraseCurrentIterator = false;
};

template <class MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
class HasSlots : public IHasSlots, public MTPolicy {
private:
    typedef std::set<ISignalBase*> SenderSet;
    typedef SenderSet::const_iterator ConstIterator;

public:
    HasSlots()
        : IHasSlots(&HasSlots::doSignalConnect, &HasSlots::doSignalDisconnect, &HasSlots::doDisconnectAll) {}

    HasSlots(HasSlots const& o)
        : IHasSlots(&HasSlots::doSignalConnect, &HasSlots::doSignalDisconnect, &HasSlots::doDisconnectAll) {
        LockBlock<MTPolicy> lock(this);
        for (auto* sender : o.m_senders) {
            sender->slotDuplicate(&o, this);
            m_senders.insert(sender);
        }
    }

    ~HasSlots() { this->disconnectAll(); }

private:
    HasSlots& operator=(HasSlots const&);

    static void doSignalConnect(IHasSlots* p, ISignalBase* sender) {
        HasSlots* const self = static_cast<HasSlots*>(p);
        LockBlock<MTPolicy> lock(self);
        self->m_senders.insert(sender);
    }

    static void doSignalDisconnect(IHasSlots* p, ISignalBase* sender) {
        HasSlots* const self = static_cast<HasSlots*>(p);
        LockBlock<MTPolicy> lock(self);
        self->m_senders.erase(sender);
    }

    static void doDisconnectAll(IHasSlots* p) {
        HasSlots* const self = static_cast<HasSlots*>(p);
        LockBlock<MTPolicy> lock(self);
        while (!self->m_senders.empty()) {
            std::set<ISignalBase*> senders;
            senders.swap(self->m_senders);
            ConstIterator it = senders.begin();
            ConstIterator itEnd = senders.end();
            while (it != itEnd) {
                ISignalBase* s = *it;
                ++it;
                s->slotDisconnect(p);
            }
        }
    }

private:
    SenderSet m_senders;
};

template <class MTPolicy, typename... Args>
class SignalWithThreadPolicy : public SignalBase<MTPolicy> {
private:
    typedef SignalBase<MTPolicy> base;

protected:
    typedef typename base::ConnectionsList ConnectionsList;

public:
    SignalWithThreadPolicy() {}

    template <class DestType>
    bool connect(DestType* pClass, void (DestType::*pMemFun)(Args...), uint32_t type = ConnectionType::AutoConnection, rtc::Thread* thread = nullptr) {
        LockBlock<MTPolicy> lock(this);
        bool unique = (type & ConnectionType::UniqueConnection) || this->m_unique;
        if (unique && this->m_connectedSlots.size() > 0) {
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
        this->m_connectedSlots.push_back(OpaqueConnection(pClass, pMemFun, type, thread));
        pClass->signalConnect(static_cast<ISignalBase*>(this));
        return true;
    }

    void emit(Args... args) {
        std::list<IHasSlots*> slotsList;
        {
            LockBlock<MTPolicy> lock(this);
            this->m_currentIterator = this->m_connectedSlots.begin();
            while (this->m_currentIterator != this->m_connectedSlots.end()) {
                OpaqueConnection const& conn = *this->m_currentIterator;
                ++(this->m_currentIterator);
                conn.emit<Args...>(args...);
                if (conn.isSingleShot()) {
                    slotsList.emplace_back(conn.getDest());
                }
            }
        }
        auto it = slotsList.begin();
        while (it != slotsList.end()) {
            this->disconnect(*it);
            ++it;
        }
    }

    void operator()(Args... args) { emit(args...); }
};

// Alias with default thread policy. Needed because both default arguments
// and variadic template arguments must go at the end of the list, so we
// can't have both at once.
template <typename... Args>
using signal = SignalWithThreadPolicy<SIGSLOT2_DEFAULT_MT_POLICY, Args...>;

// The previous verion of sigslot didn't use variadic templates, so you would
// need to write "sigslot::signal2<Arg1, Arg2>", for example.
// Now you can just write "sigslot::signal<Arg1, Arg2>", but these aliases
// exist for backwards compatibility.
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
using signal7 =
SignalWithThreadPolicy<MTPolicy, A1, A2, A3, A4, A5, A6, A7>;

template <typename A1,
          typename A2,
          typename A3,
          typename A4,
          typename A5,
          typename A6,
          typename A7,
          typename A8,
          typename MTPolicy = SIGSLOT2_DEFAULT_MT_POLICY>
using signal8 =
SignalWithThreadPolicy<MTPolicy, A1, A2, A3, A4, A5, A6, A7, A8>;

}  // namespace sigslot2

#endif /* RTC_BASE_THIRD_PARTY_SIGSLOT2_SIGSLOT_H_ */
