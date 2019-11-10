// testServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include "TestServer.h"
#include "TestAttemperServer.h"

int _tmain(int argc, _TCHAR* argv[])
{
	//CTestServer::Run(argc, argv);
	CTestAttemperServer::Run(argc, argv);

    return 0;
}

