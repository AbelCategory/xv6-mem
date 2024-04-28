#include "kernel/types.h"

//
#include "user/user.h"

//
#include "ummalloc.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define trunc(x) ((x) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(uint)))
char *top, *head, *lstptr;
uint len;



void insert(char *p, char *pre, uint nxt) {
  *(int *)(pre + 4) = p - top;
  *(int *)(p + 4) = nxt;
}

void remove(char *p, char *pre, uint nxt) {
  *(int *)(pre + 4) = nxt;
}

void split(char *p, char *pre, uint nxt_size) {
  uint siz = *(uint *)p;
  uint nxt = *(uint *)(p + 4);
  // int nn = nxt - top;
  // if(nn > len) nn = -1;
  remove(p, pre, nxt);
  if(siz >= nxt_size + 8) {
    *(uint *)p = nxt_size | 1;
    char *np = p + nxt_size;
    int asiz = siz - nxt_size;
    *(uint *)np = asiz;
    insert(np, pre, nxt);
  } else {
    *(uint *)p |= 1;
  }
}

char* merge(char *p, char *pre, char *pp) {
  uint siz = trunc(*(uint *)p), pre_sze = *(uint *)pre, nxt_sze = 1;
  if(p - top + siz < len) {
    nxt_sze = *(uint *)(p + siz);
  }
  if(!(nxt_sze & 1)) {
    int nn = *(uint *)(p + siz + 4);
    siz += trunc(nxt_sze);
    remove(p + siz, p, nn);
    *(uint *)p = siz;
  }
  if(!(pre_sze & 1)) {
    int nxt = *(uint *)(p + 4);
    siz += trunc(pre_sze);
    remove(p, pre, nxt);
    *(uint *)pre = siz;
    p = pre;
  }
  return p;
}

char* add_heap(uint siz, char *lst, char *pl) {
  void *p = sbrk(siz);
  if(p == (void *)-1) {
    return 0;
  } else {
    // *(uint *)(lst + 4) = p;
    insert(p, lst, -1);
    *(uint *)p = siz;
    *(uint *)(p + 4) = -1;
    len += siz;
    return merge(p, lst,pl);
  }
}

/*
 * mm_init - initialize the malloc package.
 */

int mm_init(void) { 
  top = sbrk(8);
  *(uint *)top = 9;
  *(uint *)(top + 4) = -1;
  return 0; 
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(uint size) {
  int newsize = ALIGN(size + SIZE_T_SIZE);
  // printf("malloc size: %d true size: %d\n", size, newsize);
  char *pl = 0, *lst = top, *p;
  char *rst, *rp;
  int pos = *(int *)(lst + 4), siz;
  uint best = -1, ok = 0;
  while(pos != -1) {
    p = top + pos;
    siz = trunc(*(uint *)p);
    if(siz >= newsize && siz - newsize < best) {
      ok = 1;
      rp = p;
      rst = lst;
      best = siz - newsize;
      // split(p, lst, newsize);
      // return (void *)(p + 8);
    }
    pos = *(int *)(lst + 4);
    pl = p; lst = p;
  }
  if(ok) {
    split(rp, rst, newsize);
    return (void *)(rp + 8);
  }
  // printf("?????\n");
  p = add_heap(newsize, lst, pl);
  lstptr = lst;
  split(p, lst, newsize);
  // printf("size!!!: %d",*(uint *)p);
  return (void *)(p + 8);
  // void *p = sbrk(newsize);
  // if (p == (void *)-1)
  //   return 0;
  // else {
  //   *(uint *)p = size;
  //   return (void *)((char *)p + SIZE_T_SIZE);
  // }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  if(ptr != 0) {
    char *p = (char *)ptr - 8;
    // printf("free ptr: %l\n", (long long)p);
    // insert(p, )
    char *pre = top, *pp = 0;
    while(1) {
      int nxt = *(int *)(pre + 4);
      if(nxt == -1 || nxt >= p - top) {
        // printf("pre: %l nxt: %l\n", (long long)pre, (long long)nxt);
        insert(p, pre, nxt);
        merge(p, pre, pp);
        break;
      }
      pp = pre;
      pre = top + nxt;
    }
  }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, uint size) {
  char *oldptr = (char *)ptr - 8, *newptr;
  // printf("??? %d\n", *(uint *)oldptr);
  uint siz = trunc(*(uint *)oldptr);
  uint newsize = ALIGN(size + 8);
  // printf("realloc ptr: %l old_size: %d, new_size: %d\n", (long long)ptr, siz, newsize);  
  if(siz < newsize && (oldptr - top) + siz >= len) {
    // printf("Case 1\n");
    sbrk(newsize - siz);
    len += newsize - siz;
    *(uint *)oldptr = newsize | 1;
    return (void *)(oldptr + 8);
  }
  if(siz < newsize) {
    // printf("Case 2\n");
    newptr = mm_malloc(size);
    memcpy(newptr, oldptr + 8, siz - 8);
    mm_free((void* )(oldptr + 8));
    return (void *)newptr;
  }
  else {
    // split(oldptr, lstptr, newsize);
    // printf("Case 3\n");
    // printf("%d\n", oldptr - top);
    char *pre = top;
    while(1) {
      int nxt = *(int *)(pre + 4);
      // printf("%d\n", nxt);
      if(nxt == -1 || nxt >= oldptr - top) {
        insert(oldptr, pre, nxt);
        split(oldptr, pre, newsize);
        break;
      }
      pre = top + nxt;
    }
    return (void *)(oldptr + 8);
  }
  // void *oldptr = ptr;
  // void *newptr;
  // uint copySize;

  // if (size == 0) {
  //   free(ptr);
  //   return 0;
  // }
  
  // newptr = mm_malloc(size);
  // if (newptr == 0) return 0;
  // copySize = *(uint *)((char *)oldptr - SIZE_T_SIZE);
  // if (size < copySize) copySize = size;
  // memcpy(newptr, oldptr, copySize);
  // mm_free(oldptr);
  // return newptr;
}
