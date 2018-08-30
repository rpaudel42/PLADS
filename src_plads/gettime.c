#include <time.h>
#include <stdio.h>

double  theseSecs = 0.0;
double  startSecs = 0.0;
double  secs;
double  CPUsecs = 0.0;
double  CPUutilisation = 0.0;
double  answer = 0;
clock_t starts;

  void start_CPU_time()
  {      
      starts = clock();;
      return;
  }

  void end_CPU_time()
  {
      CPUsecs = (double)(clock() - starts)/(double)CLOCKS_PER_SEC;
      return;
  }    

  struct timespec tp1;
  void getSecs()
  {
     clock_gettime(CLOCK_REALTIME, &tp1);
     theseSecs =  tp1.tv_sec + tp1.tv_nsec / 1e9;           
     return;
  }

  void start_time()
  {
      getSecs();
      startSecs = theseSecs;
      return;
  }

  void end_time()
  {
      getSecs();
      secs    = theseSecs - startSecs;
      return;
  }    

  void calculate()
  {
      int i, j;
      for (i=1; i<100001; i++)
      {
          for (j=1; j<10001; j++)
          {
              answer = answer + (float)i / 100000000.0;
          }
      }
  }
      
 void main()
 {
     start_time();
     start_CPU_time();
     calculate();
     end_time();
     end_CPU_time();
     CPUutilisation = CPUsecs /  secs * 100.0;
     printf("\n Answer %10.1f, Elapsed Time %7.4f, CPU Time %7.4f, CPU Ut %3.0f%\n",
              answer, secs, CPUsecs, CPUutilisation);  
 }
