#include "camRecCtrl.h"
#include <stdio.h>
#include <stdlib.h>
#include "common/app_log.h"
#include "device_model/system/led.h"


#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "camRecCtrl.cpp"
#endif

using namespace EyeseeLinux;
using namespace std;

static Mutex camRecCtr_mutex;
static CamRecCtrl *m_CamRecCtrl = NULL;

CamRecCtrl::CamRecCtrl()
{
	m_CameraMap.clear();
	m_CamRecMap.clear();
}

CamRecCtrl::~CamRecCtrl()
{

}

CamRecCtrl* CamRecCtrl::GetInstance(void)
{
	Mutex::Autolock _l(camRecCtr_mutex);
	if( m_CamRecCtrl != NULL )
		return m_CamRecCtrl;

	m_CamRecCtrl = new CamRecCtrl();
	if( m_CamRecCtrl != NULL)
		return m_CamRecCtrl;
	else
	{
		db_error("new CamRecCtrl failed\n");
		return NULL;
	}
}

int CamRecCtrl::SetCamMap(CameraMap p_CamMap)
{
	m_CameraMap = p_CamMap;

	return 0;
}

int CamRecCtrl::setRecoderMap(CamRecMap p_CamRecMap)
{
	m_CamRecMap = p_CamRecMap;

	return 0;
}

Camera *CamRecCtrl::GetCamera(int p_CamId)
{
	map<int, Camera*>::iterator iter;
	for(iter = m_CameraMap.begin(); iter != m_CameraMap.end(); iter++)
	{
		if( iter->first != p_CamId )
			continue;

		return iter->second;
	}

	return NULL;
}

Recorder *CamRecCtrl::GetRecorder(int p_CamId, int p_recorder_id)
{
	map<int, std::map<int, Recorder *>>::iterator iter;
	for(iter = m_CamRecMap.begin(); iter != m_CamRecMap.end(); iter++ )
	{
		if( iter->first != p_CamId )
			continue;

		std::map<int, Recorder*>::iterator rec_iter;
		for (rec_iter = m_CamRecMap[p_CamId].begin();rec_iter != m_CamRecMap[p_CamId].end(); rec_iter++)
		{
			if( rec_iter->first != p_recorder_id )
				continue;			

			return rec_iter->second;
		}
	}

	return NULL;
}

int CamRecCtrl::StopAllRecord()
{
	Recorder *rec = GetRecorder(1, 3);
#if 0
	if(rec != NULL )
		rec->StopRecord();
#endif
	rec = GetRecorder(1, 2);
	if(rec != NULL)
		rec->StopRecord();
#if 0
	rec = GetRecorder(0, 1);
	if(rec != NULL )
		rec->StopRecord();
#endif

	rec = GetRecorder(0, 0);
	if(rec != NULL )
		rec->StopRecord();
    
    Camera *cam = NULL;

	//add by jason_yin
	//LedControl::get()->TurnOffAllLed();
    cam = GetCamera(1);
    if(cam != NULL)
    {
        cam->releasePictureEncoder(3);
    }
    cam = GetCamera(0);
    if(cam != NULL)
    {
        cam->releasePictureEncoder(1);
    }    
	return 0;
}
