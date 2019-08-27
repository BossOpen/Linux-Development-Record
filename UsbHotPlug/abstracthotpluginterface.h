#ifndef ABSTRACTHOTPLUGINTERFACE_H
#define ABSTRACTHOTPLUGINTERFACE_H

class AbstractHotplugInterface{
public:
    //usb hotplug
    virtual void onUsbHotplug(bool value)=0;
};
#endif // ABSTRACTHOTPLUGINTERFACE_H
