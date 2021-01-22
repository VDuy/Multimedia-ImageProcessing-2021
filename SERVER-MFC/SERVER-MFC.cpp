
// SERVER-MFC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "SERVER-MFC.h"
#include <afxsock.h>
#include "opencv2\opencv.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <cmath>
#include <iomanip>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;
using namespace cv;

Scalar color = Scalar(0, 0, 0);
Scalar colorBLUE = Scalar(225, 0, 0);
Scalar colorGREEN = Scalar(0, 255, 0);
Scalar colorRED = Scalar(0, 0, 255);

RNG rng(12345);
Mat processImage(Mat input);

const int n = 5;

int delay = 0;
unsigned __int64 timestamp;

string command = "DEFAULT";

CWinApp theApp;

// declare functions
void showIMG(cv::String, cv::Mat);
DWORD WINAPI CommandThread(LPVOID);
DWORD WINAPI ClientThread(LPVOID);

Mat convolution(Mat);

// declare variables
int cols[64];
int rows[64];
int imgSize[64];
CSocket clients[64];
SOCKET listSocket[64];
int num = 0;

float kernel[n][n];

int main()
{
	printf("Starting server ...\n");
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
			WSADATA wsa;
			AfxSocketInit(&wsa);
			AfxSocketInit(&wsa);
			CSocket listen, connect;
			listen.Create(8963);
			listen.Listen(5);
			char buf[256];
			int ret = 0;

			CreateThread(0, 0, CommandThread, NULL, 0, 0);

			cout << "Connecting...";
			cout << "\nDanh sach command: RED, GREEN, BLUE, BW \n";
			while (true)
			{
				listen.Accept(clients[num]);
				listSocket[num] = clients[num].m_hSocket;
				ret = clients[num].Receive(buf, sizeof(buf), 0);
				buf[ret] = 0;
				cout << "params:" << buf << endl;
				int c, r;
				sscanf(buf, "%i %i", &c, &r);
				cols[num] = c;
				rows[num] = r;
				CreateThread(0, 0, ClientThread, &listSocket[num], 0, 0);
				num++;
			}
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

void showIMG(cv::String winName, Mat img)
{
	time_t t = time(0);
	tm* now = std::localtime(&t);

	char buf[256];
	sprintf(buf, "Time: %i/%i/%i %i:%i:%i",
		now->tm_year + 1900,
		now->tm_mon + 1, now->tm_mday,
		now->tm_hour, now->tm_min,
		now->tm_sec);
	cv::putText(img, buf,
		Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, colorRED, 1, cv::LINE_AA);
	sprintf(buf, "Reso: %ix%i, Delay: %ims", img.cols, img.rows, delay);

	//resolution.
	cv::putText(img, buf,
		Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, colorBLUE, 1, cv::LINE_AA);
	unsigned __int64 n = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	delay = n - timestamp;
	timestamp = n;
	imshow(winName, img);
}

DWORD WINAPI CommandThread(LPVOID param)
{
	while (true)
	{
		cout << "Input command: ";
		cin >> command;
	}
}

DWORD WINAPI ClientThread(LPVOID param)
{
	SOCKET client = *(SOCKET*)param;
	int i = 0;
	for (; i < 64; i++)
		if (client == clients[i])
			break;
	cout << "Client " << client << " connected\n";

	int ret;
	char title_original[64];
	sprintf(title_original, "Original - Client %i", client);
	char title_process[64];
	sprintf(title_process, "Image Processing - Client %i", client);

	Mat img = Mat::zeros(rows[i], cols[i], 16); // create frame for client
	int imgSize = img.total() * img.elemSize(); // get frame size, also is the size of buffer received
	uchar sockData[1000000];

	while (true)
	{
		ret = 0;
		//receive until get enough data for an image
		for (int i = 0; i < imgSize; i += ret)
		{
			ret = recv(client, (char*)sockData + i, imgSize, 0);
			if (ret < 0)
				break;
		}

		// decode image
		int ptr = 0;
		for (int i = 0; i < img.rows; i++)
		{
			for (int j = 0; j < img.cols; j++)
			{
				img.at<Vec3b>(i, j) = Vec3b(sockData[ptr + 0], sockData[ptr + 1], sockData[ptr + 2]);
				ptr = ptr + 3;
			}
		}

		// show image
		showIMG(title_process, processImage(img));
		waitKey(5);
	}
	return true;
}

// new

Mat processImage(Mat input) {

	Mat output = Mat::zeros(input.rows, input.cols, CV_8UC3);
	//
	if (command == "BW") {
		for (int c = 0; c < input.cols; c++)
		{
			for (int r = 0; r < input.rows; r++)
			{
				int avg = input.at<Vec3b>(r, c)[0] + input.at<Vec3b>(r, c)[1] + input.at<Vec3b>(r, c)[2];
				avg = avg / 3;
				output.at<Vec3b>(r, c)[0] = avg;
				output.at<Vec3b>(r, c)[1] = avg;
				output.at<Vec3b>(r, c)[2] = avg;
			}
		}
	}
	else if (command == "RED") {
		for (int c = 0; c < input.cols; c++)
		{
			for (int r = 0; r < input.rows; r++)
			{
				output.at<Vec3b>(r, c)[0] = 0;
				output.at<Vec3b>(r, c)[1] = 0;
				output.at<Vec3b>(r, c)[2] = input.at<Vec3b>(r, c)[2];
			}
		}
	}
	else if (command == "GREEN") {
		for (int c = 0; c < input.cols; c++)
		{
			for (int r = 0; r < input.rows; r++)
			{
				output.at<Vec3b>(r, c)[0] = 0;
				output.at<Vec3b>(r, c)[1] = input.at<Vec3b>(r, c)[1];
				output.at<Vec3b>(r, c)[2] = 0;
			}
		}
	}
	else if (command == "BLUE") {
		for (int c = 0; c < input.cols; c++)
		{
			for (int r = 0; r < input.rows; r++)
			{
				output.at<Vec3b>(r, c)[0] = input.at<Vec3b>(r, c)[0];
				output.at<Vec3b>(r, c)[1] = 0;
				output.at<Vec3b>(r, c)[2] = 0;
			}
		}
	}

	else if (command == "RGB") {
		Mat red = Mat::zeros(input.rows, input.cols, CV_8UC3);
		Mat green = Mat::zeros(input.rows, input.cols, CV_8UC3);
		Mat blue = Mat::zeros(input.rows, input.cols, CV_8UC3);
		for (int i = 0; i < input.rows; i++)
		{
			for (int j = 0; j < input.cols; j++)
			{
				red.at<Vec3b>(i, j)[2] = input.at<Vec3b>(i, j)[2];
				green.at<Vec3b>(i, j)[1] = input.at<Vec3b>(i, j)[1];
				blue.at<Vec3b>(i, j)[0] = input.at<Vec3b>(i, j)[0];
			}
		}
		imshow("RGB - R", red);
		imshow("RGB - G", green);
		imshow("RGB - B", blue);
		output = input;
		command = "other";
	}
	else {
		output = input;
	}
	return output;
}
