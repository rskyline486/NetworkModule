// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO:  在此处引用程序需要的其他头文件
#include "../include/NetworkModule.h"

#ifndef _WIN64
	#ifdef _DEBUG
		#pragma comment(lib, "../lib/NetworkModuleD.lib")
	#else
		#pragma comment(lib, "../lib/NetworkModule.lib")
	#endif // _DEBUG
#else
	#ifdef _DEBUG
		#pragma comment(lib, "../lib/x64/NetworkModuleD.lib")
	#else
		#pragma comment(lib, "../lib/x64/NetworkModule.lib")
	#endif // _DEBUG
#endif // _WIN64