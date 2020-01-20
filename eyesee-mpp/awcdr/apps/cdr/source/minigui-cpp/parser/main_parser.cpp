/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: main_parser.cpp
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#define NDEBUG

#include "parser/main_parser.h"
#include "debug/app_log.h"
#include "application.h"
#include "utils.h"

using namespace std;

#define WIDGET_BEGIN_FLAG ("<widget class")
#define PROPERTY_END_FLG ("</property>")
#define ATTRIBUTE_END_FLG ("</attribute>")
#define CTRL_NAME_FLG ("<cstring>")
#define CTRL_TEXT_FLG ("<string>")
#define RECT_BEGIN_FLG ("<rect>")
#define WIDGET_END_FLAG ("</widget>")

enum {
    TYPE_GEOMETRY = 0,
    TYPE_CAPTION,
    TYPE_TEXT,
    TYPE_TITLE,
    TYPE_END,
    TYPE_NULL,
};

static const char *g_keyword[] = {
    "geometry",
    "caption",
    "text",
    "title"
};

#define TEXT_VIEW "TextView"

static const char *g_ui_class_map[][2] = {
    {"QDialog",         "SysWindow"},
    {"QWidget",         "ViewContainer"},
    {"QListView",       "MenuItems"},
    {"QListBoxView",    "ListBoxView"},
    {"QTableView",      "ListView"},
    {"QIconView",       "IconView"},
    {"QGraphicsView",   "GraphicView"},
    {"QLabel",          "TextView"},
    {"QProgressBar",    "ProgressBar"},
    {"QCheckBox",       "SwitchButton"},
    {"QPushButton",     "Button"},
    {"QPushButtonOK",   "ButtonOK"},
    {"QPushButtonCancel", "ButtonCancel"},
};


#define NDEBUG
#undef LOG_TAG
#define LOG_TAG "MainParser"

MainParser::MainParser()
{

    unsigned int i;
    for(i=0; i<sizeof(g_keyword)/sizeof(g_keyword[0]); i++) {
        keywords_.push_back(string(g_keyword[i]));
    }
    for(i=0; i<sizeof(g_ui_class_map)/sizeof(g_ui_class_map[0]); i++) {
        ui_class_map_.insert(make_pair(string(g_ui_class_map[i][0]),
            string(g_ui_class_map[i][1])));
    }
}

MainParser::~MainParser()
{

}

string MainParser::GetResourceFile()
{
    string s = Application::GetApp()->GetAppUIPath();
    s += owner_->GetResourceName();
    s += ".ui";
    db_msg("Resource File %s", s.c_str());
    return s;
}


static bool IsWidget(const string& line)
{
    unsigned int pos = line.find(WIDGET_BEGIN_FLAG);
    return (pos != line.npos);
}

void MainParser::ParseWidget(string line)
{
    ObjectInfo obj;
    obj.level = objects_.size();
    // <widget class="QWidget" name="MediaWindow">
    // TrimString---> QWidget
    const char * ptr = TrimString(line.c_str(), "\"", obj.class_name);
    db_msg("ptr:%s ", ptr);
    db_msg("class_name:%s", obj.class_name.c_str());
    if (ptr != NULL) {
        //  name="MediaWindow">
        // TrimString---> MediaWindow
        TrimString(ptr+obj.class_name.length()+1, "\"", obj.ctrl_name);
        db_msg("ctrl_name:%s", obj.ctrl_name.c_str());
    }
    StringMap::iterator iter = ui_class_map_.find(obj.class_name);
    if (iter != ui_class_map_.end()) {
        obj.class_name = iter->second;
    }
    db_msg("class_name:%s", obj.class_name.c_str());
    obj.obj = NULL;
    obj.end = false;
    obj.parent_idx = -1;
    objects_.push_back(obj);
    db_msg("ctrl_name %s", obj.ctrl_name.c_str());
}


int MainParser::GetParentIndex()
{
    int j = objects_.size();
    while(j--) {
        db_msg("name:%s ", objects_[j].ctrl_name.c_str());
        if (objects_[j].end == false) {
            return j;
        }
    }
    return 0;
}

int MainParser::GetType(string& line)
{
    db_msg("%s", line.c_str());
    int ret = TYPE_NULL;
    StringVector::iterator it;
    if (line.find(WIDGET_END_FLAG) != string::npos) {
        return TYPE_END;
    }
    string result;
    TrimString(line.c_str(), "\"", result);
    if (result.empty()) {
        return ret;
    }

    int j=0;
    db_msg("result %s", result.c_str());
    for (it = keywords_.begin(); it != keywords_.end(); it++, j++) {
        db_msg("it %s result %s", it->c_str(), result.c_str());
        if (*it == result) {
            return j;
        }
    }

    return ret;
}

void MainParser::GetText(StringVector::iterator &it, ObjectInfo &info)
{
    while (it != lines_.end()) {
        db_msg("line %s", it->c_str());
        if ((*it).find(CTRL_TEXT_FLG) != it->npos) {
            TrimString2((*it).c_str(), ">", "<", info.text);
            break;
        }
        if ((*it).find(PROPERTY_END_FLG) != it->npos
            || ((*it).find( ATTRIBUTE_END_FLG ) != it->npos)) {
            break;
        }
        it++;
    }
}

void MainParser::GetGeometry(StringVector::iterator &it, ObjectInfo &info)
{
    string value;
    while (it != lines_.end()) {
        if ((*it).find(RECT_BEGIN_FLG) != it->npos) {
            db_msg("%s", (*it).c_str());
            it++;
            TrimString2((*it++).c_str(), ">", "<", value);
            info.x = atoi(value.c_str());
            TrimString2((*it++).c_str(), ">", "<", value);
            info.y = atoi(value.c_str());
            TrimString2((*it++).c_str(), ">", "<", value);
            info.w = atoi(value.c_str());
            TrimString2((*it).c_str(), ">", "<", value);
            info.h = atoi(value.c_str());
            break;
        }
        if ((*it).find(PROPERTY_END_FLG) != it->npos) {
            break;
        }
        it++;
    }
}

void MainParser::ParseProperty(StringVector::iterator &it)
{
    if (objects_.size() <= 0) {
        it++;
        return;
    }
    string line = *it;
    int parent_idx = GetParentIndex();  //get the next node, which is to be fill.
    db_msg("current_idx:%d ", parent_idx);
    if (parent_idx < 0) {
        it++;
        return;
    }
    ObjectInfo &info = (ObjectInfo&)(objects_[parent_idx]);
    int type = GetType(line);
    db_msg("GetText type %d", type);
    it++;
    switch(type) {
        case TYPE_CAPTION:
        case TYPE_TITLE:
        case TYPE_TEXT:
            db_msg("GetText");
            GetText(it, info);
            break;
        case TYPE_END:
            info.end = true;    //current node is filled
            info.parent_idx = GetParentIndex();
            db_msg("cur_idx:%d parent_idx:%d ctrl_name %s", parent_idx,
                info.parent_idx, info.ctrl_name.c_str());
            break;
        case TYPE_GEOMETRY:
            db_msg("geometry %s", (*it).c_str());
            GetGeometry(it, info);
            db_msg("info:%d %d %d %d ", info.x, info.y, info.w, info.h);
            break;
        default:

            break;
    }


}

static void dump_objects(ObjectVector &objects)
{
    ObjectVector::iterator it;
    int i = 0;
    db_msg("===dump_objects===");
    db_msg("%2s\t%10s\t%10s\t%3s %3s %3s %3s\t%5s\t%10s\t%2s %10s",
        "id","class_name", "ctrl_name", "x", "y", "w", "h",
        "lvl", "text", "p_id", "p_name");
    for(it=objects.begin(); it!=objects.end(); it++, i++) {
        db_msg("%2d\t%10s\t%10s\t%3d %3d %3d %3d\t%5d\t%10s\t%2d %10s",
            i, it->class_name.c_str(), it->ctrl_name.c_str(), it->x,
            it->y, it->w, it->h,
            it->level, it->text.c_str(), it->parent_idx,
            objects[it->parent_idx].ctrl_name.c_str());
    }
}

bool MainParser::analysis()
{
    StringVector::iterator it;
    for (it = lines_.begin(); it != lines_.end();) {
        db_msg("lines :%s", it->c_str());
        if (IsWidget(*it)) {
            ParseWidget(*it++);
        } else {
            db_msg("%s", it->c_str());
            ParseProperty(it);
        }
    }
    dump_objects(objects_);
    return true;
}

void MainParser::generate()
{
    View* ctrl;
    View* parent;
    ObjectVector::iterator it;
    CtrlMap& ctrl_map_ = owner_->GetCtrlMap();
    StringMap& text_map_ = owner_->GetTextMap();
    for ( it=objects_.begin(); it!=objects_.end(); it++)
    {
        if ( it->level != 0) //the controls in the window
        {
            parent = objects_[it->parent_idx].obj;
            db_msg("%s", it->class_name.c_str());
            ctrl = reinterpret_cast<View *>
                (Runtime::Create(it->class_name.c_str(), parent));
            if ( ctrl )
            {
                db_msg("~~ctrl %s %d %p", it->ctrl_name.c_str(),
                    it->parent_idx, parent);
                ctrl->PreInit(it->ctrl_name);
                ctrl->SetCaption(it->text.c_str());
                if (it->class_name == TEXT_VIEW) {
                    //R::LoadStrings fill the it->text;
                    text_map_.insert(make_pair(it->ctrl_name, string("")));
                }
                db_msg(" %d %d %d %d", it->x, it->y, it->w, it->h);
                ctrl->SetPosition(it->x, it->y, it->w, it->h);
                it->obj = ctrl;
                ctrl_map_.insert(make_pair(it->ctrl_name, ctrl));
            } else {
                db_error("fail to generate class %s", it->class_name.c_str());
            }
        } else { //the main window's level is 0
            db_msg("window it->index %s %s %d",
            it->class_name.c_str(), it->ctrl_name.c_str(), it->parent_idx);
            owner_->SetPosition( it->x, it->y, it->w, it->h);
            it->obj = owner_;
        }
    }//end for ( it=m_Nodes.begin(); it!=m_Nodes.end(); it++)
}

