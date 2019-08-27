/****************************************************************
** UsbHotplug
** usb热插拔检测
**
**
** usbhotplug.h
**
** 周 Zhou    2019-07-09T10:20:42
**
** 深圳市xx电子股份有限公司
**
**
****************************************************************/
#include "usbhotplug.h"
#include <sys/types.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include<unistd.h>
#include <list>
#define UEVENT_BUFFER_SIZE 2048

using namespace std;

static list<AbstractHotplugInterface *> interfaceList;//接口类列表
static pthread_t pth;//主线程
bool usbState = false;

UsbHotplug* UsbHotplug::m_pInstance = nullptr;

UsbHotplug::UsbHotplug()
{

}

//主循环
void *UsbHotplug::mainloop(void *)
{
    struct sockaddr_nl client;
    struct timeval tv;
    int CppLive, rcvlen, ret;
    fd_set fds;
    int buffersize = 1024;
    CppLive = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    memset(&client, 0, sizeof(client));
    client.nl_family = AF_NETLINK;
    client.nl_pid = getpid();
    client.nl_groups = 1; /* receive broadcast message*/
    setsockopt(CppLive, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
    bind(CppLive, (struct sockaddr*)&client, sizeof(client));
    while (1) {
        char buf[UEVENT_BUFFER_SIZE] = { 0 };
        FD_ZERO(&fds);
        FD_SET(CppLive, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        ret = select(CppLive + 1, &fds, NULL, NULL, &tv);
        if(ret < 0)
            continue;
        if(!(ret > 0 && FD_ISSET(CppLive, &fds)))
            continue;
        /* receive data */
        rcvlen = recv(CppLive, &buf, sizeof(buf), 0);
        if (rcvlen > 0) {
            //std::cout<<"receive data = "<<buf<<std::endl;
            string recv = buf;
            //1-1:1.0/host0/target0:0:0/0:0:0:0/block/sda
            //1-1:1.0/host0/target0:0:0/0:0:0:0/block/sda/sda1
            int index1 = recv.find_last_of("/");
            int index2 = recv.find("block");
            if(index2 > 0 && index1 > (index2+5))   //usb info changed
            {
                std::cout<<"regex_match ..."<<index1<<" "<<index2<<std::endl;
                if(recv.find_first_of("add") == 0)  //usb add
                {
                    usbState = true;
                }
                else if(recv.find_first_of("remove") == 0)  //usb remove
                {
                    usbState = false;
                }
                for(std::list<AbstractHotplugInterface *>::iterator iterator=interfaceList.begin();iterator!=interfaceList.end();iterator++)
                {
                    ((AbstractHotplugInterface *)(*iterator))->onUsbHotplug(usbState);
                }
            }
        }
    }
    close(CppLive);
}

//创建线程运行主循环
void UsbHotplug::run()
{
    pthread_create(&pth,NULL,mainloop,(void*)this);
}

//注册监听接口
int UsbHotplug::registeredlistening(AbstractHotplugInterface *context)
{
    interfaceList.push_back(context);
    return 0;
}

//移除监听接口
int UsbHotplug::unregistered(AbstractHotplugInterface *context)
{
    interfaceList.remove(context);
    return 0;
}

//启动线程
void UsbHotplug::start()
{
    run();
}
