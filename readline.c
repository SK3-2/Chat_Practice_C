#include<unistd.h>

int readline(int fd, char *ptr, int maxlen) { 
  int n, rc; 
  char c; 


  for(n = 1; n < maxlen; n++) { 
    if((rc = read(fd, &c, 1)) == 1) { 
      if (c == '\n' && n==1) {
				*ptr++ = ' '; 
				break;
			}
      if (c == '\n') break; 
      *ptr++ = c; 
    }

    else if (rc == 0) { 
      if(n == 1) return (0); 
      else break; 
    } 
  } 

  *ptr = 0; 
  return (n); 
}
