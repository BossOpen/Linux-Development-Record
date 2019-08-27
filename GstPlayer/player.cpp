/****************************************************************
** Player
** 音乐播放器主功能类
**
**
** player.cpp
**
** 周 Zhou    2019-07-09T10:20:42
**
** 深圳市xx电子股份有限公司
**
**
****************************************************************/
#include "player.h"
#include <glib.h>
#include <gst/gst.h>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <list>

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <iostream>

#include <unistd.h>
#include <baoling/bltimer/Bltimer.h>
#include <baoling/bltimer/ITimerHandler.h>

Bltimer *timer = NULL;

static std::string ModeList[3] = {"one","all","shuf"};
enum MODE
{
    SINGLE=0,
    ALL,
    SHUFFLE
};

enum HANDLE
{
    NEXT=1,
    PRIV,
};

enum STATE
{
    PAUSE=1,
    PLAY,
    STOP,
    QUIT,
    INIT
};

typedef enum {
    GST_PLAY_FLAG_VIDEO = (1 << 0), //we want video output
    GST_PALY_FLAG_AUDIO = (1 << 1),
    GST_PALY_FLAG_TEXT = (1 << 2)
}GstPlayFlags;
gint flags;
Player* Player::m_pInstance= nullptr;
static GMainLoop *loop;
static GstElement /**pipeline,*source,*decoder,*sink,*/*playbin;//定义组件
static guint bus_watch_id,timeadd_watch_id;//bus监视器返回值和定时器返回值，用于注销
static GstBus *bus;

static pthread_t pth;//主线程

static uint listPlayIndex = 0;//当前歌曲索引
static uint listFolderIndex = 0;//当前文件夹索引
static uint listPlayLenth = 0;
static MODE listPlayMode = MODE::ALL;
static STATE listPlayState = STATE::QUIT;
static std::list<AbstractPlayerInterface *> interfaceList;//接口类列表

static std::vector<std::string> playList;//播放列表
static std::vector<int> firstFileIndexList;//文件夹第一个文件列表

#define RANDOM(x) (rand()%x)

#define MAX_PATH_LEN (256)
#define USB_DIR "/run/media/usb"


class  timerInterface :public ITimerHandler{
public:
    //返回结束消息
    virtual void onTimeOut(int msg) override
    {
        GstStateChangeReturn ret = gst_element_set_state(playbin, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr("could not set sate to playing\n");
        }
        g_print("onTimeOut---------\n");
    }
};

//索引号转换为字符串显示
std::string int2str(const unsigned int &index)
{
    std::string str;
    char freq_str[5]={0};
    sprintf(freq_str,"%04u",index);
    str = freq_str;
    return str;
}

//生成歌曲索引
static uint getMusicIndex(HANDLE hd)
{
    uint playIndex = listPlayIndex;
    if(listPlayMode == MODE::ALL)
    {
        if(hd == HANDLE::NEXT)
        {
            playIndex++;
            if(playIndex == listPlayLenth)
            {
                playIndex = 0;
            }
        }
        else if(hd == HANDLE::PRIV)
        {
            if(playIndex == 0)
            {
                playIndex = (listPlayLenth-1);
            }
            else
                playIndex--;
        }
    }
    else if(listPlayMode == MODE::SHUFFLE)
    {
        srand((int)time(0));//设随机数种子
        playIndex = RANDOM(listPlayLenth);
    }
    else
    {
        return playIndex;
    }

    //获取当前文件属于哪个文件夹
    for (size_t i = 0; i < firstFileIndexList.size(); i++)
    {
        //小于当前文件夹第一个文件索引，那么一定属于上一个文件夹
        if(playIndex < firstFileIndexList[i])
        {
            listFolderIndex = i-1;
        }
    }
    //没找到则属于最后一个文件夹
    listFolderIndex = (firstFileIndexList.size()-1);

    return playIndex;
}

/****************************************************************
实现思路：链表存储文件路径。
用一个结构体保存
1. 当前文件夹的path的地址
2.下一个结构体的地址（初始化为NULL）。

遍历时，创建两个结构体指针，folderfirst和folderlast。
用folderfirst遍历指定文件夹（如/home）的时候，
如果出现新的文件夹，就创建新的结构体，用folderlast来增加到链表中。
当前文件夹遍历完成，修改folderfirst指向下一个结构体。
循环结束的条件就是，folderfirst为NULL。
****************************************************************/
//遍历文件夹获取文件列表
void traveDir(std::string path)
{
    sleep(1);

    //保存文件或文件夹路径结构体
    struct foldernode{
      char *path;                //指向文件或文件夹路径
      struct foldernode *next;   //指向下一个结构体
    };

    //DIR结构体类似于FILE，是一个内部结构
    //DIR *opendir(const char *pathname)，即打开文件目录，返回的就是指向DIR结构体的指针
    DIR *dir;
    struct dirent *ptr;//dirent起着一个索引作用
    const char ch = '.';
    char *filename;//当前文件名指针
    char *filepath;//当前文件绝对路径指针
    char *foldername ;//当前文件夹名指针
    char *folderpath ;//当前文件夹绝对路径指针
    bool isFirstFile = true;//是否为当前文件夹第一个文件

    //路径存储链表
    struct foldernode * folderstart;
    folderstart = (struct foldernode *)malloc(sizeof(struct foldernode));
    folderstart->path = (char *)malloc(path.length()+1);
    strcpy(folderstart->path, path.c_str());//路径赋值
    folderstart->next = NULL;

    struct foldernode * folderfirst; //用来搜索目录
    folderfirst = folderstart;

    struct foldernode * folderlast; //用来往链表添加目录
    folderlast = folderstart;

    struct foldernode * folderhead; //用来释放链表
    folderhead = folderstart;

    while(folderfirst != NULL)
    {
        //打开文件夹
        dir = opendir(folderfirst->path);
        if (dir != NULL)
        {
            //读取文件夹
            while ((ptr = readdir(dir)) != NULL)
            {
                //把当前目录.，上一级目录..及隐藏文件都去掉，避免死循环遍历目录
                if(strcmp(ptr->d_name, ".")==0 || strcmp(ptr->d_name, "..")==0)
                    continue;
                else if(ptr->d_type == 8)//文件
                {
                    filename = (char *)malloc(0x100);
                    filepath = (char *)malloc(0x1000);
                    memset(filename, 0x100, 0);
                    memset(filepath, 0x1000, 0);

                    strcpy(filename, ptr->d_name);
                    //组成绝对路径
                    //sprintf(filepath, "%s/%s", folderfirst->path, filename);
                    //播放需要uri格式
                    sprintf(filepath, "file://%s/%s", folderfirst->path, filename);

                    char *ret = strrchr(filepath, ch);//获取最后一个'.'的位置
                    if(ret == NULL)
                        continue;
                    //匹配后缀名
                    if(strcmp(ret,".mp3")==0 || strcmp(ret,".MP3")==0 || strcmp(ret,".wma")==0)
                    {
                        playList.push_back(filepath);
                        //printf("%s\n", filepath);
                        //当前文件夹的第一个文件索引
                        if(isFirstFile)
                        {
                            firstFileIndexList.push_back(playList.size()-1);
                            printf("%s   %d\n", filepath,playList.size()-1);
                            isFirstFile= false;
                        }
                    }

                    //释放内存
                    if(filename != NULL)
                        free(filename);
                    if(filepath != NULL)
                        free(filepath);
                }
                else if(ptr->d_type == 4)//路径
                {
                    foldername = (char *)malloc(0x100);
                    folderpath = (char *)malloc(0x1000);
                    memset(foldername, 0x100, 0);
                    memset(folderpath, 0x1000, 0);

                    strcpy(foldername, ptr->d_name);
                    //组成绝对路径
                    sprintf(folderpath, "%s/%s", folderfirst->path , foldername);
                    //创建新结构体
                    struct foldernode *foldernew;
                    foldernew = (struct foldernode *)malloc(sizeof(struct foldernode));
                    foldernew->path = (char *)malloc(strlen(folderpath)+1);
                    strcpy(foldernew->path, folderpath);//路径赋值
                    foldernew->next = NULL;
                    //指针移到下一个位置
                    folderlast->next = foldernew;
                    folderlast = foldernew;
                    //释放内存
                    if(filename != NULL)
                        free(foldername);
                    if(filepath != NULL)
                        free(folderpath);
                }
            }
        }
        else
        {
            return;
        }

        folderfirst = folderfirst->next; //指针移到下一个位置，准备搜索下一个文件夹

        isFirstFile= true;
        closedir(dir);
    }
    listPlayLenth = playList.size();

    //释放内存
    printf("------------free\n");
    struct foldernode* cur = folderhead;
    struct foldernode* prev = folderhead;
    while(cur != NULL)
    {
        prev = cur;
        cur = cur->next;
        free(prev->path);
        free(prev);
        prev = NULL;
    }
    folderhead = NULL;
    printf("------------free---------\n");
}

//定义消息处理函数
static gboolean onBusCall(GstBus *bus,GstMessage *msg,gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;//这个是主循环的指针，在接受EOS消息时退出循环
    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
        {
            g_print("End of stream\n");       
            if (!interfaceList.empty()){
                 for(std::list<AbstractPlayerInterface *>::iterator iterator=interfaceList.begin();iterator!=interfaceList.end();iterator++)
                 {
                     ((AbstractPlayerInterface *)(*iterator))->onMessage((char*)"PlayEnd");
                 }
            }
            break;
        }
        case GST_MESSAGE_ERROR:
        {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg,&error,&debug);
            g_free(debug);
            g_printerr("ERROR:%s\n",error->message);
            g_error_free(error);

            if (!interfaceList.empty()){
                 for(std::list<AbstractPlayerInterface *>::iterator iterator=interfaceList.begin();iterator!=interfaceList.end();iterator++)
                 {
                     ((AbstractPlayerInterface *)(*iterator))->onMessage((char*)"PlayError");
                 }
            }
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
        {
                GstState oldstate, newstate, pendstate;
                gst_message_parse_state_changed(msg, &oldstate, &newstate, &pendstate);
                //g_print("state changed from %d to %d (%d)\n", oldstate, newstate, pendstate);
                if(newstate == GST_STATE_PLAYING)
                {
                    listPlayState = STATE::PLAY;
                }
                else if(newstate == GST_STATE_PAUSED)
                {
                    listPlayState = STATE::PAUSE;
                }
                else
                {
                    listPlayState = STATE::STOP;
                }

                break;
        }
        default:
             break;
    }
    return TRUE;
}

//(GST_SECOND * 60 * 60)) : 99  hour
//计算时间时分秒
#define GST_TIMES_ARGS(t) \
        GST_CLOCK_TIME_IS_VALID (t) ? \
        (guint) ((((GstClockTime)(t)) / (GST_SECOND * 60)) % 60) : 99, \
        GST_CLOCK_TIME_IS_VALID (t) ? \
        (guint) ((((GstClockTime)(t)) / GST_SECOND) % 60) : 99
//定义时间格式
#define GST_TIMES_FORMAT "%02u:%02u"
//定时获取播放时间
static gboolean onPositionUpdate (GstElement *playbin)
{
    gint64 pos, len;

    if (gst_element_query_position (playbin, GST_FORMAT_TIME, &pos)
    && gst_element_query_duration (playbin, GST_FORMAT_TIME, &len))
    {
        char position[10],lenth[10];
        sprintf(position,GST_TIMES_FORMAT,GST_TIMES_ARGS (pos));
        sprintf(lenth,GST_TIMES_FORMAT,GST_TIMES_ARGS (len));

        if (!interfaceList.empty())
        {
            for(std::list<AbstractPlayerInterface *>::iterator iterator=interfaceList.begin();iterator!=interfaceList.end();iterator++)
            {
                ((AbstractPlayerInterface *)(*iterator))->onCurrentPosition(position,lenth);
            }
        }
    }

    return TRUE;
}

//启动线程
void Player::start()
{
    run();
}

//注册监听接口
int Player::registeredlistening(AbstractPlayerInterface *context)
{
    interfaceList.push_back(context);
    return 0;
}

//移除监听接口
int Player::unregistered(AbstractPlayerInterface *context)
{
    interfaceList.remove(context);
    return 0;
}

std::string Player::requestPlayIndex()
{
    return int2str(listPlayIndex+1);
}

//上一曲
std::string Player::requestPrev()
{
    listPlayIndex = getMusicIndex(HANDLE::PRIV);
    requestStartPlay();
    return int2str(listPlayIndex+1);
}

//下一曲
std::string Player::requestNext()
{
    listPlayIndex = getMusicIndex(HANDLE::NEXT);
    requestStartPlay();
    return int2str(listPlayIndex+1);
}

//上一个文件夹
std::string Player::requestPrevFolder()
{
    if(listFolderIndex == 0)
    {
        listFolderIndex = (firstFileIndexList.size()-1);
    }
    else
        listFolderIndex--;
    //当前播放文件索引
    listPlayIndex = firstFileIndexList[listFolderIndex];
    requestStartPlay();
    return int2str(listFolderIndex+1);
}

//下一个文件夹
std::string Player::requestNextFolder()
{
    listFolderIndex++;
    if(listFolderIndex == firstFileIndexList.size())
    {
        listFolderIndex = 0;
    }
    //当前播放文件索引
    listPlayIndex = firstFileIndexList[listFolderIndex];
    requestStartPlay();
    return int2str(listFolderIndex+1);
}

//快进
void Player::requestSeekUp()
{
    requestSeek(2);
}

//快退
void Player::requestSeekDn()
{
    requestSeek(-2);
}
#include "../handle/audiohandle.h"
//启动播放
void Player::requestStartPlay()
{
    m_pAudioHandle->setSource(1);
    std::cout<<"setSystemVolume "<<m_pAudioHandle->setSystemVolume(8)<<std::endl;
    std::cout<<"unmute "<<m_pAudioHandle->unmute()<<std::endl;

    gst_element_set_state (playbin, GST_STATE_NULL);
    gst_element_set_state (playbin, GST_STATE_READY);

    //设置播放文件
    g_object_set(playbin, "uri", playList[listPlayIndex].c_str(), NULL);
    printf("Now playing %s\r\n",playList[listPlayIndex].c_str());


    //延时播放 时间50*40=2s  循环一次  定时器id为0
    BLTIMER *blTimer = timer->requestBltimer();
    blTimer->trigger = 40;
    blTimer->protracted = false;
    blTimer->id = 10;
    timer->requestAppendTimer(blTimer);

    /*switch (gst_element_set_state (playbin, GST_STATE_PAUSED)) {
        case GST_STATE_CHANGE_FAILURE:
            //ignore, we should get an error message posted on the bus
            break;
        case GST_STATE_CHANGE_NO_PREROLL:
            g_print ("Pipeline is live.\n");
            break;
        case GST_STATE_CHANGE_ASYNC:
            g_print ("Prerolling...\r");
            break;
        default:
            break;
    }*/
    g_print("Running---------\n");
}

//播放暂停
std::string Player::requestPlayPause()
{
    GstStateChangeReturn ret;
    if(listPlayState == STATE::PLAY)
    {
        ret = gst_element_set_state(playbin, GST_STATE_PAUSED);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr("could not set sate to playing\n");
        }
        g_print("Paused\n");
    }
    else
    {
        ret = gst_element_set_state(playbin, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr("could not set sate to playing\n");
        }
        g_print("Playing\n");
    }
    return "NULL";
}

void Player::requestPause()
{
    GstStateChangeReturn ret;
    ret = gst_element_set_state(playbin, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("could not set sate to playing\n");
    }
    g_print("Paused\n");
}

//模式切换
std::string Player::requestChangeMode()
{
    if(listPlayMode == MODE::ALL)
    {
        listPlayMode = MODE::SHUFFLE;
    }
    else if(listPlayMode == MODE::SHUFFLE)
    {
        listPlayMode = MODE::SINGLE;
    }
    else if(listPlayMode == MODE::SINGLE)
    {
        listPlayMode = MODE::ALL;
    }
    return ModeList[listPlayMode];
}

//退出播放器
void Player::requestQuitPlayer()
{
    g_print ("Returned, stopping playback\n");
    GstStateChangeReturn ret = gst_element_set_state(playbin, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("could not set sate to playing\n");
    }
    gst_object_unref (playbin);//取消pipeline注册
    g_print ("Deleting pipeline\n");

    g_source_remove (bus_watch_id);//移除bus监听
    g_source_remove (timeadd_watch_id);//移除定时器

    g_main_loop_quit(loop);//退出主循环
    g_main_loop_unref (loop);//取消循环注册
    /* Out of the main loop, clean up nicely */

    listPlayIndex = 0;//当前歌曲索引
    listFolderIndex = 0;//当前文件夹索引
    listPlayLenth = 0;
    listPlayMode = MODE::ALL;
    listPlayState = STATE::QUIT;

    firstFileIndexList.clear();//清空文件夹第一个文件列表
    playList.clear();//清空播放列表
}

Player::Player()
{
    //初始化定时器
    timer = Bltimer::getInstance();
    timer->registeredlistening(new timerInterface);
    timer->start();
}

//主循环
void *Player::mainloop(void *)
{
    //读取文件列表
    traveDir(USB_DIR);

    printf("playList.size %d \r\n", listPlayLenth);

    gst_init(NULL,NULL);
    loop = g_main_loop_new(NULL,FALSE);//创建主循环，在执行 g_main_loop_run后正式开始循环

    playbin = gst_element_factory_make("playbin", "playbin");
    if (!playbin) {
        g_printerr("could not create playbin\n");
        return (void *)-1;
    }

    g_object_set(playbin, "uri", playList[listPlayIndex].c_str(), NULL);
    printf("uri %s\r\n",playList[listPlayIndex].c_str());

    g_object_get(playbin, "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_VIDEO | GST_PALY_FLAG_AUDIO;
    flags &= ~GST_PALY_FLAG_TEXT;
    g_object_set(playbin, "flags", flags, NULL);

    //得到 管道的消息总线
    bus = gst_pipeline_get_bus((GstPipeline*)playbin);
    //添加消息监视器
    bus_watch_id = gst_bus_add_watch(bus,onBusCall,loop);
    // run timer
    timeadd_watch_id = g_timeout_add (1000, (GSourceFunc) onPositionUpdate, playbin);

    if (!interfaceList.empty()){
         for(std::list<AbstractPlayerInterface *>::iterator iterator=interfaceList.begin();iterator!=interfaceList.end();iterator++)
         {
             ((AbstractPlayerInterface *)(*iterator))->onInitFinish();
         }
    }

    //开始循环
    g_main_loop_run(loop);
    if (!interfaceList.empty()){
         for(std::list<AbstractPlayerInterface *>::iterator iterator=interfaceList.begin();iterator!=interfaceList.end();iterator++)
         {
             ((AbstractPlayerInterface *)(*iterator))->onQuitFinish();
         }
    }
    return (void *)0;
}

//创建线程运行主循环
void Player::run()
{
    pthread_create(&pth,NULL,mainloop,(void*)this);
}

void Player::requestSeek(int step)
{
    gint64 pos_ns, dur_ns, seek_ns;
    GstFormat format;

    format = GST_FORMAT_TIME;

    gst_element_query_duration(playbin,format,&dur_ns);
    gst_element_query_position(playbin,format,&pos_ns);

    seek_ns = pos_ns + step*GST_SECOND;

    if (!gst_element_seek (playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
             GST_SEEK_TYPE_SET, seek_ns,
             GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
    {
        g_print ("Seek failed!\n");
    }
}

Player *Player::getInstance()
{
     if(m_pInstance == nullptr){
        m_pInstance = new Player();
     }
     return m_pInstance;
}
