# FileSystem
This program is user space portable indexed file system like the Ext filesystems(ext2/3/4) and UNIX file systems(ufs). The program allocates approximately 33MB of drive space per disk image and the files within the file system images will persist when the program exits.
# Features
•	Program allocates 4226 blocks for the file system   
•	File system shall support files up to 10KB in size    
•	File system can support up to 128 files         
•	File system uses indexed allocation scheme       
•	File system supports file names up to 32 characters       
•	File system file names must be only alphanumeric with a “.”       
•	File system stores the directory in the first disk block(block 0)     
•	File system allocates blocks 3 – 131 for inodes     
•	File system uses block 1 as an inode map      
•	File system uses block 2 for the free block map     
•	The directory structure for the file system is a single level hierarchy with no subdirectories    
# Commands
•	put <file name> – copy local file to file system image       
•	get <file name> <new file name> - retrieves the file from the file system image and deposits it into the directory under new file name. Note: if new file name is empty it will deposit into a file with the same name          
•	list [-h] – list the files currently in the file system image. [-h] lists hidden files as well      
•	df – displays the amount of disk space left in the file system         
•	open <file image name> - open a file system image       
•	close <file image name> - close a file system image      
•	createfs <disk image name> - create a new file system image      
•	savefs – saves the current file system image          
•	attrib [+attribute][-attribute]<file name> - set or remove attribute for the file. Supported attributes are read-only and hidden. Can only accept 1 attribute change at a time.   
•	quit – exits the application      

