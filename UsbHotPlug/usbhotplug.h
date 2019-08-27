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
#ifndef USBHOTPLUG_H
#define USBHOTPLUG_H

#include "abstracthotpluginterface.h"

class UsbHotplug
{
    #ifdef m_pAbstractUsbHotplug
    #undef m_pAbstractUsbHotplug
    #endif
    #define m_pAbstractUsbHotplug  (UsbHotplug::getInstance())

public:
    static UsbHotplug *getInstance()
    {
         if(m_pInstance == nullptr){
            m_pInstance = new UsbHotplug();
         }
         return m_pInstance;
    }

    //启动线程
    void start();
    //注册监听接口
    int registeredlistening(AbstractHotplugInterface *context);
    //移除监听接口
    int unregistered(AbstractHotplugInterface *context);

private:
    UsbHotplug();
    static UsbHotplug *m_pInstance;
    //主循环
    static void * mainloop(void *);
    //创建线程运行主循环
    void run();
};

#endif // USBHOTPLUG_H
