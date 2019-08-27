/****************************************************************
** AbstractPlayerInterface
** 音乐播放器数据返回接口类
**
**
** abstractplayerinterface.h
**
** 周 Zhou    2019-07-09T10:20:42
**
** 深圳市xx电子股份有限公司
**
**
****************************************************************/
#ifndef ABSTRACTPLAYERINTERFACE_H
#define ABSTRACTPLAYERINTERFACE_H

class AbstractPlayerInterface{
public:
    //返回播放时长和总时长
    virtual void onCurrentPosition(char* position,char* duration)=0;
    //返回错误或结束消息
    virtual void onMessage(char* msg)=0;
    //返回初始化完成状态
    virtual void onInitFinish()=0;
    //返回播放器退出状态
    virtual void onQuitFinish()=0;
};
#endif // ABSTRACTPLAYERINTERFACE_H
