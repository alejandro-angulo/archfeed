typedef struct {
  int  color;
  int  entries;
  int  poll;
  int  update;
  int  wrap;
  int  verbose;
  char outfilename[255]; 
} args;

extern args flags;
