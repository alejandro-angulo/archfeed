typedef struct {
  int  color;
  int  entries;
  int  newest;
  int  poll;
  int  skip;
  int  update;
  int  wrap;
  int  verbose;
  char outfilename[256]; 
} args;

extern args flags;
