## PlatformIO Source folder 

This source folder contains symlinks for a PlatformIO, since project includes several Ardiono projects.\

As an example how to link Master as a working part:
```
ln -s ./Master/Master.ino ./src/main.cpp

ln -s ./Master/lib/ ./src/lib/
```
Folders and files are ignored for Git since they are duplicated.

