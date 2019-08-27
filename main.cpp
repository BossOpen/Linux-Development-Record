#include <unistd.h>
#include <iostream>
#include "usbhotplug/usbhotplug.h"

using namespace std;

class  hotplugInterface :public AbstractHotplugInterface{
public:
    virtual void onUsbHotplug(bool value)
    {
        std::cout<<"onUsbHotplug "<<value<<std::endl;
        if(value)
        {
            ;
        }
        else
        {
            ;
        }
    }
};

int main() {
	m_pAbstractUsbHotplug->registeredlistening(new hotplugInterface);
    m_pAbstractUsbHotplug->start();
    while (1) {
        usleep(500000);
    }

    return 0;
}
