// The MIT License (MIT)

// Copyright (c) 2013 lailongwei<lailongwei@126.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of 
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to 
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of 
// the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "comm/TestCase_Comm_Facade.h"

namespace
{
    class TestFacade : public LLBC_IFacade
    {
    public:
        virtual void OnInitialize()
        {
            LLBC_PrintLine("Service initialize");
        }

        virtual void OnDestroy()
        {
            LLBC_PrintLine("Service destroy");
        }

        virtual void OnStart()
        {
            LLBC_PrintLine("Service start");
            _timer = new LLBC_Timer(new LLBC_Delegate1<void, TestFacade, LLBC_Timer *>(this, &TestFacade::OnTimerTimeout),
                                    new LLBC_Delegate1<void, TestFacade, LLBC_Timer *>(this, &TestFacade::OnTimerCancel));
            _timer->Schedule(2000, 2000);
        }

        virtual void OnStop()
        {
            LLBC_PrintLine("Service stop");
            _timer->Cancel();
            LLBC_XDelete(_timer);
        }

    public:
        virtual void OnUpdate()
        {
            LLBC_PrintLine("Update...");
        }

        virtual void OnIdle(int idleTime)
        {
            LLBC_PrintLine("Idle, idle time: %d...", idleTime);
        }

    public:
        virtual void OnTimerTimeout(LLBC_Timer *timer)
        {
            LLBC_PrintLine("Timer timeout!");
        }

        virtual void OnTimerCancel(LLBC_Timer *timer)
        {
            LLBC_PrintLine("Time cancelled!");
        }

    private:
        LLBC_Timer *_timer;
    };

    class TestFacadeFactory : public LLBC_IFacadeFactory
    {
    public:
        virtual LLBC_IFacade *Create() const
        {
            return LLBC_New(TestFacade);
        }
    };
}

TestCase_Comm_Facade::TestCase_Comm_Facade()
{
}

TestCase_Comm_Facade::~TestCase_Comm_Facade()
{
}

int TestCase_Comm_Facade::Run(int argc, char *argv[])
{
    LLBC_PrintLine("Facade test:");

    // Parse arguments.
    if (argc < 4)
    {
        LLBC_FilePrintLine(stderr, "argument error, eg: ./a {internal-drive | external-drive} <host> <port>");
        return LLBC_FAILED;
    }

    const int port = LLBC_Str2Int32(argv[3]);
    const LLBC_String driveType = LLBC_String(argv[1]).tolower();
    if (driveType == "internal-drive")
        return TestInInternalDriveService(argv[2], port);
    else
        return TestInExternalDriveService(argv[2], port);
}

int TestCase_Comm_Facade::TestInInternalDriveService(const LLBC_String &host, int port)
{
    LLBC_PrintLine("Facade test(In internal-drive service), host: %s, port: %d", host.c_str(), port);

    // Create and init service.
    LLBC_IService *svc = LLBC_IService::Create(LLBC_IService::Normal, "FacadeTest");
    svc->SetFPS(1);
    svc->RegisterFacade<TestFacadeFactory>();

    LLBC_PrintLine("Start service...");
    svc->Start(10);

    LLBC_PrintLine("Press any key to restart service(stop->start)...");
    getchar();
    svc->Stop();
    svc->Start(5);

    LLBC_PrintLine("Press any key to stop service...");
    getchar();
    svc->Stop();

    LLBC_PrintLine("Press any key to destroy service...");
    getchar();
    LLBC_Delete(svc);

    return LLBC_OK;
}

int TestCase_Comm_Facade::TestInExternalDriveService(const LLBC_String &host, int port)
{
    LLBC_PrintLine("Facade test(In external-drive service), host: %s, port: %d", host.c_str(), port);

    // Create and init service.
    LLBC_IService *svc = LLBC_IService::Create(LLBC_IService::Normal, "FacadeTest");
    svc->SetFPS(1);
    svc->RegisterFacade<TestFacadeFactory>();
    svc->SetDriveMode(LLBC_IService::ExternalDrive);

    LLBC_PrintLine("Start service...");
    svc->Start(2);

    int waitSecs = 10, nowWaited = 0;
    LLBC_PrintLine("Calling Service.OnSvc, %d seconds later will restart service...", waitSecs);
    while (nowWaited != waitSecs)
    {
        svc->OnSvc();
        ++nowWaited;
    }

    LLBC_PrintLine("Restart service...");
    svc->Stop();
    svc->Start(2);

    LLBC_PrintLine("Calling Service.OnSvc, %d seconds later will stop service...", waitSecs);
    nowWaited = 0;
    while (nowWaited != waitSecs)
    {
        svc->OnSvc();
        ++nowWaited;
    }

    LLBC_PrintLine("Stop service...");
    svc->Stop();

    LLBC_PrintLine("Press any key to destroy service...");
    getchar();
    LLBC_Delete(svc);

    return LLBC_OK;
}
