// Copyright (c) 2009 - Decho Corporation

#include <boost/bind.hpp>

#include "mordor/atomic.h"
#include "mordor/fiber.h"
#include "mordor/iomanager.h"
#include "mordor/parallel.h"
#include "mordor/sleep.h"
#include "mordor/test/test.h"
#include "mordor/workerpool.h"

using namespace Mordor;
using namespace Mordor::Test;

MORDOR_SUITE_INVARIANT(Scheduler)
{
    MORDOR_TEST_ASSERT(!Scheduler::getThis());
}

static void doNothing()
{
}

// Stop can be called multiple times without consequence
MORDOR_UNITTEST(Scheduler, idempotentStopHijack)
{
    WorkerPool pool;
    pool.stop();
    pool.stop();
}

MORDOR_UNITTEST(Scheduler, idempotentStopHybrid)
{
    WorkerPool pool(2);
    pool.stop();
    pool.stop();
}

MORDOR_UNITTEST(Scheduler, idempotentStopSpawn)
{
    WorkerPool pool(1, false);
    pool.stop();
    pool.stop();
}

// Start can be called multiple times without consequence
MORDOR_UNITTEST(Scheduler, idempotentStartHijack)
{
    WorkerPool pool;
    pool.start();
    pool.start();
}

MORDOR_UNITTEST(Scheduler, idempotentStartHybrid)
{
    WorkerPool pool(2);
    pool.start();
    pool.start();
}

MORDOR_UNITTEST(Scheduler, idempotentStartSpawn)
{
    WorkerPool pool(1, false);
    pool.start();
    pool.start();
}

// When hijacking the calling thread, you can stop() from anywhere within
// it
MORDOR_UNITTEST(Scheduler, stopScheduledHijack)
{
    WorkerPool pool;
    pool.schedule(boost::bind(&Scheduler::stop, &pool));
    pool.dispatch();
}

MORDOR_UNITTEST(Scheduler, stopScheduledHybrid)
{
    WorkerPool pool(2);
    pool.schedule(boost::bind(&Scheduler::stop, &pool));
    pool.yieldTo();
}

// When hijacking the calling thread, you don't need to explicitly start
// or stop the scheduler; it starts on the first yieldTo, and stops on
// destruction
MORDOR_UNITTEST(Scheduler, hijackBasic)
{
    Fiber::ptr doNothingFiber(new Fiber(&doNothing));
    WorkerPool pool;
    MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &pool);
    pool.schedule(doNothingFiber);
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::INIT);
    pool.dispatch();
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::TERM);
}

// Similar to above, but after the scheduler has stopped, yielding
// to it again should implicitly restart it
MORDOR_UNITTEST(Scheduler, hijackMultipleDispatch)
{
    Fiber::ptr doNothingFiber(new Fiber(&doNothing));
    WorkerPool pool;
    MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &pool);
    pool.schedule(doNothingFiber);
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::INIT);
    pool.dispatch();
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::TERM);
    doNothingFiber->reset();
    pool.schedule(doNothingFiber);
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::INIT);
    pool.dispatch();
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::TERM);
}

// Just calling stop should still clear all pending work
MORDOR_UNITTEST(Scheduler, hijackStopOnScheduled)
{
    Fiber::ptr doNothingFiber(new Fiber(&doNothing));
    WorkerPool pool;
    MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &pool);
    pool.schedule(doNothingFiber);
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::INIT);
    pool.stop();
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::TERM);
}

// TODO: could improve this test by having two fibers that
// synchronize and MORDOR_ASSERT( that they are on different threads
MORDOR_UNITTEST(Scheduler, hybridBasic)
{
    Fiber::ptr doNothingFiber(new Fiber(&doNothing));
    WorkerPool pool(2);
    MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &pool);
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::INIT);
    pool.schedule(doNothingFiber);
    Scheduler::yield();
    pool.stop();
    MORDOR_TEST_ASSERT_EQUAL(doNothingFiber->state(), Fiber::TERM);
}

void
otherThreadProc(Scheduler *scheduler, bool &done)
{
    MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), scheduler);
    done = true;
}

MORDOR_UNITTEST(Scheduler, spawnBasic)
{
    bool done = false;
    WorkerPool pool(1, false);
    Fiber::ptr f(new Fiber(
        boost::bind(&otherThreadProc, &pool, boost::ref(done))));
    MORDOR_TEST_ASSERT(!Scheduler::getThis());
    MORDOR_TEST_ASSERT_EQUAL(f->state(), Fiber::INIT);
    MORDOR_TEST_ASSERT(!done);
    pool.schedule(f);
    volatile bool &doneVolatile = done;
    while (!doneVolatile);
    pool.stop();
    MORDOR_TEST_ASSERT_EQUAL(f->state(), Fiber::TERM);
}

MORDOR_UNITTEST(Scheduler, switchToStress)
{
    WorkerPool poolA(1, true), poolB(1, false);

    // Ensure we return to poolA
    SchedulerSwitcher switcher;
    for (int i = 0; i < 1000; ++i) {
        if (i % 2) {
            poolA.switchTo();
            MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &poolA);
        } else {
            poolB.switchTo();
            MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &poolB);
        }
    }
}

void
runInContext(Scheduler &poolA, Scheduler &poolB)
{
    SchedulerSwitcher switcher(&poolB);
    MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &poolB);
    MORDOR_TEST_ASSERT_NOT_EQUAL(Scheduler::getThis(), &poolA);
    MORDOR_THROW_EXCEPTION(OperationAbortedException());
}

MORDOR_UNITTEST(Scheduler, switcherExceptions)
{
    WorkerPool poolA(1, true), poolB(1, false);

    MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &poolA);
    MORDOR_TEST_ASSERT_NOT_EQUAL(Scheduler::getThis(), &poolB);

    MORDOR_TEST_ASSERT_EXCEPTION(runInContext(poolA, poolB), OperationAbortedException);

    MORDOR_TEST_ASSERT_EQUAL(Scheduler::getThis(), &poolA);
    MORDOR_TEST_ASSERT_NOT_EQUAL(Scheduler::getThis(), &poolB);
}

static void increment(int &total)
{
    ++total;
}

MORDOR_UNITTEST(Scheduler, parallelDo)
{
    WorkerPool pool;

    int total = 0;
    std::vector<boost::function<void ()> > dgs;
    dgs.push_back(boost::bind(&increment, boost::ref(total)));
    dgs.push_back(boost::bind(&increment, boost::ref(total)));

    parallel_do(dgs);
    MORDOR_TEST_ASSERT_EQUAL(total, 2);
}

static void exception()
{
    MORDOR_THROW_EXCEPTION(OperationAbortedException());
}

MORDOR_UNITTEST(Scheduler, parallelDoException)
{
    WorkerPool pool;

    std::vector<boost::function<void ()> > dgs;
    dgs.push_back(&exception);
    dgs.push_back(&exception);

    MORDOR_TEST_ASSERT_EXCEPTION(parallel_do(dgs), OperationAbortedException);
}

static bool checkEqual(int x, int &sequence)
{
    MORDOR_TEST_ASSERT_EQUAL(x, sequence);
    ++sequence;
    return true;
}

MORDOR_UNITTEST(Scheduler, parallelForEach)
{
    const int values[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    WorkerPool pool;

    int sequence = 1;
    parallel_foreach(&values[0], &values[10], boost::bind(
        &checkEqual, _1, boost::ref(sequence)), 4);
    MORDOR_TEST_ASSERT_EQUAL(sequence, 11);
}

MORDOR_UNITTEST(Scheduler, parallelForEachLessThanParallelism)
{
    const int values[] = { 1, 2 };
    WorkerPool pool;

    int sequence = 1;
    parallel_foreach(&values[0], &values[2], boost::bind(
        &checkEqual, _1, boost::ref(sequence)), 4);
    MORDOR_TEST_ASSERT_EQUAL(sequence, 3);
}

static bool checkEqualStop5(int x, int &sequence)
{
    MORDOR_TEST_ASSERT_EQUAL(x, sequence);
    ++sequence;
    return sequence <= 5;
}

MORDOR_UNITTEST(Scheduler, parallelForEachStopShort)
{
    const int values[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    WorkerPool pool;

    int sequence = 1;
    parallel_foreach(&values[0], &values[10], boost::bind(
        &checkEqualStop5, _1, boost::ref(sequence)), 4);
    // 5 was told to stop, 6, 7, and 8 were already scheduled
    MORDOR_TEST_ASSERT_EQUAL(sequence, 9);
}

static bool checkEqualExceptionOn5(int x, int &sequence)
{
    MORDOR_TEST_ASSERT_EQUAL(x, sequence);
    ++sequence;
    if (sequence == 6)
        MORDOR_THROW_EXCEPTION(OperationAbortedException());
    return true;
}

MORDOR_UNITTEST(Scheduler, parallelForEachException)
{
    const int values[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    WorkerPool pool;

    int sequence = 1;
    try {
        parallel_foreach(&values[0], &values[10], boost::bind(
            &checkEqualExceptionOn5, _1, boost::ref(sequence)), 4);
        MORDOR_TEST_ASSERT(false);
    } catch (OperationAbortedException)
    {}
    // 5 was told to stop (exception), 6, 7, and 8 were already scheduled
    MORDOR_TEST_ASSERT_EQUAL(sequence, 9);
}

#ifdef DEBUG
MORDOR_UNITTEST(Scheduler, scheduleForThreadNotOnScheduler)
{
    Fiber::ptr doNothingFiber(new Fiber(&doNothing));
    WorkerPool pool(1, false);
    MORDOR_TEST_ASSERT_ASSERTED(pool.schedule(doNothingFiber, gettid()));
    pool.stop();
}
#endif

static void sleepForABit(std::set<tid_t> &threads,
    boost::mutex &mutex, Fiber::ptr scheduleMe, int *count)
{
    {
        boost::mutex::scoped_lock lock(mutex);
        threads.insert(gettid());
    }
    Mordor::sleep(10000);
    if (count && atomicDecrement(*count) == 0)
        Scheduler::getThis()->schedule(scheduleMe);
}

MORDOR_UNITTEST(Scheduler, spreadTheLoad)
{
    std::set<tid_t> threads;
    {
        boost::mutex mutex;
        WorkerPool pool(4);
        // Wait for the other threads to get to idle first
        Mordor::sleep(100000);
        int count = 8;
        for (size_t i = 0; i < 8; ++i)
            pool.schedule(boost::bind(&sleepForABit, boost::ref(threads),
                boost::ref(mutex), Fiber::getThis(), &count));
        // We have to have one of these fibers reschedule us, because if we
        // let the pool destruct, it will call stop which will wake up all
        // the threads
        Scheduler::yieldTo();
    }
    // Make sure we hit every thread
    MORDOR_TEST_ASSERT_EQUAL(threads.size(), 4u);
}

static void fail()
{
    MORDOR_NOTREACHED();
}

static void cancelTheTimer(Timer::ptr timer)
{
    // Wait for the other threads to get to idle first
    Mordor::sleep(100000);
    timer->cancel();
}

MORDOR_UNITTEST(Scheduler, stopIdleMultithreaded)
{
    IOManager ioManager(4);
    unsigned long long start = TimerManager::now();
    Timer::ptr timer = ioManager.registerTimer(10000000ull, &fail);
    // Wait for the other threads to get to idle first
    Mordor::sleep(100000);
    ioManager.schedule(boost::bind(&cancelTheTimer, timer));
    ioManager.stop();
    // This should have taken less than a second, since we cancelled the timer
    MORDOR_TEST_ASSERT_LESS_THAN(TimerManager::now() - start, 1000000ull);
}

static void startTheFibers(std::set<tid_t> &threads,
    boost::mutex &mutex)
{
    Mordor::sleep(100000);
    for (size_t i = 0; i < 8; ++i)
        Scheduler::getThis()->schedule(boost::bind(&sleepForABit,
            boost::ref(threads), boost::ref(mutex), Fiber::ptr(),
            (int *)NULL));
}

MORDOR_UNITTEST(Scheduler, spreadTheLoadWhileStopping)
{
    std::set<tid_t> threads;
    {
        boost::mutex mutex;
        WorkerPool pool(4);
        // Wait for the other threads to get to idle first
        Mordor::sleep(100000);

        pool.schedule(boost::bind(&startTheFibers, boost::ref(threads),
            boost::ref(mutex)));
        pool.stop();
    }
    // Make sure we hit every thread
    MORDOR_TEST_ASSERT_EQUAL(threads.size(), 4u);
}
