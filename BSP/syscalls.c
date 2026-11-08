#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>
extern char _end;
extern char __heap_limit;
static char *heap_end;
void *_sbrk(ptrdiff_t increment) {
    if(heap_end==0)heap_end=&_end;
    char *previous=heap_end;
    if(heap_end+increment>&__heap_limit){errno=ENOMEM;return (void *)-1;}
    heap_end+=increment;return previous;
}
int _write(int file,char *ptr,int len){(void)file;(void)ptr;return len;}
int _close(int file){(void)file;return -1;}
int _fstat(int file,struct stat *st){(void)file;st->st_mode=S_IFCHR;return 0;}
int _isatty(int file){(void)file;return 1;}
int _lseek(int file,int offset,int whence){(void)file;(void)offset;(void)whence;return 0;}
int _read(int file,char *ptr,int len){(void)file;(void)ptr;(void)len;return 0;}
void _exit(int status){(void)status;for(;;){}}
int _kill(int pid,int sig){(void)pid;(void)sig;errno=EINVAL;return -1;}
int _getpid(void){return 1;}
