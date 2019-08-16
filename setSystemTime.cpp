#include<time.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>

static time_t curTimer = time(NULL);
char szBuf[10] = {0};
//设置系统时间
int setSysTime(bool timeAdd)
{
    memset(szBuf,0,sizeof(szBuf));
    struct timeval set_tv;
    if(timeAdd)
    {
        curTimer += 60;//时间加一分
    }
    else
        curTimer -= 60;//时间减一分
    set_tv.tv_sec = curTimer;
    set_tv.tv_usec = 0;
    //调用settimeofday函数, 设置系统时间
    if (settimeofday(&set_tv, (struct timezone *)0) < 0) {
        printf("set time error !\r\n");
        return -1;
    }
    //格式化时间为字符串
    strftime(szBuf, sizeof(szBuf), "%H:%M", localtime(&curTimer));
    std::cout<<szBuf<<std::endl;
    return 0;
}

//获取系统时间
void getSysTime()
{
    memset(szBuf,0,sizeof(szBuf));
    curTimer = time(NULL);//获取当前时间
    //格式化时间为字符串
    strftime(szBuf, sizeof(szBuf), "%H:%M", localtime(&curTimer));
    std::cout<<szBuf<<std::endl;
}

int main() {
    getSysTime();
    //misc->requestDisplay(3,szBuf);

    while (1) {
        usleep(1000000);
        setSysTime(true);
        //misc->requestDisplay(3,szBuf);
    }

    return 0;
}
