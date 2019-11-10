// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO:  在此处引用程序需要的其他头文件
#define WIN32_LEAN_AND_MEAN //去掉windows.h中的WinSock.h，避免其与WinSock2.h的冲突 
#include <windows.h>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib")