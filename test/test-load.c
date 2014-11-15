#include "../src/libtiming.c"

#define REPEAT 1000
/* use a template to generate instruction */
int inst_template(const char* templ, ...);
int main()
{
   volatile int* loadvar = (int *)malloc(sizeof(int));
   //uint64_t t_res = timing_res();
   uint64_t t_err = timing_err();
   unsigned long beg = 0, end = 0, sum = 0, ref = 0;
   for(int i=0;i<REPEAT;i++){
      beg = timing();
      ref = inst_template("load",loadvar);
      end = timing();
      sum += end-beg-t_err;
   }
   sum /= REPEAT;
   printf("load Inst Time:%lu, ref:10000, no use:%lu\n", sum,ref);
}
