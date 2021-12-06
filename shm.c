#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
  struct proc *curproc = myproc(); //Get the current process
  char *u_shm_adr; //Get an address
  int i; //Declared for later pruposes
  acquire(&(shm_table.lock)); //Acquire the lock done through init
  for(i = 0; i < 64; i++) { //Go through each of the pages (0 - 63) to find id
    if(shm_table.shm_pages[i].id == id) { //If the id matches with what I want, do
      shm_table.shm_pages[i].refcnt++; //Another wants to point so increase the pointer
      mappages(curproc->pgdir, PGROUNDUP(curproc -> sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
      release(&(shm_table.lock)); //Release the lock since we finished the pointer
      return 0; //Continue since reference increased, return here for faster
    }
  }
  for(i = 0; i < 64; i++) { //Since id isn't found, find a page that is empty
    if(shm_table.shm_pages[i].id == 0) { //If the id matches with what I want, do
      shm_table.shm_pages[i].id = id; //Set the id to the one I want
      shm_table.shm_pages[i].frame = kalloc(); //Given in website explanation
      shm_table.shm_pages[i].refcnt = 1; //There is now 1 reference to it
      memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
      mappages(curproc->pgdir, PGROUNDUP(curproc -> sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
      break; //Do bellow since its the same
    }
  }

  release(&(shm_table.lock)); //Release the lock since the table is full.
  return 0; //The usual continue on, first loop is return since it does it faster
}


int shm_close(int id) {
  int i; //Declared out because need for release later
  initlock(&(shm_table.lock), "SHM lock"); //Begin with a lock since we are doing different operation than open
  acquire(&(shm_table.lock)); //Lock with the new lock
  for(i = 0; i < 64; i++) { //Go through each of the pages (0 - 63)
    if(shm_table.shm_pages[i].id == id) { //If I found the id, do
      shm_table.shm_pages[i].refcnt--; //Close one of the pointers to it
      if(shm_table.shm_pages[i].refcnt > 0) { } //As long as the reference count > 0 do nothing
      else { //Otherwise clear as done in shm_init
        shm_table.shm_pages[i].id = 0;
        shm_table.shm_pages[i].frame = 0;
        shm_table.shm_pages[i].refcnt = 0;
      }
      break; //Breakout since id has been found
    }
  }

  release(&(shm_table.lock)); //Release the lock since we're done
  return 0; //Freed up one so go on
}
