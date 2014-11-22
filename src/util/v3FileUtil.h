/****************************************************************************
  FileName     [ v3FileUtil.h ]
  PackageName  [ v3/src/util ]
  Synopsis     [ V3 File and Directory Manipulation Functions ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_FILE_UTIL_H
#define V3_FILE_UTIL_H

#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DIR_MOD_MASK 0777

static inline const char* v3GetExecPath() {
   char buf[PATH_MAX + 1];
   if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1) return NULL;
   string str = buf;
   return str.substr(0, str.rfind('/')).c_str();
}

static inline const char* v3GetAbsPath(const char* fileName) {
   // Note that we didn't check if the fileName exists, i.e. return value of realpath()
   if (!fileName) return 0;
   char AbsPath[4096]; realpath(fileName, AbsPath);
   char* absPath = (char*)(malloc(sizeof(char) * (strlen(AbsPath) + 1)));
   strcpy(absPath, AbsPath);
   return absPath;
}

static inline bool v3ExistDir(const char* dirName) {
   struct stat st;
   return (stat(dirName,&st) == 0);
}

static inline void v3DeleteDir(const char* dirName) {
   // Lazy version...
   char* cmd = new char[100];
   strcpy(cmd, "rm -rf ");
   strcat(cmd, dirName);
   system(cmd);
}

static inline bool v3CreateDir(const char* dirName, bool replace = true) {
   if (v3ExistDir(dirName)) {
      if (!replace) return false;
      else v3DeleteDir(dirName);
   }
   int temp = umask(0);
   if ((temp = mkdir(dirName, DIR_MOD_MASK)) != 0) return false;
   return true;
}

// Format Transformation
static inline void dotToPng(const string fileName) {
   const string name = ".v3." + fileName + ".png"; 
   const string cmd = "dot -Tpng " + fileName + " -o " + name + "; mv " + name + " " + fileName;
   system(cmd.c_str());
}

static inline void dotToPs(const string fileName) {
   const string name = ".v3." + fileName + ".ps"; 
   const string cmd = "dot -Tps " + fileName + " -o " + name + "; mv " + name + " " + fileName;
   system(cmd.c_str());
}

#endif

