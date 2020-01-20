#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

// 允许存储在/tmp目录中的log的大小, 30M
// 目前log分段大小为25M, 所以/tmp/log目录下当前最多只保存两个日志文件
#define TMP_LOG_SIZE (5*(1<<20))

#define MOUNT_PATH   "/mnt/extsd"
#define TMP_LOG_DIR  "/tmp/log/"
#define LOG_BAK_DIR  MOUNT_PATH "/log_bak/"
// #define TMP_LOG_DIR  "src/"
// #define LOG_BAK_DIR  "bak/"

map<uint64_t, string> g_file_map;

int g_file_cnt = 0;


// 只遍历一层目录
void get_file_size(const char *file_path, uint64_t &size)
{
    struct stat st;
    memset(&st, 0, sizeof(st));

    DIR *dir = NULL;
    struct dirent *dirent = NULL;
    g_file_map.clear();

    int ret = stat(file_path, &st);
    if (ret < 0) {
        //cerr << "get file stat failed: " << strerror(errno) << endl;
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        if ((dir = opendir(file_path)) != NULL) {
            char cur_path[128] = {0};
            getcwd(cur_path, sizeof(cur_path));
            chdir(file_path);
            while ((dirent = readdir(dir))) {
                if ((strstr(dirent->d_name, "INFO"))) {
                    continue;
                }

                stat(dirent->d_name, &st);
                if (!S_ISDIR(st.st_mode)) {
                    size += st.st_size;
                    // cout << "file: " << dirent->d_name << " size: "<< st.st_size << " file last modify time: " << ctime(&st.st_mtime) << endl;
                    g_file_map.insert(make_pair(st.st_mtime, dirent->d_name));
                }
            }
            free(dirent);
            closedir(dir);
            chdir(cur_path);
        }
    } else {
        size = st.st_size;
        g_file_map.insert(make_pair(st.st_mtime, file_path));
    }

    // cout << "total size: " << size << "bytes" << endl;
}

bool is_mounted()
{
    const char *path = "/proc/mounts";
    FILE *fp = NULL;
    char line[255] = {0};

    if (!(fp = fopen(path, "r"))) {
        printf("fopen failed, path=%s\n",path);
        return 0;
    }

    while(fgets(line, sizeof(line), fp)) {
        if (line[0] == '/' && (strstr(line, MOUNT_PATH) != NULL)) {
            fclose(fp);
            fp = NULL;
            return true;
        } else {
            memset(line,'\0',sizeof(line));
            continue;
        }

    }

    if (fp) {
        fclose(fp);
    }

    return false;
}

void back_log_file(const string dest, const string src)
{
    if (dest.empty() || src.empty()) {
        cerr << "dest or src is NULL" << endl;
        return;
    }

    // check mount
    if (!is_mounted()) {
        cerr << "not mount, can not back log file" << endl;
        return;
    }

    /* ignore the file:/tmp/log/sdvcam.INFO */
    if(src.find("INFO") != string::npos) {
        cout << "ignore check the file:" << src << endl;
        return;
    }

    if (access(LOG_BAK_DIR, F_OK) != 0) {
        cerr << "create log back directory" << endl;
        mkdir(LOG_BAK_DIR, 0755);
    }

    stringstream ss;
    ss << "cp -rf " << src << " " << dest;
    system(ss.str().c_str());
    cout << "cmd: " << ss.str() << endl;
    sync();
    int ret = 0;
    ret = truncate(src.c_str(), 0);
    if (ret) {
        fprintf(stderr, "log_guardian remove %s failed, errno:%d  %s\n", src.c_str(), errno, strerror(errno));
    }
    cout << "truncate file to 0 size: " << src << endl;

    g_file_cnt++;
}

int main_ther(int argc, char *argv[])
{
    char num[32] = {0};
    while(1) {
        uint64_t file_size = 0;
        get_file_size(TMP_LOG_DIR, file_size);
        if (file_size > TMP_LOG_SIZE) {
            if (g_file_map.size() > 0) {
                memset(num, 0, sizeof(num));
                sprintf(num, "%d", g_file_cnt);
                string dest = LOG_BAK_DIR + g_file_map.begin()->second + "_" + num;
                string src = TMP_LOG_DIR + g_file_map.begin()->second;
                back_log_file(dest, src);
            }
        }
        sleep(1);
    }

    // map<uint64_t, string>::iterator it;
    // for (it = g_file_map.begin(); it != g_file_map.end(); it++) {
        // cout << "file: " << it->second << " file last modify time: " << it->first << endl;
    // }

    return 0;
}


int main(int argc, char *argv[])
{
    char cmd[128] = {0};
    char log_file[128] = {0};
    string log_src;
    int ret = 0, cnt = 0;
    uint64_t dir_size = 0;
    struct stat st;
    DIR *dir = NULL;
    struct dirent *dirent = NULL;
    char cur_path[128] = {0};

    while (1) {

        dir_size = 0;
        get_file_size(TMP_LOG_DIR, dir_size);

        if (dir_size < TMP_LOG_SIZE) {
            sleep(1);
            continue;
        }

        /* check mount */
        if (is_mounted()) {
            fprintf(stderr, "log_guardian The /tmp/log dir size:%dMB too big.\n", (dir_size)/1024/1024);

            /* create TF-card log back directory */
            if (access(LOG_BAK_DIR, F_OK) != 0) {
                fprintf(stderr, "log_guardian create log back directory\n");
                if (mkdir(LOG_BAK_DIR, 0755) != 0) {
                    fprintf(stderr, "log_guardian create log back directory fail:%s\n", strerror(errno));
                    sleep(1);
                    continue;
                }
            }

            /* cp tmp/log to tf-card  */
            if ((dir = opendir(TMP_LOG_DIR)) != NULL) {
                while ((dirent = readdir(dir))) {

                    fprintf(stderr, "log_guardian dirent->d_name:%s\n", dirent->d_name);

                    /* ignore the sdvcam.INFO file */
                    if ((strstr(dirent->d_name, "INFO"))) {
                        continue;
                    }

                    /* ignore the dirctor file */
                    if (DT_DIR == dirent->d_type) {
                        fprintf(stderr, "log_guardian The file:%s is dir, so ignore.\n", dirent->d_name);
                        continue;
                    }

                    /* cp log file */
                    memset(cmd, 0, sizeof(cmd));
                    //sprintf(cmd, "cp -rf %s%s  %s%s_%d", TMP_LOG_DIR, dirent->d_name, LOG_BAK_DIR, dirent->d_name, cnt++);
                    //system(cmd);
                    //sync();
                    //fprintf(stderr, "log_guardian cmd:%s\n", cmd);

                    /* truncate /tmp/log file */
                    memset(log_file, 0, sizeof(log_file));
                    sprintf(log_file, "%s%s", TMP_LOG_DIR, dirent->d_name);
                    ret = truncate(log_file, 0);
                    if (ret) {
                        fprintf(stderr, "log_guardian truncate %s failed, errno:%d  %s\n\n", log_file, errno, strerror(errno));
                    } else {
                        fprintf(stderr, "log_guardian truncate file:%s to 0 size\n\n", log_file);
                    }
                }
                free(dirent);
                closedir(dir);
            } else {
                fprintf(stderr, "log_guardian get file stat failed: %s\n", strerror(errno));
                sleep(1);
                continue;
            }
        }
        else {

            fprintf(stderr, "log_guardian The /tmp/log dir size:%dMB too big.\n", (dir_size)/1024/1024);

            /* cp tmp/log to tf-card  */
            if ((dir = opendir(TMP_LOG_DIR)) != NULL) {
                while ((dirent = readdir(dir))) {

                    fprintf(stderr, "log_guardian dirent->d_name:%s\n", dirent->d_name);

                    /* ignore the sdvcam.INFO file */
                    if ((strstr(dirent->d_name, "INFO"))) {
                        continue;
                    }

                    /* ignore the dirctor file */
                    if (DT_DIR == dirent->d_type) {
                        fprintf(stderr, "log_guardian The file:%s is dir, so ignore.\n", dirent->d_name);
                        continue;
                    }

                    /* truncate /tmp/log file */
                    memset(log_file, 0, sizeof(log_file));
                    sprintf(log_file, "%s%s", TMP_LOG_DIR, dirent->d_name);
                    ret = truncate(log_file, 0);
                    if (ret) {
                        fprintf(stderr, "log_guardian truncate %s failed, errno:%d  %s\n\n", log_file, errno, strerror(errno));
                    } else {
                        fprintf(stderr, "log_guardian truncate file:%s to 0 size\n\n", log_file);
                    }
                }
                free(dirent);
                closedir(dir);
            } else {
                fprintf(stderr, "log_guardian get file stat failed: %s\n", strerror(errno));
                sleep(1);
                continue;
            }
        }

        sleep(1);
    }

    return 0;
}

