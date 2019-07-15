#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH_LEN (256)   //最大路径长度

static void trave_dir(char* path) {
    DIR *d = NULL;
    struct dirent *dp = NULL; /* readdir函数的返回值就存放在这个结构体中 */
    struct stat st;    
    char p[MAX_PATH_LEN] = {0};
    const char ch = '.';
    
    if(stat(path, &st) < 0 || !S_ISDIR(st.st_mode)) {
        printf("invalid path: %s\n", path);
        return;
    }

    if(!(d = opendir(path))) {
        printf("opendir[%s] error: %m\n", path);
        return;
    }

    while((dp = readdir(d)) != NULL) {
        /* 把当前目录.，上一级目录..及隐藏文件都去掉，避免死循环遍历目录 */
        if((!strncmp(dp->d_name, ".", 1)) || (!strncmp(dp->d_name, "..", 2)))
            continue;

        //组成绝对路径
        snprintf(p, sizeof(p) - 1, "%s/%s", path, dp->d_name);
        stat(p, &st);

        if(!S_ISDIR(st.st_mode)) {  //文件
            char *ret = strrchr(p, ch);//获取最后一个'.'的位置
	        if(ret == NULL)
                continue;
            if(strcmp(ret,".mp3")==0 || strcmp(ret,".MP3")==0) //匹配后缀名
            	printf("%s\n", p);
        } else {     //路径
            printf("%s/\n", dp->d_name);
            trave_dir(p);
        }
    }
    closedir(d);

    return;
}

int main(int argc, char **argv)
{   
    char *path = NULL;
 
    if (argc != 2) {
        printf("Usage: %s [dir]\n", argv[0]);
        printf("use DEFAULT option: %s .\n", argv[0]);
        printf("-------------------------------------------\n");
        path = "./";
    } else {
        path = argv[1];
    }

    trave_dir(path);

    return 0;
}
