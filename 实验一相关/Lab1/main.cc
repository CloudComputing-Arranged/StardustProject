#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>

#include "sudoku.h"

char puzzle[1000][128];
int puzzleans[1000][N];
int nump = 0; //存入数独计数
int numt = 0; //线程计数
int numth = 0; //线程处理数独计数
int total_solved = 0;
int total = 0;

bool (*solve)(int,int) = solve_sudoku_basic;  //

pthread_mutex_t numthLock;
pthread_mutex_t numtLock;
pthread_mutex_t ttLock;
pthread_mutex_t ttsLock;
pthread_mutex_t outLock;

int64_t now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void* runner(void *args)    //线程函数
{
    int numoft;
    pthread_mutex_lock(&numtLock);
    numoft = numt;
    numt++;
    pthread_mutex_unlock(&numtLock);    //获取当前线程号 

    //int* numofth = (int*) args;
    int numofth;
    for(int i=0;i<100;i++)
    {
      pthread_mutex_lock(&numthLock);
      numofth = numth;
      numth++;
      pthread_mutex_unlock(&numthLock);   //实时获取当前处理第几个数独

      if (strlen(puzzle[numofth]) >= N) {
      pthread_mutex_lock(&ttLock);
      ++total;
      pthread_mutex_unlock(&ttLock);    //

      input(puzzle[numofth],numoft);//
      //init_cache();
      if (solve(0,numoft)) { //
        pthread_mutex_lock(&ttsLock);
        ++total_solved;
        pthread_mutex_unlock(&ttsLock);

      pthread_mutex_lock(&outLock);
      int i1;
	    for(i1=0;i1<N;i1++) puzzleans[numofth][i1] = board[numoft][i1];//
      pthread_mutex_unlock(&outLock);

        // if (!solved(numoft)) //这个注释可以取消
        //   assert(0);
      }
      else {
        puzzleans[numofth][0]=-1;
      }
    }

   }
}

// void* intopuzzle(void *args)     //对于单独开设输入线程的尝试
// {
//   FILE* fp = (FILE*) args;
//   while (fgets(puzzle[nump], sizeof puzzle[nump], fp) != NULL) {
//     nump++;
//   }
// }

int main(int argc, char* argv[])
{
  int multt=0;
  for(multt=0;multt<10;multt++)
    init_neighbors(multt);  //

  char filename[10];
  scanf("%s",&filename);//

  FILE* fp = fopen(filename, "r");



  int64_t start = now();

  while (fgets(puzzle[nump], sizeof puzzle[nump], fp) != NULL) {
    nump++;
  }
  // int i3;      //检查输入使用 debug使用
  // for(i3=0;i3<10;i3++)
  // {
  //   for(int i1=0;i1<N;i1++) printf("%c",puzzle[i3][i1]);
  //   printf("\n");
  // }

  // pthread_t th;    //原本想给文件读取单独开一个线程 但是参数传输有问题 所以暂且搁置
  // if(pthread_create(&th, NULL, intopuzzle, &fp)!=0)
  // {
  //   perror("th : pthread_create failed");
  //   exit(1);
  // }

  pthread_mutex_init(&numtLock,NULL);
  pthread_mutex_init(&numthLock,NULL);
  pthread_mutex_init(&ttLock,NULL);
  pthread_mutex_init(&ttsLock,NULL);
  pthread_mutex_init(&outLock,NULL);
  
  pthread_t ths[10];    //创建10个线程 
  int i2;
  for(i2=0;i2<10;i2++)
  {
    if(pthread_create(&ths[i2], NULL, runner, NULL)!=0)
    {
      perror("th : pthread_create failed");
      exit(1);
    }
  }

  //pthread_join(th, NULL);

  for(i2=0;i2<10;i2++)    //等待所有线程完成
    pthread_join(ths[i2], NULL);

  int i3,i4;
  for(i4=0;i4<total;i4++)   //输出结果
  {
    if(puzzleans[i4][0]!=-1)
	    for(i3=0;i3<N;i3++) printf("%d",puzzleans[i4][i3]);//
	  
    else
      printf("No: %s\n", puzzle[i4]);

    printf("\n");
  }

  int64_t end = now();
  double sec = (end-start)/1000000.0;
  printf("%f sec %f ms each %d\n", sec, 1000*sec/total, total_solved);

  return 0;
}

