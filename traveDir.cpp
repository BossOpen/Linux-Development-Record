#include <stdio.h>
#include <dirent.h> 
#include <stdlib.h> 
#include <string.h> 
#include <vector>
#include <string>

using namespace std;

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
void traveDir(string path)
{

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
                    sprintf(filepath, "%s/%s", folderfirst->path, filename);

                    char *ret = strrchr(filepath, ch);//获取最后一个'.'的位置
                    if(ret == NULL)
                        continue;
                    //匹配后缀名
                    if(strcmp(ret,".mp3")==0 || strcmp(ret,".MP3")==0 || strcmp(ret,".wma")==0)
                    {
                        playList.push_back(filepath);
                        printf("%s\n", filepath);
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
        closedir(dir);
    }

    //释放内存
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
}

int main(int argc,char **argv) 
{
	traveDir("/home/blzt/mp3"); 
	return 0;
}
