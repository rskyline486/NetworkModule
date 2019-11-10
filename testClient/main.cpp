// testClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include "TestClient.h"
#include "TestAttemperClient.h"

int _tmain(int argc, _TCHAR* argv[])
{
	//CTestClient::Run(argc, argv);
	CTestAttemperClient::Run(argc, argv);

	return 0;
}

