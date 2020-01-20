#include "OtaUpdate.h"
#include "update4GModule.h"
#include "ota_private.h"
#include <string.h>
#include "xml_parser.h"
#include <sstream>
#include"app_log.h"
#include"ota_common.h"

using namespace std;

HWND     hMainWnd;
static gal_pixel gp_linec = 0xFF2772DB;
static gal_pixel gp_line_bottom = 0xFFD8D8D8;
static gal_pixel gp_line_clear = 0xE5192547;
char rm_buf[256]={0};

static int prompt_w = 460;
static int prompt_h = 232;
static int y_pos;
static int x_pos;
#define OTA_TIMER  1001
#define COST_TIME  120
#define SCREENT_W 640
#define SCREENT_H 360
#define ProgressBar_line_x 30
#define ProgressBar_line_y 122+72
#define ProgressBar_line_w prompt_w-ProgressBar_line_x*2 //400

char buf[64]={0};
LOGFONT *mfont;
XmlParser * m_XmlParser = NULL;
bool m_over = false;//update over
bool m_over_s = true;//update over success
bool m_draw_full_flag = true;
bool m_ota_system_version = false;
static unsigned int ret_val = 0;
typedef struct __icon_path_
{
   const char * icon_path;
}Icon_path;
static Icon_path IconPath[]=
{
	{"/usr/share/minigui/res/images/tips_finish.png"},
	{"/usr/share/minigui/res/images/tips_fail.png"}
};

typedef struct __string_info
{
    const char * item_name;
    string str;
}stringinfo;
static stringinfo stringinfo_[]=
{
	{"ml_ota_install_packet"},
	{"ml_ota_install_failed"},
	{"ml_ota_install_finsh"},
	{"ml_ota_install_notice"},
	{"ml_ota_instal_check_md5_fail"},
	{"ml_ota_instal_found_img_fail"}
};
static void getIconRect(RECT * rect)
{
	rect->left = 198;
	rect->right = rect->left+64;
	rect->top = 28;
	rect->bottom = rect->top+64;
}

static void getTopStringAllRect(RECT * rect)
{
	rect->left = 0;
	rect->right = rect->left+prompt_w;
	rect->top = 48;
	rect->bottom = rect->top+42;
}
static void getTopSecondStringRect(RECT * rect)
{
	rect->left = 0;
	rect->right = rect->left+prompt_w;
	rect->top = 48+48;
	rect->bottom = rect->top+72;
}


static void getAllWindowRect(RECT * rect)
{
	rect->left = 0;
	rect->right = rect->left+prompt_w;
	rect->top = 0;
	rect->bottom = rect->top+prompt_h;
}

void drawProgress(HDC hwnd,int pos_,bool flag)
{
    //db_warn("db_warn--->  pos_ = %d",pos_);
    if((pos_) < ProgressBar_line_w)
    {
        RECT _rect_;
        sprintf(buf,"%s ( %2d%s",stringinfo_[0].str.c_str(),(pos_),"% )");
        getTopStringAllRect(&_rect_);
        DrawText( hwnd, buf, -1, &_rect_,DT_CENTER);
        //second str
        RECT _rect_s;
        getTopSecondStringRect(&_rect_s);
        DrawText( hwnd, stringinfo_[3].str.c_str(), -1, &_rect_s,DT_CENTER);

        SetPenWidth (hwnd, 4);
        SetPenColor(hwnd, gp_line_bottom);
        LineEx (hwnd, ProgressBar_line_x, ProgressBar_line_y, ProgressBar_line_x+ProgressBar_line_w, ProgressBar_line_y);
        SetPenColor(hwnd, gp_linec);
        LineEx (hwnd, ProgressBar_line_x, ProgressBar_line_y, ProgressBar_line_x+(pos_)*4, ProgressBar_line_y);
    }
    else
    {

	    //ready to update the tip string
        RECT rect_s,rect_icon;
        getTopSecondStringRect(&rect_s);
	    getIconRect(&rect_icon);
        if(flag )
		{
			if(m_draw_full_flag)
			{
				//draw full progress
				 RECT _rect_;
				 sprintf(buf,"%s ( %d%s",stringinfo_[0].str.c_str(),100,"% )");
				 getTopStringAllRect(&_rect_);
				 DrawText( hwnd, buf, -1, &_rect_,DT_CENTER);
				//second str
				RECT _rect_s;
				getTopSecondStringRect(&_rect_s);
				DrawText( hwnd, stringinfo_[3].str.c_str(), -1, &_rect_s,DT_CENTER);

				SetPenWidth (hwnd, 4);
				SetPenColor(hwnd, gp_line_bottom);
				LineEx (hwnd, ProgressBar_line_x, ProgressBar_line_y, ProgressBar_line_x+ProgressBar_line_w, ProgressBar_line_y);
				SetPenColor(hwnd, gp_linec);
				LineEx (hwnd, ProgressBar_line_x, ProgressBar_line_y, ProgressBar_line_x+ProgressBar_line_w, ProgressBar_line_y);
				return ;
			}

			    BITMAP *data = (BITMAP*)malloc(sizeof(BITMAP));
				int ret = LoadBitmapFromFile(hwnd, data, IconPath[0].icon_path);//success
				if (ret != 0)
				{
					free(data);
				 data = NULL;
				}
                else
				    FillBoxWithBitmap (hwnd, rect_icon.left, rect_icon.top,rect_icon.right-rect_icon.left,rect_icon.bottom-rect_icon.top,data);
					
				DrawText( hwnd, stringinfo_[2].str.c_str(), -1, &rect_s,DT_CENTER);
        }
		else 
	    {//failed
                BITMAP *data = (BITMAP*)malloc(sizeof(BITMAP));
    		  int ret = LoadBitmapFromFile(hwnd, data, IconPath[1].icon_path);//failed
	 	    if (ret != 0)
	 		{
	 	        free(data);
	 		    data = NULL;
	 	    }
           else
                FillBoxWithBitmap (hwnd, rect_icon.left, rect_icon.top,rect_icon.right-rect_icon.left,rect_icon.bottom-rect_icon.top,data);
	 	   
				if(ret_val == OTA_UPDATE_FAIL)
	     				DrawText( hwnd, stringinfo_[1].str.c_str(), -1, &rect_s,DT_CENTER);
				else if(ret_val == OTA_MD5_CHECK_FAIL)
					DrawText( hwnd, stringinfo_[4].str.c_str(), -1, &rect_s,DT_CENTER);
				else if(ret_val == OTA_FOUND_IMG_FAIL)
					DrawText( hwnd, stringinfo_[5].str.c_str(), -1, &rect_s,DT_CENTER);
        }

    	SetPenColor(hwnd, gp_line_clear);
    	LineEx (hwnd, ProgressBar_line_x, ProgressBar_line_y, ProgressBar_line_x+ProgressBar_line_w, ProgressBar_line_y);
    }
}

static long int OTAPro(HWND hWnd, unsigned int  message, WPARAM wParam, LPARAM lParam)
{
    static int pos=0;
    switch(message)
    {
        case MSG_CREATE:
            {
		        pos=0;
		        SetTimer(hWnd,OTA_TIMER,100);//1s
            }
        break;
       case MSG_PAINT:
		{
		    HDC hdc = 0;
    		hdc = BeginPaint(hWnd);
			SetBkMode(hdc, BM_TRANSPARENT);
            SelectFont(hdc, mfont);
		    SetTextColor(hdc, 0xffffffff);
			RECT _rect_s;
		    if(!m_over)
            {
                if(m_ota_system_version) 
                    drawProgress(hdc, pos/2, false);
                else
                    drawProgress(hdc, pos, false);
		    }
            else
            {
    			if(m_over_s)//鎴愬姛鍗囩骇
                {
    				if(m_draw_full_flag == false)
                    {
					    db_msg("waited for 2 s");
					    sleep(2);
    			    }
    				drawProgress(hdc, ProgressBar_line_w, true);
    			}
                else //鍗囩骇澶辫触
                {
    				drawProgress(hdc, ProgressBar_line_w, false);
    		    }
		    }
    		EndPaint(hWnd, hdc);
    		if(m_over)
             {
    				if(m_draw_full_flag)
    				{
    					db_msg("m_draw_full_flag need to set false\n");
    					m_draw_full_flag = false;
    				}
                    else
                    {
    					db_msg("ready to reboot   rm_buf = %s\n",rm_buf);
                        KillTimer(hWnd,OTA_TIMER);
    					system(rm_buf);
    					sleep(3);
    					system("reboot -f");
    			    }
    		}
    		return 0;
        }
        case MSG_TIMER:
            if(wParam == OTA_TIMER)
            {
            	if(m_ota_system_version) 
                	pos+=1;
		        else 
			        pos+=3; //4G update
                

		  RECT rect_str;
		  getAllWindowRect(&rect_str);//reflesh all rect
		::InvalidateRect(hWnd, &rect_str, TRUE);
            }
            break;

        case MSG_UPDATE_OVER:
        {
            db_msg("******MSG_UPDATE_OVER****wParam:%u   *****\n",wParam);
	            ret_val = wParam;
	            m_over = true;
            if(wParam  == OTA_UPDATE_SUCCESS){
		        m_over_s = true;
            }
            else
            {
                m_over_s = false;
            }
	     RECT rect_all;
            getAllWindowRect(&rect_all);
	     ::InvalidateRect(hWnd, &rect_all, TRUE);
        }
        break;

        case MSG_CLOSE:
            KillTimer(hWnd,OTA_TIMER);
            DestroyMainWindow(hWnd);
            PostQuitMessage(hWnd);
            break;

        default:
            return DefaultMainWinProc(hWnd,message,wParam,lParam);
    }

    return 0;
}


static void getScreenInfo(int *w, int *h)
{
	//总的提示框的大小
    *w = 460;
    *h = 232;
}


int initWindow()
{
	

    getScreenInfo(&prompt_w, &prompt_h);
	
	y_pos =(SCREENT_H - prompt_h)/2+60;//+60 是因为这个屏幕是480 高度，实际显示是360，上下都没有了60，所以y要向下偏移60
	x_pos = (SCREENT_W - prompt_w)/2;
    MAINWINCREATE CreateInfo;
    CreateInfo.dwStyle=WS_VISIBLE ;
    CreateInfo.dwExStyle = WS_EX_NONE | WS_EX_TROUNDCNS | WS_EX_BROUNDCNS|WS_EX_TRANSPARENT|WS_EX_TOPMOST;
    CreateInfo.spCaption = "OTA Update";
    CreateInfo.hMenu = 0;
   // CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = OTAPro;
    CreateInfo.lx = x_pos;
    CreateInfo.ty = y_pos;
    CreateInfo.rx = x_pos+prompt_w;
    CreateInfo.by = y_pos+prompt_h;
    CreateInfo.iBkColor = gp_line_clear;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

   mfont = CreateLogFont("sxf", "arialuni", "UTF-8",
	  FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
	  FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 24, 0);

    hMainWnd = CreateMainWindow(&CreateInfo);
    if(hMainWnd==HWND_INVALID)
        return 0;
   SetWindowFont(hMainWnd,mfont);
    m_XmlParser = new XmlParser();
    m_XmlParser->start();
    m_XmlParser->analysis();
    m_XmlParser->GetString(std::string(stringinfo_[0].item_name),stringinfo_[0].str);
    m_XmlParser->GetString(std::string(stringinfo_[1].item_name),stringinfo_[1].str);
    m_XmlParser->GetString(std::string(stringinfo_[2].item_name),stringinfo_[2].str);
    m_XmlParser->GetString(std::string(stringinfo_[3].item_name),stringinfo_[3].str);
    m_XmlParser->GetString(std::string(stringinfo_[4].item_name),stringinfo_[4].str);
    m_XmlParser->GetString(std::string(stringinfo_[5].item_name),stringinfo_[5].str);
    //db_msg("debug_zhb------stringinfo_[0].str= %s stringinfo_[1].str = %s  ,stringinfo_[2].str = %s\n",stringinfo_[0].str.c_str(),stringinfo_[1].str.c_str(),stringinfo_[2].str.c_str());
	//db_msg("debug_zhb------stringinfo_[3].str= %s stringinfo_[4].str = %s  stringinfo_[5].str = %s\n",stringinfo_[3].str.c_str(),stringinfo_[4].str.c_str(),stringinfo_[5].str.c_str());
    return 1;
}


int MiniGUIMain(int argc, const char* argv[])
{
    MSG msg;
    if(!initWindow()){
        fprintf(stderr,"create ota window failed");
    }
    ShowWindow(hMainWnd,SW_SHOWNORMAL);

 	switch(argc)
	{
		case 2:
		{
			db_warn("update the system version");
			m_ota_system_version = true;
			OtaUpdate *ota = new OtaUpdate(hMainWnd);
			char * version_path = (char*)argv[1];
			sprintf(rm_buf,"rm %s%s -rf",version_path,"*.img");
			ota->startUpdatePkg(version_path);
		}break;
		case 3:
		{
			db_warn("update the 4G modlue version");
			char *mode_select = (char*)argv[2];
			m_ota_system_version = false;
			Update4GModule *u4m = new Update4GModule(hMainWnd);
			char * version_path = (char*)argv[1];
			u4m->m_path_version = version_path;
		       sprintf(rm_buf,"rm %s%s -rf",u4m->m_path_version.c_str(),"*.bin");
		       u4m->startUpdate4GModule();
		}break;
	}

    while(GetMessage(&msg, hMainWnd)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    MainWindowThreadCleanup(hMainWnd);
    return 0;
}
