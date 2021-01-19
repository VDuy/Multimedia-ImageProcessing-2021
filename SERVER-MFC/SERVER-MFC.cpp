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
bool isCreated = false;
float sumKernel = -1;
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
void createKernelNormalize();
void createKernelGaussian(float);

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
			CSocket listen, connect;
			listen.Create(8963);
			listen.Listen(5);
			char buf[256];
			int ret = 0;
			cout << "\nDanh sach command: RED, GREEN, BLUE, BW, TACK, BLUR, NORMALIZE, REVERT, CMYK, INFO, SHUTDOWN \n";
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
	tm *now = std::localtime(&t);

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
	SOCKET client = *(SOCKET *)param;
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
			ret = recv(client, (char *)sockData + i, imgSize, 0);
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
	else if (command == "TACK") {
		command = "default";
		imshow("TACK 1 frame", input);
		
		imwrite("img.jpg", input);
		output = input;
	}
	else if (command == "INFO") {
		for (int i = 0; i < input.rows; i++)
		{
			for (int j = 0; j < input.cols; j++)
			{
				cout << input.at<Vec3b>(i, j);
			}
		}
		command = "default";
		output = input;
	}
	// 
	else if (command == "BLUR" || command == "NORMALIZE") {
		if (command == "BLUR") {
			createKernelGaussian(0.84);
		}
		else if (command == "NORMALIZE") {
			createKernelNormalize();
		}

		input.copyTo(output);
		for (int i = 0; i < 10; i++)
		{
			Mat data_padding;
			int pad = n / 2;
			copyMakeBorder(output, data_padding, pad, pad, pad, pad, BORDER_CONSTANT, Scalar(0, 0, 0));
			
			int height = output.rows;
			int width = output.cols;
			cout << "wh: " << width << ":" << height << endl;
			for (int r = pad; r < height - pad; ++r) {
				for (int c = pad; c < width - pad; ++c) {
					float sum_c1 = 0.0f;
					float sum_c2 = 0.0f;
					float sum_c3 = 0.0f;
					for (int kr = -pad; kr <= pad; kr++)
					{
						for (int kc = -pad; kc <= pad; kc++)
						{
							//if (r + c < 50) printf("%i : ", data_padding.at<Vec3b>(r + kr, c + kc)[0]);
							sum_c1 += data_padding.at<Vec3b>(r + kr, c + kc)[0] * kernel[kr + pad][kc + pad];
							sum_c2 += data_padding.at<Vec3b>(r + kr, c + kc)[1] * kernel[kr + pad][kc + pad];
							sum_c3 += data_padding.at<Vec3b>(r + kr, c + kc)[2] * kernel[kr + pad][kc + pad];
						}
						//if (r + c < 50)cout << endl;
					}
					output.at<Vec3b>(r - pad, c - pad)[0] = (int)sum_c1 / sumKernel; // blue channel 
					output.at<Vec3b>(r - pad, c - pad)[1] = (int)sum_c2 / sumKernel; // green channel
					output.at<Vec3b>(r - pad, c - pad)[2] = (int)sum_c3 / sumKernel; // red channel
					//if (r + c < 50) printf("convolution: %i \n", output.at<Vec3b>(r, c)[0]);
				}
			}
			/*cout << "size after blur: " << output.rows << " : " << output.cols;
			char* s = "blur ";
			s[6] = '0' + i;
			imshow(s, output);
			waitKey(5);*/
		}
		imshow("after blur: ", output);
		command = "TACK";
	}
	//
	else if (command == "REVERT") {
		for (int i = 0; i < input.rows; i++)
		{
			for (int j = 0; j < input.cols; j++)
			{
				output.at<Vec3b>(i, j)[0] = 255 - input.at<Vec3b>(i, j)[0];
				output.at<Vec3b>(i, j)[1] = 255 - input.at<Vec3b>(i, j)[1];
				output.at<Vec3b>(i, j)[2] = 255 - input.at<Vec3b>(i, j)[2];
			}
		}
	}
	//
	else if (command == "CMYK") {
		Mat cyan = Mat::zeros(input.rows, input.cols, CV_8UC3);
		Mat magenta = Mat::zeros(input.rows, input.cols, CV_8UC3);
		Mat yellow = Mat::zeros(input.rows, input.cols, CV_8UC3);
		Mat key = Mat::zeros(input.rows, input.cols, CV_8UC1);
		float r, g, b, k;
		for (int i = 0; i < input.rows; i++)
		{
			for (int j = 0; j < input.cols; j++)
			{
				// chuyen ve float 
				r = input.at<Vec3b>(i, j)[2] / 255.;
				g = input.at<Vec3b>(i, j)[1] / 255.;
				b = input.at<Vec3b>(i, j)[0] / 255.;
				
				k = std::min(std::min(1 - r, 1 - g), 1 - b);

				cyan.at<Vec3b>(i, j)[0] = (1 - r - k) / (1 - k) * 255.;
				cyan.at<Vec3b>(i, j)[1] = (1 - r - k) / (1 - k) * 255.;

				magenta.at<Vec3b>(i, j)[0] = (1 - g - k) / (1 - k) * 255.;
				magenta.at<Vec3b>(i, j)[2] = (1 - g - k) / (1 - k) * 255.;

				yellow.at<Vec3b>(i, j)[2] = (1 - b - k) / (1 - k) * 255.;
				yellow.at<Vec3b>(i, j)[1] = (1 - b - k) / (1 - k) * 255.;

				key.at<uchar>(i, j) = k * 255.;
			}
		}
		imshow("CMYK - C", cyan);
		imshow("CMYK - M", magenta);
		imshow("CMYK - Y", yellow);
		imshow("CMYK - K", key);
		output = input;
		command = "other";
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


void createKernelNormalize() {
	printf("\ncreate kernel 11x11 normalize ::: \n");
	sumKernel = 0;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			kernel[i][j] = 1.0 / n*n*1.0;
			printf("%7.7f ; ", kernel[i][j]);
		}
		cout << endl;
	}
	sumKernel = 1;
}

void createKernelGaussian(float sigma)
{
	double r, s = 2.0 * sigma * sigma;
	double M_PI = 3.14159265359;
	double sum = 0.0;

	printf("\ncreate kernel 5x5 gaussian ::: \n");
	// ma tran gaussian 5x5 
	int halfN = n / 2;
	for (int x = -halfN; x <= halfN; x++) {
		for (int y = -halfN; y <= halfN; y++) {
			r = sqrt(x * x + y * y);
			kernel[x + halfN][y + halfN] = (exp(-(r * r) / s)) / (M_PI * s);
			sum += kernel[x + halfN][y + halfN];
		}
	}

	// 
	sumKernel = 0;
	for (int i = 0; i < n; ++i) {
		cout << endl;
		for (int j = 0; j < n; ++j)
		{
			kernel[i][j] /= sum;
			printf("%7.7f ; ", kernel[i][j]);
			sumKernel += kernel[i][j];
		}
	}

}

Mat convolution(Mat input) {
	Mat output = Mat::zeros(input.rows, input.cols, 16);
	Mat data_padding;
	int pad = 2;
	copyMakeBorder(input, data_padding, pad, pad, pad, pad, BORDER_CONSTANT, Scalar(0, 0, 0));
	//imshow("adding border", data_padding);
	int height = input.rows;
	int width = input.cols;
	cout << "wh: " << width << ":" << height << endl;
	for (int r = pad; r < height - pad; ++r) {
		for (int c = pad; c < width - pad; ++c) {
			float sum_c1 = 0.0f;
			float sum_c2 = 0.0f;
			float sum_c3 = 0.0f;
			for (int kr = -pad; kr <= pad; kr++)
			{
				for (int kc = -pad; kc <= pad; kc++)
				{
					if (r + c < 50) printf("%i : ", data_padding.at<Vec3b>(r + kr, c + kc)[0]);
					sum_c1 += data_padding.at<Vec3b>(r + kr, c + kc)[0] * kernel[kr + pad][kc + pad];
					sum_c2 += data_padding.at<Vec3b>(r + kr, c + kc)[1] * kernel[kr + pad][kc + pad];
					sum_c3 += data_padding.at<Vec3b>(r + kr, c + kc)[2] * kernel[kr + pad][kc + pad];
				}
				if (r + c < 50)cout << endl;
			}
			output.at<Vec3b>(r - pad, c - pad)[0] = (int)sum_c1 / sumKernel; // blue channel 
			output.at<Vec3b>(r - pad, c - pad)[1] = (int)sum_c2 / sumKernel; // green channel
			output.at<Vec3b>(r - pad, c - pad)[2] = (int)sum_c3 / sumKernel; // red channel
			if (r + c < 50) printf("convolution: %i \n", output.at<Vec3b>(r, c)[0]);
		}
	}
	return output;
}