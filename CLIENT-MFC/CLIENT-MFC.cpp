// CLIENT-MFC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CLIENT-MFC.h"
#include <afxsock.h>
#include "opencv2\opencv.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CWinApp theApp;

using namespace std;
using namespace cv;

int main()
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			wprintf(L"Fatal Error: MFC initialization failed\n");
			nRetCode = 1;
		}
		else
		{

			VideoCapture cap;

		
			cap = VideoCapture(0);
			Mat frame;
			WSADATA wsa;
			AfxSocketInit(&wsa);

			CSocket client;
			client.Create();
			system("pause");
			client.Connect(L"127.0.0.1", 8963);
			char buf[256];
			cap >> frame;

			// get camera information
			int imgSize = frame.total()*frame.elemSize();
			int rows = frame.rows;
			int cols = frame.cols;

			

			sprintf(buf, "%i %i", cols, rows);
			cout << ":" << buf << ":";
			client.Send(buf, strlen(buf), 0);
			int ret;

			while (true) {
				cap >> frame;
				imshow("Camera", frame);

				// encode image 
				frame = (frame.reshape(0, 1));

				// send data 
				ret = client.Send(frame.data, imgSize, 0);
				if (ret <= 0) break;

				// delay 5ms
				waitKey(5);

			}
			cout << "Disconnected\n";
			system("pause");

		}
	}
	else
	{
		// TODO: change error code to suit your needs
		wprintf(L"Fatal Error: GetModuleHandle failed\n");
		nRetCode = 1;
	}

	return nRetCode;
}
