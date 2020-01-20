#pragma once

#include "common/message.h"
#include "common/subject.h"
#include "common/singleton.h"
#include "common/common_inc.h"

namespace EyeseeLinux {
class DialogStatusManager
    : public Singleton<DialogStatusManager>
    , public ISubjectWrap(DialogStatusManager)
{
    friend class Singleton<DialogStatusManager>;
    public:
	DialogStatusManager();
	~DialogStatusManager();
	void setMDialogEventFinish(bool flag){m_dialog_event_finish = flag;}
	bool getMDialogEventFinish(){return m_dialog_event_finish;} 
private:
	bool m_dialog_event_finish;
	
}; 

}
