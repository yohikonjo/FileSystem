/*
  Name: Rafel Tsige
  ID: 1001417200
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments
#define NUM_BLOCKS 4226 
#define BLOCK_SIZE 8192 
#define FILE_SIZE 10240000
#define NUM_FILES 128 
#define NUM_INODES 128 
#define MAX_BLOCKS_PER_FILE 32 


uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];
int used_blocks[NUM_BLOCKS];
uint32_t num_bytes = NUM_FILES * FILE_SIZE; 
int num_files = 0;     	        // Counts the number of files currently in the file system. 
FILE *curr_image;               // Used to keep track of the current disk image.
char *curr_image_name;          // Used to store the name of the current disk image name.
bool is_open;

struct _DirectoryEntry
{
  char *name;
  uint32_t inode;
  uint8_t valid;
};

struct _DirectoryEntry *directory_ptr;

struct _Inode
{
  int32_t blocks[1250];
  time_t file_time;
  uint32_t size;
  uint8_t valid;
  uint8_t read_attrib;
  uint8_t hide_attrib;
};

struct _Inode *inode_ptr;

int *free_inode_map;
int *free_block_map; 

/*Initializes the values within the file system.*/
void initialize()
{
  int i;
  int inode_idx = 0;
  directory_ptr = (struct _DirectoryEntry *)&blocks[0][0];
  free_block_map = (int *)&blocks[1][0];
  free_inode_map = (int *)&blocks[2][0];
  inode_ptr = (struct _Inode*) &blocks[5][0];
  for(i = 0; i < NUM_FILES; i++)
  {
    /*Directory Initializtions*/
    directory_ptr[i].valid = 0;
    directory_ptr[i].inode = -1;

    /*Inode Initializtions*/
    
    inode_ptr[i].valid = 0;
    inode_ptr[i].hide_attrib = 0;
    inode_ptr[i].read_attrib = 0;
    
    int j;
    for(j = 0; j < 1250; j++)
    {
      inode_ptr[i].blocks[j] = -1;
    }

    /*Free Inode Map Initializations*/
    free_inode_map[i] = 0;

    /*Free Block Map Initializations*/
    free_block_map[i] = 0;
    
  }

  /*Used Blocks Initialization*/
  for(i = 0; i < NUM_BLOCKS; i++)
  {
    used_blocks[i] = 0;
  }
}

/*Finds and returns an index that is not being used in the file system's directory*/
int findFreeDirectoryEntry()
{
  int i; 
  int retval = -1;
  for(i = 0; i < NUM_FILES; i++)
  {
    if(directory_ptr[i].valid == 0)
    {
      retval = i;
      break;
    }
  }
  return retval;
}

/*Finds and returns an index that is not being used in the file system's inode list*/
int findFreeInode()
{
  int i;
  int retval = -1;
  for(i = 0; i < NUM_FILES; i++)
  {
    if(inode_ptr[i].valid == 0)
    {
      retval = i;
      break;
    }
  }
  return retval;
}

/*Finds and returns an index of block that is not being used in an inode in the file system*/
int findFreeInodeBlockEntry(int inode_index)
{
  int i; 
  int retval = -1;
  for(i = 0; i < 32; i++)
  {
    if(inode_ptr[inode_index].blocks[i] == -1)
    {
      retval = i;
      break;
    }
  }
  return retval;
}

/*Finds and returns an index of block that is not being used in the file system*/
int findFreeBlock()
{
  int retval = -1;
  int i = 0;
  for(i = 130; i < 4226; i++)
  {
    if(used_blocks[i] == 0)
    {
      retval = i;
      break;
    }
  }
  return retval;
}

/*Finds and returns the index where the file with the same file name is stored in the file system*/
int findFile(char *filename)
{
  int i;
  for(i = 0; i < NUM_FILES; i++)
  {
    if(strcmp(filename, directory_ptr[i].name) == 0 && directory_ptr[i].valid == 1)
    {
      return i;
    }
  }
  return -1;
}

/*Switches the attribute values of the file given.*/
void attrib(char * att, char * filename)
{ 
  int dir_idx = findFile(filename);
  int inode_idx = directory_ptr[dir_idx].inode;
  if(dir_idx == -1)
  {
    printf("attrib: File not found.");
    return;
  }

  if(strcmp(att, "-h") == 0)
  {
    inode_ptr[inode_idx].hide_attrib = 0;
    return;
  }
  
  if(strcmp(att, "+h") == 0)
  {
    inode_ptr[inode_idx].hide_attrib = 1;
    return;
  }

  if(strcmp(att, "-r") == 0)
  {
    inode_ptr[inode_idx].read_attrib = 0;
    return;
  }

  if(strcmp(att, "+r") == 0)
  {
    inode_ptr[inode_idx].hide_attrib = 1;
    return;
  }
  else
  {
    printf("attrib error: Invalid input.\n");
    return;
  }
}

/*List all the files in the file system image that are not hidden*/
void list()
{  
  int i, inode_idx, len;
  char temp[30], time[30] = {"Thu Jan 01 11:59:59 1970"};
  if(num_files == 0)
  {
    printf("list: No files found.\n");
    return;
  }
  else
  {
    for(i = 0; i < NUM_FILES; i++)
    {
      inode_idx = directory_ptr[i].inode;
      if(inode_idx != -1 && directory_ptr[i].valid == 1  && inode_ptr[inode_idx].hide_attrib != 1)
      {
        strcpy(temp, ctime(&(inode_ptr[inode_idx].file_time)));
        len = strlen(temp);
        memcpy(time, temp, len-1);
        printf("%d %s %s\n", inode_ptr[inode_idx].size, time, directory_ptr[i].name);
        
      }
         
    }  
    return;
  }
  
}

/*Lists all the files in the file system image*/
void hid_list(char *hide)
{
  int i, inode_idx, len, read;
  int hidden = 2;
  char temp[30], time[30] = {"Thu Jan 01 11:59:59 1970"};
  if(num_files == 0)
  {
    printf("list: No files found.\n");
    return;
  }
  else
  {
    if(strcmp(hide, "-h") == 0)
    {
      hidden = 1;
    }

    else
    {
      printf("list: Invalid parameter entry.\n");
      return;
    }
    for(i = 0; i < NUM_FILES; i++)
    {
      inode_idx = directory_ptr[i].inode;
      if(inode_idx != -1 && directory_ptr[i].valid == 1  && hidden == 1)
      {
        strcpy(temp, ctime(&(inode_ptr[inode_idx].file_time)));
        len = strlen(temp);
        memcpy(time, temp, len-1);
        printf("%d %s %s\n", inode_ptr[inode_idx].size, time, directory_ptr[i].name);
        
      }
         
    }  
    return;
  }
}

/*Deletes the given file from the file system*/
void del(char *filename1)
{
  int i, inode_idx;
  int dir_idx = findFile(filename1);
  int inode_index = directory_ptr[dir_idx].inode;
  if(strlen(filename1) == 0 || num_files == 0 || dir_idx == -1)
  {
    printf("del error: File not found.");
    return;
  }
  if(inode_ptr[inode_index].read_attrib == 1 || inode_ptr[inode_index].hide_attrib == 1)
  {
    printf("del error: You do not have access to this file.\n");
    return;
  }
  for(i = 0; i < NUM_FILES; i++)
  {
    inode_idx = directory_ptr[i].inode;
    if(strcmp(filename1, directory_ptr[i].name) == 0 && (inode_ptr[i].read_attrib == 0 && inode_ptr[i].hide_attrib == 0))
    {
      directory_ptr[i].valid = 0;
      directory_ptr[i].inode = -1;
      inode_ptr[i].valid = 0;
      inode_ptr[i].read_attrib = 0;
      inode_ptr[i].hide_attrib = 0;
      int j;
      for(j = 0; j < 1250; j++)
      {
        inode_ptr[i].blocks[j] = -1;
      }
      free_inode_map[i] = 0;
      used_blocks[inode_ptr[inode_idx].blocks[i]] = -1;
      num_files--;
      printf("del: File has been successfully removed from image.\n");
      return;
    }
  }
  return;
}

/*Displays the amount of disk space remaining in the file system*/
int df()
{
  int count = 0;
  int i;

  for(i = 130; i < NUM_BLOCKS; i++)
  {
    if(used_blocks[i] == 0)
    {
      count++;
    }
  }
  return count * BLOCK_SIZE;
}

/*Copies the contents of the given file from the file system image into the directory into a file with the same name*/
int put(char *filename)
{
  struct stat buf;

  int status = stat(filename, &buf);

  if(strlen(filename) > 32)
  {
    printf("Error: Filename length exceeded maximum.\n");
  }

  if(status == -1)
  {
    printf("Error: File not found.\n");
    return;
  }

  if(buf.st_size > df())
  {
    printf("Error: Not enough room in the file system \n");
    return;
  }

  int dir_idx = findFreeDirectoryEntry();
  if(dir_idx == -1)
  {
    printf("Error: Not enough room in the file system \n");
    return;
  }

  directory_ptr[dir_idx].valid = 1;
  
  directory_ptr[dir_idx].name = (char*)malloc(strlen(filename));

  strcpy(directory_ptr[dir_idx].name, filename);

  int inode_idx = findFreeInode();

  if(inode_idx == -1)
  {
    printf("Error: No free inodes\n");
    return;
  }

  directory_ptr[dir_idx].inode = inode_idx;

  inode_ptr[inode_idx].size = buf.st_size;
  inode_ptr[inode_idx].file_time = time(NULL);
  inode_ptr[inode_idx].valid = 1;
  inode_ptr[inode_idx].read_attrib = 0;
  inode_ptr[inode_idx].hide_attrib = 0;

  //Open the input file read-only
  FILE *ifp = fopen(filename, "r");

  int copy_size = buf.st_size;
  int offset = 0;


  while(copy_size >= BLOCK_SIZE)
  {
    int block_index = findFreeBlock();

    if(block_index == -1)
    {
      printf("Error: Can't find free block\n");
      // Cleanup a bunch of directory and inode stuff
      return 1;
    }
    used_blocks[block_index] = 1;
    
    int inode_block_entry = findFreeInodeBlockEntry(inode_idx);
    if(inode_block_entry == -1)
    {
      printf("Error: Can't find free node block\n");
      //Cleanup a bunch of directory and inode stuff
      return;
    }
    inode_ptr[inode_idx].blocks[findFreeInodeBlockEntry(inode_idx)] = block_index;

    //Index into the input file by offset number of bytes. Initially offset is set to 
    //zero so we copy BLOCK_SIZE number of bytes from the front of the file. We then 
    //increase the offset by BLOCK_SIZE and continue the process. This will make us 
    //copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
    fseek(ifp, offset, SEEK_SET);

    //Read BLOCK_SIZE number of bytes from the input file and store them in our 
    //data array.
    int bytes = fread( blocks[block_index], BLOCK_SIZE, 1, ifp);

    //If bytes == 0 and we haven't reached the end of the file then something is 
    //wrong. If 0 is returned and we also have the EOF flag set then that is OK.
    //It means we've reached the end of our input file.
    if(bytes == 0 && !feof(ifp))
    {
      printf("An error occurred read from the input file.\n");
      return -1;
    }
    clearerr(ifp);

    copy_size -= BLOCK_SIZE;
    offset += BLOCK_SIZE;
    
  }

  if(copy_size > 0)
  {
    int block_index = findFreeBlock();

    if(block_index == -1)
    {
      printf("Error: Can't find free bock\n");
      // Cleanup a bunch of directory and inode stuff
      return;
    }

    int inode_block_entry = findFreeInodeBlockEntry(inode_idx);
    if(inode_block_entry == -1)
    {
      printf("Error: Can't find free node block\n");
      //Cleanup a bunch of directory and inode stuff
      return;
    }
    inode_ptr[inode_idx].blocks[findFreeInodeBlockEntry(inode_idx)] = block_index;
    
    used_blocks[block_index] = 1;
    num_files++;
    
    //handle remainder
    fseek(ifp, offset, SEEK_SET);
    int bytes = fread(blocks[block_index], copy_size, 1, ifp);
  }
  fclose(ifp);
  return;
}

/*Retrieves the given file from the file system and places its contents into the other file given if there is one or in itself if not.*/
int get(char *filename)
{ 
  int dir_idx = findFile(filename);
  int inode_idx = directory_ptr[dir_idx].inode;
  if(dir_idx == -1 || inode_ptr[inode_idx].hide_attrib == 1)
  {
    printf("get error: The file you are trying to get is not on the file system image.");
    return;
  }

  if(inode_ptr[inode_idx].read_attrib == 1)
  {
    printf("get error: You do not have permission to access this file.");
    return;
  }
  
  struct _Inode *inode = &(inode_ptr[inode_idx]);
 
  int    status;                   // Hold the status of all return values.
  struct stat buf;             // stat struct to hold the returns from the stat call
  // Call stat with out input filename to verify that the file exists.  It will also 
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't 
  // care about anything but the filesize.
  status =  stat( filename, &buf ); 
  
  if(status != -1)
  {
    FILE *ofp;
    ofp = fopen(filename, "w");

    if( ofp == NULL )
    {
      printf("Could not open output file: %s\n", filename );
      perror("Opening output file returned");
      return -1;
    }

    // Initialize our offsets and pointers 
    int block_index = 0;
    int copy_size   = buf.st_size;
    int offset      = 0;

    printf("Writing %d bytes to %s\n", (int)buf.st_size, filename);

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file. 
    while( copy_size > 0 )
    { 

      int num_bytes; 

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else 
      {
        num_bytes = BLOCK_SIZE;
      }

      int new_block_index = inode->blocks[block_index];

      // Write num_bytes number of bytes from our data array into our output file.
      fwrite(&blocks[new_block_index], num_bytes, 1, ofp); 

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      block_index ++;

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek(ofp, offset, SEEK_SET);
    }
      // Close the output file, we're done. 
    fclose(ofp);
  }
}


/*Copies the contents of the given file from the file system image into the directory into a new file with the given name*/
int new_get(char *filename, char *new)
{ 
  int dir_idx = findFile(filename);
  int inode_idx = directory_ptr[dir_idx].inode;
  
  if(dir_idx == -1 || inode_ptr[inode_idx].hide_attrib == 1)
  {
    printf("get error: The file you are trying to get is not on the file system image.");
    return;
  }

  if(inode_ptr[inode_idx].read_attrib == 1)
  {
    printf("get error: You do not have permission to access this file.");
    return;
  }
  
  struct _Inode *inode = &(inode_ptr[inode_idx]);
 
  int    status;                   // Hold the status of all return values.
  struct stat buf;             // stat struct to hold the returns from the stat call
  // Call stat with out input filename to verify that the file exists.  It will also 
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't 
  // care about anything but the filesize.
  status =  stat( filename, &buf ); 
  
  if(status != -1)
  {
    FILE *ofp;
    ofp = fopen(new, "w");

    if( ofp == NULL )
    {
      printf("Could not open output file: %s\n", new);
      perror("Opening output file returned");
      return -1;
    }

    // Initialize our offsets and pointers 
    int block_index = 0;
    int copy_size   = buf.st_size;
    int offset      = 0;

    printf("Writing %d bytes to %s\n", (int)buf.st_size, new);

    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file. 
    while( copy_size > 0 )
    { 

      int num_bytes; 

      // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
      // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
      // end up with garbage at the end of the file.
      if( copy_size < BLOCK_SIZE )
      {
        num_bytes = copy_size;
      }
      else 
      {
        num_bytes = BLOCK_SIZE;
      }

      int new_block_index = inode->blocks[block_index];

      // Write num_bytes number of bytes from our data array into our output file.
      fwrite(&blocks[new_block_index], num_bytes, 1, ofp); 

      // Reduce the amount of bytes remaining to copy, increase the offset into the file
      // and increment the block_index to move us to the next data block.
      copy_size -= BLOCK_SIZE;
      offset    += BLOCK_SIZE;
      block_index ++;

      // Since we've copied from the point pointed to by our current file pointer, increment
      // offset number of bytes so we will be ready to copy to the next area of our output file.
      fseek(ofp, offset, SEEK_SET);
    }
      // Close the output file, we're done. 
    fclose(ofp);
  }
}

/*Creates a new file system*/
void createfs(char *file)
{
  FILE *ifp = fopen(file, "w");
  if(ifp == NULL)
  {
    printf("createfs: File not found.\n");
    return;
  }
  memset(&blocks[0], 0, NUM_BLOCKS*BLOCK_SIZE);
  initialize();
  printf("File system created.\n");
  //fwrite(&blocks[0], BLOCK_SIZE, NUM_BLOCKS, ifp);
}

/*Saves the current file system*/
void savefs()
{
  if(is_open == true)
  {
    curr_image = fopen(curr_image_name, "w");
    fwrite(&blocks[0][0], BLOCK_SIZE, NUM_BLOCKS, curr_image);
    fclose(curr_image);
    printf("File system saved.\n");
  }
  else
  {
    printf("save error: No file system is open.\n");
    return;
  }
}

/*Opens the file system that the user requeested.*/
void open_image(char *file)
{
  FILE *ifp = fopen(file, "r");
  if(ifp == NULL)
  {
    printf("Error: File system image not found.\n");
    return;
  }
  fread(&blocks, NUM_BLOCKS, BLOCK_SIZE, ifp);
  curr_image = ifp;
  curr_image_name = (char*)malloc(strlen(file));
  strcpy(curr_image_name, file);
  is_open = true;
  printf("Opened file system image %s successfully.\n", file);
  fclose(ifp);
}

/*Closes the current file system*/
void close_image(char *file)
{
  if(file == NULL)
  {
    printf("close error: No file system image name given.\n");
    return;
  }

  if(strcmp(file, curr_image_name) == 0)
  {
    printf("Closed file system image %s.\n", file);
    curr_image = NULL;
    curr_image_name = NULL;
    is_open = false;
    return;
  }
  else
  {
    printf("close error: File system %s is currently not open.", file);
  }
}

int main()
{
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];
    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    } 

    if(strcmp(token[0], "quit") != 0 && strcmp(token[0], "createfs") != 0 && strcmp(token[0], "open") != 0 && strcmp(token[0], "df") != 0 && strcmp(token[0], "put") != 0 && strcmp(token[0], "attrib") != 0 && strcmp(token[0], "list") != 0 && strcmp(token[0], "del") != 0 && strcmp(token[0], "close") != 0 && strcmp(token[0], "savefs") != 0 && strcmp(token[0], "get") != 0)
    {
      printf("Error: Invalid command.\n");
      continue;
    }

    if(strcmp(token[0], "quit") == 0)
    {
      exit(0);
    }

    else
    {
      if(is_open == false)
      {
        if(strcmp(token[0], "createfs") == 0)
        {
          createfs(token[1]);
          continue;
        }
        
        if(strcmp(token[0], "open") == 0)
        {
          open_image(token[1]);
          continue;
        }
        else
        {
          printf("Error: No file system image open.\n");
          continue;
        }
      }
      if(strcmp(token[0], "df") == 0)
      {
        printf("%d bytes free.\n", df());
      }

      if(strcmp(token[0], "put") == 0)
      {
        put(token[1]);
        continue;
      }

      if(strcmp(token[0], "list") == 0)
      {
        if(token_count < 3)
        {
          list();
        }
        else
        {
          hid_list(token[1]); 
        }   
      }

      if(strcmp(token[0], "del") == 0)
      {
        del(token[1]);
      }

      if(strcmp(token[0], "open") == 0)
      {
        printf("Error: File system image already open.\n");
      }

      if(strcmp(token[0], "close") == 0)
      {
        close_image(token[1]);
      }

      if(strcmp(token[0], "savefs") == 0)
      {
        savefs();
      }
    
      if(strcmp(token[0], "get") == 0)
      {
        if(token_count < 4)
        {
          get(token[1]);
        }
        else
        {
          new_get(token[1], token[2]);
        }
      }

      if(strcmp(token[0], "attrib") == 0)
      {
        attrib(token[1], token[2]);
      }
     
    }
    
    free( working_root );

  }
  return 0;
}
