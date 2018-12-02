
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "dehazing.h"
#include <time.h>
#include <conio.h>
#include <omp.h>
#include <Windows.h>
#include <thread>
#include <queue>
#include <mutex>
#include <vlc\libvlc.h>
#include <vlc\libvlc_media.h>
#include <vlc\libvlc_media_player.h>

using namespace std;

#pragma comment(lib, "libvlc.lib")
#pragma comment(lib, "libvlccore.lib")

#define VIDEO_PATH "E:\\FFOutput\\20181122154719.mp4"
#define OUTPUT_VIDEO_PATH "E:\\save_video\\"

string get_local_time(){
	time_t nowTime = time(NULL);
	tm t;
	localtime_s(&t,&nowTime);
	string year, day, month, hour, min, sec;

	year = to_string(t.tm_year + 1900);

	if(t.tm_mon < 10)
		month = "0" + to_string(t.tm_mon + 1);
	else
		month = to_string(t.tm_mon + 1);
	if(t.tm_mday < 10)
		day = "0" + to_string(t.tm_mday);
	else
		day = to_string(t.tm_mday);
	if(t.tm_hour < 10)
		hour = "0" + to_string(t.tm_hour);
	else
		hour = to_string(t.tm_hour);
	if(t.tm_min < 10)
		min = "0" + to_string(t.tm_min);
	else
		min = to_string(t.tm_min);
	if(t.tm_sec < 10)
		sec = "0" + to_string(t.tm_sec);
	else
		sec = to_string(t.tm_sec);

	string namestr = OUTPUT_VIDEO_PATH + year + "." + month + "." + day + "_" + hour + "." + min + "." + sec + ".avi";
	return namestr;
}

CvSize size = cvSize(1980, 1080);
string namestr;
const char* name;
dehazing dehazingImg1 = dehazing(size.width, size.height, 10, true, false, 5.0f, 1.0f, 200);
IplImage *imOutput1 = cvCreateImage(size, IPL_DEPTH_8U, 3);
IplImage *imInput1 = cvCreateImage(size, IPL_DEPTH_8U, 3);
int nFrame1 = 0;
CvVideoWriter* wrVideo = cvCreateVideoWriter(name, CV_FOURCC('M','J','P','G'), 7, size);
int quit = 0;


struct ctx
{
	IplImage* image;
	HANDLE mutex;
	uchar* pixels;
};

void *lock(void *data, void**p_pixels)
{
	 struct ctx *ctx = (struct ctx*)data;
	 WaitForSingleObject(ctx->mutex, INFINITE);
     *p_pixels = ctx->pixels;	
	 return NULL;
	 
}

void display(void *data, void *id){
   (void) data;
   assert(id == NULL);
}

void unlock(void *data, void *id, void *const *p_pixels){
	struct ctx *ctx = (struct ctx*)data;
	/* VLC just rendered the video, but we can also render stuff */
	uchar *pixels = (uchar*)*p_pixels;
	
	cvCvtColor(ctx->image, imInput1, CV_RGBA2RGB);
	dehazingImg1.HazeRemoval(imInput1, imOutput1, nFrame1);
	cvShowImage("input", imInput1);
	cvShowImage("Output", imOutput1);

	if(nFrame1 > 300){
		cvReleaseVideoWriter(&wrVideo);
		namestr = get_local_time();
		name = namestr.c_str();
		wrVideo = cvCreateVideoWriter(name, CV_FOURCC('M', 'J', 'P', 'G'), 7, size);
		nFrame1 = 0;
	}
	cvWriteFrame(wrVideo, imOutput1);
	nFrame1 ++;
	if(char(waitKey(40) == 'q')){
		quit = 1;
	}
	ReleaseMutex(ctx->mutex);
	assert(id == NULL); /* picture identifier, not needed here */
}

queue <IplImage*> Q;
mutex mu;

DWORD WINAPI SaveVideo(LPVOID laParameter);

void thread1(CvVideoWriter* wrVideo1){
	IplImage *p;
	while(!Q.empty()){
		cout << "we are writing video!   Q's elements: " << Q.size() << endl;
		mu.lock();
		p = Q.front();
		Q.pop();
		mu.unlock();
		cvShowImage("write video", p);
		time_t start_t = clock();
		cvWriteFrame(wrVideo1, p);
		cout<< "write frame:  " << (float)(clock()-start_t)/CLOCKS_PER_SEC << "secs" <<endl;
		waitKey(10);
	}
}

void thread0(IplImage* imInput, IplImage* imOutput, CvCapture* cvSequence, dehazing* dehazingImg, int frames){
	int nFrame;
	for( nFrame = 0; nFrame < 100; nFrame++ )
	{
		//cvNamedWindow("test", 0);	
		imInput = cvQueryFrame(cvSequence);
		IplImage *p = cvCreateImage(cvSize(imOutput->width, imOutput->height),IPL_DEPTH_8U, 3);
		if(!imInput)
			break;
		cvShowImage("input",imInput);
		dehazingImg->HazeRemoval(imInput,p,nFrame);

		if(!p) break;
		cvShowImage("output",p);
		
		mu.lock();
		Q.push(p);
		mu.unlock();

		//cvShowImage("write video", Q.front());
		cout << " we are showing image!  Q's elements: " << Q.size() << endl;
		waitKey(10);
	} 
}


int dehaze_func()
{	

	CvCapture* cvSequence = cvCaptureFromFile("E://FFOutput//20181114080111_720p.avi");
	
	int nWid = (int)cvGetCaptureProperty(cvSequence,CV_CAP_PROP_FRAME_WIDTH); //atoi(argv[3]);
	int nHei = (int)cvGetCaptureProperty(cvSequence,CV_CAP_PROP_FRAME_HEIGHT); //atoi(argv[4]);

	cout << nWid << " " << nHei << endl;

	//cv::VideoWriter vwSequenceWriter("test_out.avi", 0, 25, cv::Size(nWid, nHei), true);
	//CvVideoWriter* wrVideo1 = cvCreateVideoWriter("E://wei//video_output//20181114075329_720p_20_tt_200.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25, Size(nWid, nHei));
	CvVideoWriter* wrVideo1 = cvCreateVideoWriter("E://wei//video_output//20181114080111_720p_20_tt_200_X264.avi",CV_FOURCC('X','2','6','4') , 25, Size(nWid, nHei));

	IplImage *imInput;
	IplImage *imOutput = cvCreateImage(cvSize(nWid, nHei),IPL_DEPTH_8U, 3);
	int Frames = cvGetCaptureProperty(cvSequence, CV_CAP_PROP_FRAME_COUNT);
	cout << "frames: " << Frames << endl;

	dehazing dehazingImg(nWid, nHei, 20, true, true, 10.0f, 1.0f, 200);

	
	time_t start_t;
	//cvNamedWindow("test",0);
	//cvResizeWindow("test", 1920, 1080);
	//cout << "hello!!!!!!!!!!!!     " << atoi(argv[3]) << endl;
	
	int nFrame;
	for( nFrame = 0; nFrame < Frames -10; nFrame++ )
	{
		//cvNamedWindow("test", 0);
		start_t = clock();
		imInput = cvQueryFrame(cvSequence);

		dehazingImg.HazeRemoval(imInput,imOutput,nFrame);
		if(!imOutput) break;
		cvShowImage("test",imOutput);
		
		//Q.push(imOutput);
		cvWriteFrame(wrVideo1, imOutput);
		cout<< nFrame <<" frame:  " << (float)(clock()-start_t)/CLOCKS_PER_SEC << "secs" <<endl;
		waitKey(30);
	} 
	
	
	//thread task0(thread0, imInput, imOutput,cvSequence, &dehazingImg, Frames);
	//task0.join();
	//thread task1(thread1, wrVideo1);
	//task1.join();
	//cout << "task1 has recalled" << endl;
	getch();

	cvReleaseCapture(&cvSequence); 
	cvReleaseVideoWriter(&wrVideo1);
 	cvReleaseImage(&imOutput);
	cvDestroyWindow("test");

	return 0;
}


void loadCamera(){
		// 捕捉视频文件  
	CvCapture* g_capture1 = cvCreateCameraCapture(0);//通过参数设置,
                                                                                //确定要读入的视频文件
	CvCapture* g_capture2 = cvCreateCameraCapture(0);

	//CvCapture* g_capture2 = cvCaptureFromFile("haze.avi");
 
	// 读取、显示视频文件的帧数  
	int frames1 = (int)cvGetCaptureProperty(g_capture1, CV_CAP_PROP_FRAME_COUNT);	                                                                               
	int frames2 = (int)cvGetCaptureProperty(g_capture2, CV_CAP_PROP_FRAME_COUNT);
	


	// 读取视频文件信息  
	double fps1 = (int)cvGetCaptureProperty(g_capture1, CV_CAP_PROP_FPS);//帧率

	CvSize size1 = cvSize(
		(int)cvGetCaptureProperty(g_capture1, CV_CAP_PROP_FRAME_WIDTH), //视频1流中的帧宽度
		(int)cvGetCaptureProperty(g_capture1, CV_CAP_PROP_FRAME_HEIGHT));//视频1流中的帧高度
	CvSize size2 = cvSize(
		(int)cvGetCaptureProperty(g_capture2, CV_CAP_PROP_FRAME_WIDTH),
		(int)cvGetCaptureProperty(g_capture2, CV_CAP_PROP_FRAME_HEIGHT));

	cout << "frame size: " << size1.width << " " << size1.height <<endl;

	//创建视频文件名字
	const char* name;
	string namestr;
	namestr = get_local_time();
	name = namestr.c_str();
	
	//计算视频时间

	// 创建 VideoWriter   
	CvVideoWriter* wrVideo1 = cvCreateVideoWriter("video.avi", CV_FOURCC('M', 'J', 'P', 'G'), 10, size1);
	CvVideoWriter* wrVideo2 = cvCreateVideoWriter( name, CV_FOURCC('M', 'J', 'P', 'G'), 10, size2);


	VideoWriter outputVideo;

	int frs = 0;
 
	// 开始播放并保存视频  
	IplImage* frame1;
	IplImage* frame1_dst;
	IplImage* frame2;
	IplImage* frame2_dst = cvCreateImage(size1, IPL_DEPTH_8U, 3);

	dehazing dehazingImg(size1.width, size1.height, 10, false, false, 5.0f, 1.0f, 200);

	int count = 0;
	while (true)
	{
		if(count > 300){
			cvReleaseVideoWriter(&wrVideo2);
			namestr = get_local_time();
			name = namestr.c_str();
			wrVideo2 = cvCreateVideoWriter(name, CV_FOURCC('M', 'J', 'P', 'G'), 10, size2);
			count = 0;
		}

		frame1 = cvQueryFrame(g_capture1);
		frame2 = cvQueryFrame(g_capture2);


		// 建立播放窗口  
		cvNamedWindow("Video Test 1", 0);
		cvNamedWindow("Video Test 2", 0);
		cvResizeWindow("Video Test 1",1920, 1080);
		cvResizeWindow("Video Test 2", 1920, 1080);
		// 获取、显示源文件的帧画面  
		dehazingImg.HazeRemoval(frame2, frame2_dst,count);
		
		if (!frame1) break;
		cvShowImage("Video Test 1", frame1);	
		if (!frame2) break;
		cvShowImage("Video Test 2", frame2_dst);

		
		// 保存：将当前帧写入到目标视频文件  
		cvWriteFrame(wrVideo1, frame1);
		cvWriteFrame(wrVideo2, frame2_dst);

		// 若按下 q 键，则退出程序  
 		if(char(waitKey(40) == 'q')){
			break;
		}

		count ++;
		cout << "frame: " << count << endl;
	}

	// 释放内存，关闭窗口  
	cvReleaseCapture(&g_capture1);
	cvReleaseCapture(&g_capture2);
	cvReleaseVideoWriter(&wrVideo1);
	cvReleaseVideoWriter(&wrVideo2);
	cvDestroyWindow("Video Test 1");
	cvDestroyWindow("Video Test 2");
	
}

void thread3(){
	for (int i = 0; i < 5; i ++){
		cout << "thread3 is working !" << endl;
		
	}
}

void thread4(){
	for (int i = 0; i < 5; i ++){
		cout << "thread4 is working !" << endl;
		
	}
}

bool CtrlHandler(DWORD CtrlType){
	switch(CtrlType){
	case CTRL_C_EVENT:	
		quit = true;
		return true;
	default:
		cout << "ctrl handler !!!!!!!!!!!" << endl;
		return false;
	}
}

int main(){
	//创建窗体
	
	//loadCamera();
	//dehaze_func();
	/*
	thread task01(thread3);
	thread task02(thread4);
	task01.join();
	task02.join();
	*/

	
	namestr = get_local_time();
	name = namestr.c_str();
	/*
	dehazingImg1 = dehazing(size.width, size.height, 10, true, false, 5.0f, 1.0f, 200);
	imOutput1 = cvCreateImage(size, IPL_DEPTH_8U, 3);
	imInput1 = cvCreateImage(size, IPL_DEPTH_8U, 3);*/
	wrVideo = cvCreateVideoWriter(name, CV_FOURCC('M','J','P','G'), 7, size);
	//SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, true);

	libvlc_instance_t * inst; 
	libvlc_media_player_t *mp;  
	libvlc_media_t *m;  
  
	inst = libvlc_new(0,NULL);//load the vlc engine  
  
	m = libvlc_media_new_path(inst,VIDEO_PATH);//create a new item; xxx_path  
	mp = libvlc_media_player_new_from_media(m);//create a media player playing environment  
	libvlc_media_release(m);//no need to keey the media now  

	struct ctx* context = (struct ctx*) malloc(sizeof(*context));
	context->mutex = CreateMutex(NULL, FALSE, NULL);
	context->image = cvCreateImage(cvSize(1980, 1080),IPL_DEPTH_8U, 4);
	context->pixels = (unsigned char*)context->image->imageData;

	libvlc_video_set_callbacks(mp, lock, unlock, display, context);
	libvlc_video_set_format(mp, "RV32", 1980, 1080, 1980*4);
	//libvlc_video_set_format_callbacks(mp, setup, cleanup);
	
	libvlc_media_player_play(mp);//play the media_player  
	//Sleep(100*1000);//let it play a bit  
	
	while(1){
		if(quit){
			cout << "退出程序!" << endl;
			cvReleaseVideoWriter(&wrVideo);	
			libvlc_media_player_stop(mp);//stop playing  
			libvlc_media_player_release(mp);//free the media_player  
			libvlc_release(inst);
			break;
		}
		Sleep(100);
	}

	cvReleaseImage(&imOutput1);
	cvReleaseImage(&imInput1);
	cvDestroyWindow("input");
	cvDestroyWindow("Output");

	//}

	//cvReleaseVideoWriter(&wrVideo);

	/*
	cvNamedWindow("image", CV_WINDOW_AUTOSIZE);
	libvlc_media_t* media = NULL;
	libvlc_media_player_t* mediaPlayer = NULL;
	//char const* vlc_argv[] = {"--plugin-path", "C:\\Users\\Oscar\\Documents\\libvlc\\vlc-1.1.4"};
	char const* vlc_argv[] = {"--plugin-path", "D:\\vlc-3.0.4"};
	
	libvlc_instance_t* instance = libvlc_new(2,vlc_argv);
	mediaPlayer = libvlc_media_player_new(instance);
	media = libvlc_media_new_path(instance, "E://FFOutput//20181114080111_720p.avi");

	struct ctx* context = ( struct ctx* )malloc( sizeof( *context ) );
	context->mutex = CreateMutex(NULL, FALSE,NULL);
	context->image = cvCreateImage(cvSize(1280, 720), IPL_DEPTH_8U, 4);
        context->pixels = (unsigned char *)context->image->imageData;
	
	libvlc_media_player_set_media( mediaPlayer, media);
	libvlc_video_set_callbacks(mediaPlayer, lock, unlock, display, context);
    
	libvlc_media_player_play(mediaPlayer);
*/

	return 0;
}