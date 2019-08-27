#include <unistd.h>
#include <iostream>
#include "player/player.h"

using namespace std;


class  playerInterface :public AbstractPlayerInterface{
public:
    virtual void onCurrentPosition(char* position,char* duration) override {
        std::cout<<position<<duration<<std::endl;
    }
    virtual void onMessage(char* msg) override {
        std::cout<<msg<<std::endl;
    }
    virtual void onInitFinish() override {
        std::cout<<"onInitFinish"<<std::endl;
    }
    virtual void onQuitFinish() override {
        std::cout<<"onQuitFinish"<<std::endl;
    }
};

int main() {
	m_pAbstractPlayer->registeredlistening(new playerInterface);
	m_pAbstractPlayer->start();
    while (1) {
        usleep(500000);
    }

    return 0;
}
