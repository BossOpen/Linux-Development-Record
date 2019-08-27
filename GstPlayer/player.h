/****************************************************************
** Player
** 音乐播放器主功能类
**
**
** player.h
**
** 周 Zhou    2019-07-09T10:20:42
**
** 深圳市xx电子股份有限公司
**
**
****************************************************************/
#ifndef PLAYER_H
#define PLAYER_H

#include "abstractplayerinterface.h"
#include <string>

class Player
{
    #ifdef m_pAbstractPlayer
    #undef m_pAbstractPlayer
    #endif
    #define m_pAbstractPlayer (Player::getInstance())
public:
    static Player *getInstance();

    //启动线程
    void start();
    //注册监听接口
    int registeredlistening(AbstractPlayerInterface *context);
    //移除监听接口
    int unregistered(AbstractPlayerInterface *context);
    //
    std::string requestPlayIndex();
    //上一曲
    std::string requestPrev();
    //下一曲
    std::string requestNext();
    //上一个文件夹
    std::string requestPrevFolder();
    //下一个文件夹
    std::string requestNextFolder();
    //快进
    void requestSeekUp();
    //快退
    void requestSeekDn();
    //启动播放
    void requestStartPlay();
    //模式切换
    std::string requestChangeMode();
    //播放暂停
    std::string requestPlayPause();
    //暂停
    void requestPause();
    //退出播放器
    void requestQuitPlayer();

private:
    Player();
    static Player *m_pInstance;
    //主循环
    static void * mainloop(void *);
    //创建线程运行主循环
    void run();
    void requestSeek(int step);
};

#endif // PLAYER_H
