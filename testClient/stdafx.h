// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
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