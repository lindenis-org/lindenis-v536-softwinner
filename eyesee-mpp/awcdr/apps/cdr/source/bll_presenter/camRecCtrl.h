#ifndef __CAM_REC_CTRL_H__
#define __CAM_REC_CTRL_H__

#include "common/subject.h"
#include <utils/Mutex.h>
#include <pthread.h>
#include "common_type.h"

using namespace EyeseeLinux;

class CamRecCtrl
	: public ISubjectWrap(CamRecCtrl)
{
	public:
		CamRecCtrl();
		~CamRecCtrl();

	public:
		static CamRecCtrl* GetInstance(void);
		int SetCamMap(CameraMap p_CamMap);
		int setRecoderMap(CamRecMap p_CamRecMap);

		Camera *GetCamera(int p_CamId);
		Recorder *GetRecorder(int p_CamId, int p_recorder_id);

		int StopAllRecord();
	private:
		CameraMap m_CameraMap;
		CamRecMap m_CamRecMap;
};

#endif

