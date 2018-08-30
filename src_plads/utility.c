//******************************************************************************
// utility.c
//
// Miscellaneous utility functions.
//
// Date      Name       Description
// ========  =========  ========================================================
// 08/07/17  Eberle     Initial version.
//
//******************************************************************************

#include "plads.h"

//******************************************************************************
// NAME: OutOfMemoryError
//
// INPUTS: (char *context)
//
// RETURN: (void)
//
// PURPOSE: Print out of memory error with context, and then exit.
//******************************************************************************

void OutOfMemoryError(char *context)
{
   printf("ERROR: out of memory allocating %s.\n", context);
   exit(1);
}


//******************************************************************************
// NAME: PrintBoolean
//
// INPUTS: (BOOLEAN boolean)
//
// RETURN: (void)
//
// PURPOSE: Print BOOLEAN input as 'TRUE' or 'FALSE'.
//******************************************************************************

void PrintBoolean(BOOLEAN boolean)
{
   if (boolean)
      printf("true\n");
   else
      printf("false\n");
}


//******************************************************************************
// NAME: FindOldestFile
//
// INPUTS: char * - directory of files
//
// RETURN: struct dirent * - oldest file
//
// PURPOSE: Find oldest file in specified directory.
//******************************************************************************

struct dirent * FindOldestFile(char * directory)
{
   struct dirent * oldestEntry = NULL;
   struct dirent * entry = NULL;
   char fullName[FILE_NAME_LEN];
   char oldestFileName[FILE_NAME_LEN];
   struct stat stbuf;
   long int oldestTime = MAX_TIME_STAMP;
   DIR * d = NULL;
   BOOLEAN atLeastOneFile = FALSE;

   // Open link to directory containing files
   d = opendir(directory);
   if (!d)
   {
     fprintf(stderr, "Cannot open directory '%s': %s\n",
             directory, strerror (errno));
     exit(EXIT_FAILURE);
   }

   // search directory for oldest file
   while (d)
   {
      // See if there is another unread file
      entry = readdir(d);
      if (entry)
      {
         // skip self and parent
         if ((strcmp(entry->d_name,".") == 0) || (strcmp(entry->d_name,"..") == 0))
         {
            //printf("   ... skipping self/parent\n");
            continue;
         }
         // concatenate dirctory to file name
         strcpy(fullName,directory);
         strcat(fullName,entry->d_name);
         //printf ("   processing graph input file: %s\n", fullName);

         // get file statistics (i.e., modified time)
         stat(fullName,&stbuf);
         //printf("   time last modified: %li\n", stbuf.st_mtime);

         // see if the file is the oldest...
         if (stbuf.st_mtime < oldestTime)
         {
            oldestTime = stbuf.st_mtime;
            oldestEntry = entry;
            sprintf(oldestFileName,"%s",entry->d_name);
         }
         atLeastOneFile = TRUE;
      }
      else
         // no more files to process
         break;
   }

   // if there is an oldest file, save it to the entry name
   //
   // NOTE:  This is a kludge due to some memory leak (I think) when
   //        the number of files in the directory is large...  gets me
   //        by for now...
   //
   if (atLeastOneFile)
   {
      if (strlen(oldestFileName) > 1)
      {
         sprintf(oldestEntry->d_name,"%s",oldestFileName);
      }
      else
      {
         printf("   ... oldestEntry->d_name = NULL\n");
         fflush(stdout);
      }
   }

   // close directory
   closedir(d);

   return oldestEntry;
}


//******************************************************************************
// NAME: MoveFile
//
// INPUTS: (Configuration *)
//
// RETURN: struct dirent * - oldest file
//
// PURPOSE: Move file from one directory to another.
//******************************************************************************

int MoveFile(char * entry,char * sourceDirectory,char * destDirectory)
{
   int status = 0;
   char fullSourceName[FILE_NAME_LEN];
   char fullDestName[FILE_NAME_LEN];

   strcpy(fullSourceName,sourceDirectory);
   strcat(fullSourceName,entry);

   strcpy(fullDestName,destDirectory);
   strcat(fullDestName,entry);

   status = rename(fullSourceName, fullDestName);

   if (status != 0)
      printf("Error: unable to move the following file: %s\n",fullSourceName);

   return status;
}


//******************************************************************************
// NAME: MoveAnomalousSubstructureFiles
//
// INPUTS: int numPartition - partition number of anomalous substructures
//
// RETURN: int - number of anomalous substructure files moved
//
// PURPOSE: Move anomalous substructure files and count them.
//******************************************************************************

ULONG MoveAnomalousSubstructureFiles(ULONG numPartition, char * directory)
{
   struct dirent * entry;
   DIR * d;
   char fullName[FILE_NAME_LEN];
   char moveFileFrom[FILE_NAME_LEN];
   char moveFileTo[FILE_NAME_LEN];
   ULONG numAnomSub = 0;
   FILE *file;

   // Open link to directory containing files
   d = opendir(".");
   if (!d)
   {
     fprintf(stderr, "Cannot open directory '%s': %s\n",
             directory, strerror (errno));
     exit(EXIT_FAILURE);
   }

   while (d)
   {
      // See if there is another unread file
      entry = readdir(d);
      if (entry)
      {
         sprintf(fullName,"anom_%lu_%lu",numPartition,numAnomSub+1);
         // does file exist?
         file = fopen(fullName,"r");
         if (file != NULL)
         {
            fclose(file);
            sprintf(moveFileFrom,"%s",fullName);
            sprintf(moveFileTo,"%s%s",directory,fullName);
            rename(moveFileFrom,moveFileTo);

            // move corresponding anomalous instance file as well
            // NOTE:  This assumes there will always be one....
            sprintf(fullName,"anomInst_%lu_%lu",numPartition,numAnomSub+1);
            sprintf(moveFileFrom,"%s",fullName);
            sprintf(moveFileTo,"%s%s",directory,fullName);
            rename(moveFileFrom,moveFileTo);

            numAnomSub++;
         }
         else
            break;
      }
      else
         // no more files to process
         break;
   }

   // close directory
   closedir(d);

   return numAnomSub;
}


//******************************************************************************
// NAME: AllocateNormativePatternList
//
// INPUTS: (void)
//
// RETURN: (NormativePatternList *) - newly-allocated empty normative pattern list
//
// PURPOSE: Allocate and return an empty normative pattern list.
//******************************************************************************

NormativePatternList *AllocateNormativePatternList(void)
{
   NormativePatternList *normativePatternList;

   normativePatternList = (NormativePatternList *) malloc(sizeof(NormativePatternList));
   if (normativePatternList == NULL)
      OutOfMemoryError("AllocateNormativePatternList:normativePatternList");
   normativePatternList->head = NULL;

   return normativePatternList;
}


//******************************************************************************
// NAME: AllocateNormativePattern
//
// INPUTS: (ULONG partitionNum) - partition number of this normative pattern
//         (ULONG bestSubNum) - best substructure number in this partition
//
// RETURN: (NormativePattern *) - pointer to newly allocated normative pattern
//
// PURPOSE: Allocate and return space for new normative pattern.
//******************************************************************************

NormativePattern *AllocateNormativePattern(ULONG partitionNum, ULONG bestSubNum)
{
   NormativePattern *normativePattern;

   normativePattern = (NormativePattern *) malloc(sizeof(NormativePattern));
   if (normativePattern == NULL)
      OutOfMemoryError("AllocateNormativePattern:normativePattern");
   normativePattern->partitionNumber = partitionNum;
   normativePattern->bestSubNumber = bestSubNum;
   normativePattern->score = 0;
   strcpy(normativePattern->graphInputFileName,"");
   normativePattern->parentNormativePattern = NULL;

   return normativePattern;
}

//******************************************************************************
// NAME: NormativePatternListInsert
//
// INPUTS: (NormativePattern *normativePattern) - normative pattern to insert
//         (NormativePatternList *normativePatternList) - list to insert into
//
// RETURN: (void)
//
// PURPOSE: Insert given normative pattern on to given normative pattern list.
//******************************************************************************

void NormativePatternListInsert(NormativePattern *normativePattern,
                                NormativePatternList *normativePatternList)
{
   NormativePatternListNode *normativePatternListNode;

   normativePatternListNode = AllocateNormativePatternListNode(normativePattern);
   normativePatternListNode->next = normativePatternList->head;
   normativePatternList->head = normativePatternListNode;
}


//******************************************************************************
// NAME: AllocateNormativePatternListNode
//
// INPUTS: (NormativePattern *normativePattern) - normative pattern to be stored
//                                                in node
//
// RETURN: (NormativePatternListNode *) - newly allocated NormativePatternListNode
//
// PURPOSE: Allocate a new NormativePatternListNode.
//******************************************************************************

NormativePatternListNode *AllocateNormativePatternListNode(NormativePattern *normativePattern)
{
   NormativePatternListNode *normativePatternListNode;

   normativePatternListNode = (NormativePatternListNode *) malloc(sizeof(NormativePatternListNode));
   if (normativePatternListNode == NULL)
      OutOfMemoryError("AllocateNormativePatternListNode:NormativePatternListNode");
   normativePatternListNode->normativePattern = normativePattern;
   normativePatternListNode->next = NULL;

   return normativePatternListNode;
}


//******************************************************************************
// NAME: AllocateAnomalousSubstructureList
//
// INPUTS: (void)
//
// RETURN: (AnomalousSubstructureList *) - newly-allocated empty anomalous
//                                         substructure list
//
// PURPOSE: Allocate and return an empty anomalous substructure list.
//******************************************************************************

AnomalousSubstructureList *AllocateAnomalousSubstructureList(void)
{
   AnomalousSubstructureList *anomalousSubstructureList;

   anomalousSubstructureList = (AnomalousSubstructureList *)
                               malloc(sizeof(AnomalousSubstructureList));
   if (anomalousSubstructureList == NULL)
      OutOfMemoryError("AllocateAnomalousSubstructureList:anomalousSubstructureList");
   anomalousSubstructureList->head = NULL;

   return anomalousSubstructureList;
}


//******************************************************************************
// NAME: AllocateAnomalousSubstructure
//
// INPUTS: (ULONG partitionNum) - partition number of this anomalous substructure
//         (ULONG anonNum) - number of this anomaly in this partition
//
// RETURN: (AnomalousSubstructure *) - pointer to newly allocated anomalous
//                                     substructure
//
// PURPOSE: Allocate and return space for new anomalous substructure.
//******************************************************************************

AnomalousSubstructure *AllocateAnomalousSubstructure(ULONG partitionNum,
                                                     ULONG anomNum)
{
   AnomalousSubstructure *anomalousSubstructure;

   anomalousSubstructure = (AnomalousSubstructure *)
                           malloc(sizeof(AnomalousSubstructure));
   if (anomalousSubstructure == NULL)
      OutOfMemoryError("AllocateAnomalousSubstructure:anomalousSubstructure");
   anomalousSubstructure->partitionNumber = partitionNum;
   anomalousSubstructure->anomalousNumber = anomNum;
   anomalousSubstructure->score = 0.0;
   //anomalousSubstructure->mostAnomalous = FALSE;
   anomalousSubstructure->parentAnomalousSubstructure = NULL;

   return anomalousSubstructure;
}


//******************************************************************************
// NAME: AnomalousSubstructureListInsert
//
// INPUTS: (AnomalousSubstructure *anomalousSubstructure) - anomalous
//                                                       substructure to insert
//         (AnomalousSubstructureList *anomalousSubstructureList) - list to
//                                                                  insert into
//
// RETURN: (void)
//
// PURPOSE: Insert given anomalous substructure on to given anomalous substructure list.
//******************************************************************************

void AnomalousSubstructureListInsert(AnomalousSubstructure *anomalousSubstructure,
                                AnomalousSubstructureList *anomalousSubstructureList)
{
   AnomalousSubstructureListNode *anomalousSubstructureListNode;

   anomalousSubstructureListNode = AllocateAnomalousSubstructureListNode(anomalousSubstructure);
   anomalousSubstructureListNode->next = anomalousSubstructureList->head;
   anomalousSubstructureList->head = anomalousSubstructureListNode;
}


//******************************************************************************
// NAME: AllocateAnomalousSubstructureListNode
//
// INPUTS: (AnomalousSubstructure *anomalousSubstructure) - anomalous
//                                             substructure to be stored in node
//
// RETURN: (AnomalousSubstructureListNode *) - newly allocated
//                                             AnomalousSubstructureListNode
//
// PURPOSE: Allocate a new AnomalousSubstructureListNode.
//******************************************************************************

AnomalousSubstructureListNode *AllocateAnomalousSubstructureListNode(AnomalousSubstructure *anomalousSubstructure)
{
   AnomalousSubstructureListNode *anomalousSubstructureListNode;

   anomalousSubstructureListNode = (AnomalousSubstructureListNode *)
                                   malloc(sizeof(AnomalousSubstructureListNode));
   if (anomalousSubstructureListNode == NULL)
      OutOfMemoryError("AllocateAnomalousSubstructureListNode:AnomalousSubstructureListNode");
   anomalousSubstructureListNode->anomalousSubstructure = anomalousSubstructure;
   anomalousSubstructureListNode->next = NULL;

   return anomalousSubstructureListNode;
}


//******************************************************************************
// NAME: FreeNormativePattern
//
// INPUTS: (NormativePattern *normativePattern) - normativePattern to free
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of given normativePattern, if there are no more
// references to it.
//******************************************************************************

void FreeNormativePattern(NormativePattern *normativePattern)
{
   if (normativePattern != NULL)
   {
      free(normativePattern);
   }
}


//******************************************************************************
// NAME: FreeNormativePatternListNode
//
// INPUTS: (NormativePatternListNode *normativePatternListNode)
//
// RETURN: (void)
//
// PURPOSE: Free memory used by given normativePattern list node.
//******************************************************************************

void FreeNormativePatternListNode(NormativePatternListNode *normativePatternListNode)
{
   if (normativePatternListNode != NULL)
   {
      FreeNormativePattern(normativePatternListNode->normativePattern);
      free(normativePatternListNode);
   }
}


//******************************************************************************
// NAME: FreeNormativePatternList
//
// INPUTS: (NormativePatternList *normativePatternList)
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of normativePattern list.
//******************************************************************************

void FreeNormativePatternList(NormativePatternList *normativePatternList)
{
   NormativePatternListNode *normativePatternListNode;
   NormativePatternListNode *normativePatternListNode2;

   if (normativePatternList != NULL)
   {
      normativePatternListNode = normativePatternList->head;
      while (normativePatternListNode != NULL)
      {
         normativePatternListNode2 = normativePatternListNode;
         normativePatternListNode = normativePatternListNode->next;
         FreeNormativePatternListNode(normativePatternListNode2);
      }
      free(normativePatternList);
   }
}


//******************************************************************************
// NAME: FreeAnomalousSubstructure
//
// INPUTS: (AnomalousSubstructure *anomalousSubstructure) - anomalousSubstructure
//                                                          to free
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of given anomalousSubstructure, if there are no more
// references to it.
//******************************************************************************

void FreeAnomalousSubstructure(AnomalousSubstructure *anomalousSubstructure)
{
   if (anomalousSubstructure != NULL)
   {
      free(anomalousSubstructure);
   }
}


//******************************************************************************
// NAME: FreeAnomalousSubstructureListNode
//
// INPUTS: (AnomalousSubstructureListNode *anomalousSubstructureListNode)
//
// RETURN: (void)
//
// PURPOSE: Free memory used by given anomalousSubstructure list node.
//******************************************************************************

void FreeAnomalousSubstructureListNode(AnomalousSubstructureListNode *anomalousSubstructureListNode)
{
   if (anomalousSubstructureListNode != NULL)
   {
      FreeAnomalousSubstructure(anomalousSubstructureListNode->anomalousSubstructure);
      free(anomalousSubstructureListNode);
   }
}


//******************************************************************************
// NAME: FreeAnomalousSubstructureList
//
// INPUTS: (AnomalousSubstructureList *anomalousSubstructureList)
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of anomalousSubstructure list.
//******************************************************************************

void FreeAnomalousSubstructureList(AnomalousSubstructureList *anomalousSubstructureList)
{
   AnomalousSubstructureListNode *anomalousSubstructureListNode;
   AnomalousSubstructureListNode *anomalousSubstructureListNode2;

   if (anomalousSubstructureList != NULL)
   {
      anomalousSubstructureListNode = anomalousSubstructureList->head;
      while (anomalousSubstructureListNode != NULL)
      {
         anomalousSubstructureListNode2 = anomalousSubstructureListNode;
         anomalousSubstructureListNode = anomalousSubstructureListNode->next;
         FreeAnomalousSubstructureListNode(anomalousSubstructureListNode2);
      }
      free(anomalousSubstructureList);
   }
}


//******************************************************************************
// NAME:  CountNumberOfAnomalousInstances
//
// INPUTS:  (ULONG firstPartition) - first partition number
//          (char * directory) - directory of anomalous substructures
//
// RETURN: int - number of anomalous substructure files
//
// PURPOSE: Count number of anomalous substructures for this partition range.
//
// NOTE:  This assumes that there is not a skip in the numbering of anomalous
//        instances, which is a reasonable assumption...
//******************************************************************************

ULONG CountNumberOfAnomalousInstances(ULONG firstPartition, 
                                      ULONG lastPartition,
                                      char * directory)
{
   char fullName[FILE_NAME_LEN];
   ULONG numAnomSub = 0;
   FILE *file;

   ULONG numPartition = firstPartition;
   ULONG anomSubNumber = 1;

   while (numPartition <= lastPartition)
   {
      sprintf(fullName,"%sanom_%lu_%lu",directory,numPartition,anomSubNumber);
      // does file exist?
      file = fopen(fullName,"r");
      if (file != NULL)
      {
         fclose(file);
         anomSubNumber++;
         numAnomSub++;
      }
      else
      {
         numPartition++;
         anomSubNumber = 1;
      }
   }

   return numAnomSub;
}


//******************************************************************************
// NAME: AllocatePidInfoList
//
// INPUTS: (void)
//
// RETURN: (PidInfoList *) - newly-allocated empty PID info list
//
// PURPOSE: Allocate and return an empty PID info list.
//******************************************************************************

PidInfoList *AllocatePidInfoList(void)
{
   PidInfoList *pidInfoList;

   pidInfoList = (PidInfoList *) malloc(sizeof(PidInfoList));
   if (pidInfoList == NULL)
      OutOfMemoryError("AllocatePidInfoList:pidInfoList");
   pidInfoList->head = NULL;

   return pidInfoList;
}


//******************************************************************************
// NAME: AllocatePidInfo
//
// INPUTS: (pid_t PID) - PID (process ID)
//
// RETURN: (PidInfo *) - pointer to newly allocated PID info substructure
//
// PURPOSE: Allocate and return space for new PID info substructure.
//******************************************************************************

PidInfo *AllocatePidInfo(pid_t pid)
{
   PidInfo *pidInfo;

   pidInfo = (PidInfo *) malloc(sizeof(PidInfo));
   if (pidInfo == NULL)
      OutOfMemoryError("AllocatePidInfo:pidInfo");
   pidInfo->PID = pid;

   return pidInfo;
}


//******************************************************************************
// NAME: PidInfoListInsert
//
// INPUTS: (PidInfo *pidInfo) - PID info substructure to insert
//         (PidInfoList *pidInfoList) - list to insert into
//
// RETURN: (void)
//
// PURPOSE: Insert given PID info substructure on to given PID info list.
//******************************************************************************

void PidInfoListInsert(PidInfo *pidInfo, PidInfoList *pidInfoList)
{
   PidInfoListNode *pidInfoListNode;

   pidInfoListNode = AllocatePidInfoListNode(pidInfo);
   pidInfoListNode->next = pidInfoList->head;
   pidInfoList->head = pidInfoListNode;
}


//******************************************************************************
// NAME: AllocatePidInfoListNode
//
// INPUTS: (PidInfo *pidInfo) - PID info to be stored in node
//
// RETURN: (PidInfoListNode *) - newly allocated PidInfoListNode
//
// PURPOSE: Allocate a new PidInfoListNode.
//******************************************************************************

PidInfoListNode *AllocatePidInfoListNode(PidInfo *pidInfo)
{
   PidInfoListNode *pidInfoListNode;

   pidInfoListNode = (PidInfoListNode *)
                                   malloc(sizeof(PidInfoListNode));
   if (pidInfoListNode == NULL)
      OutOfMemoryError("AllocatePidInfoListNode:PidInfoListNode");
   pidInfoListNode->pidInfo = pidInfo;
   pidInfoListNode->next = NULL;

   return pidInfoListNode;
}


//******************************************************************************
// NAME: PidInfoListDelete
//
// INPUTS: (pid_t pid) - pid to be deleted from list
//         (PidInfoList *pidInfoList) - list to have pid deleted
//
// RETURN: (void)
//
// PURPOSE: Remove given PID info substructure from the given PID info list.
//******************************************************************************

void PidInfoListDelete(pid_t pid, PidInfoList *pidInfoList)
{
   PidInfoListNode *pidInfoListNode;
   PidInfoListNode *pidInfoListNodePrev;
   PidInfoListNode *pidInfoListNodeNext;

   if (pidInfoList != NULL)
   {
      pidInfoListNode = pidInfoList->head;
      while (pidInfoListNode != NULL)
      {
         if (pidInfoListNode->pidInfo->PID == pid)
         {
            if (pidInfoListNode == pidInfoList->head)
            {
               pidInfoListNodeNext = pidInfoListNode->next;
               pidInfoList->head = pidInfoListNodeNext;
               pidInfoListNodePrev = pidInfoListNode;
               FreePidInfoListNode(pidInfoListNodePrev);
               return;
            }
            else
            {
               pidInfoListNodeNext = pidInfoListNode->next;
               pidInfoListNodePrev->next = pidInfoListNodeNext;
               FreePidInfoListNode(pidInfoListNode);
               return;
            }
         }
         pidInfoListNodePrev = pidInfoListNode;
         pidInfoListNode = pidInfoListNode->next;
      }
   }
}


//******************************************************************************
// NAME: FreePidInfo
//
// INPUTS: (PidInfo *pidInfo) - pidInfo to free
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of given pidInfo, if there are no more
// references to it.
//******************************************************************************

void FreePidInfo(PidInfo *pidInfo)
{
   if (pidInfo != NULL)
   {
      free(pidInfo);
   }
}


//******************************************************************************
// NAME: FreePidInfoListNode
//
// INPUTS: (PidInfoListNode *pidInfoListNode)
//
// RETURN: (void)
//
// PURPOSE: Free memory used by given pidInfo list node.
//******************************************************************************

void FreePidInfoListNode(PidInfoListNode *pidInfoListNode)
{
   if (pidInfoListNode != NULL)
   {
      FreePidInfo(pidInfoListNode->pidInfo);
      free(pidInfoListNode);
   }
}


//******************************************************************************
// NAME: FreePidInfoList
//
// INPUTS: (PidInfoList *pidInfoList)
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of pidInfo list.
//******************************************************************************

void FreePidInfoList(PidInfoList *pidInfoList)
{
   PidInfoListNode *pidInfoListNode;
   PidInfoListNode *pidInfoListNode2;

   if (pidInfoList != NULL)
   {
      pidInfoListNode = pidInfoList->head;
      while (pidInfoListNode != NULL)
      {
         pidInfoListNode2 = pidInfoListNode;
         pidInfoListNode = pidInfoListNode->next;
         FreePidInfoListNode(pidInfoListNode2);
      }
      free(pidInfoList);
   }
}

//******************************************************************************
// NAME:  RemoveNoLongerNeededFiles
//
// INPUTS:  (char * directory) - directory of anomalous substructures
//          (ULONG firstPartition) - starting partition to delete all from
//          (ULONG lastPartition) - ending partition to delete all from
//
// RETURN: void
//
// PURPOSE: Do a wildcard remove on all files from the specified directory
//          from a specified partition to a specified partition.
//******************************************************************************

void RemoveNoLongerNeededFiles(char * directory, ULONG firstPartition,
                               ULONG lastPartition)
{
   char rmCommand[FILE_NAME_LEN];
   ULONG partition;

   for (partition = firstPartition; partition <= lastPartition; partition++)
   {
      sprintf(rmCommand,"rm %s*_%lu_*",directory,partition);
      system(rmCommand);
   }
}


//******************************************************************************
// NAME: CalculateDensity
//
// INPUTS: (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//
// RETURN: double - density
//
// PURPOSE: Calculate the density of the specified graph input file.
//******************************************************************************

double CalculateDensity(Configuration *configuration, char * entry,
                        char * sourceDirectory)
{
   double density = 0.0;
   char fullSourceName[FILE_NAME_LEN];
   char stringValue[FILE_NAME_LEN];
   FILE *graphInputFilePtr;
   ULONG numVertices = 0;
   ULONG numEdges = 0;

   strcpy(fullSourceName,sourceDirectory);
   strcat(fullSourceName,entry);

   graphInputFilePtr = fopen(fullSourceName,"r"); // read mode
   if (graphInputFilePtr == NULL)
   {
      perror("Error opening graph input file for calculating density -- exiting PLADS.\n");
      exit(-1);
   }

   // Loop over lines, skipping lines starting with "//", until end of file
   //
   // NOTE:  This doesn't properly read a string, but all I care about is
   //        whether or not the first character is a "v" or "e".
   //
   fscanf(graphInputFilePtr,"%s",stringValue);
   while (!feof(graphInputFilePtr))
   {
      if (strcmp(stringValue,"v") == 0)
      {
         numVertices++;
      }
      else
      {   
         if ((strcmp(stringValue,"e") == 0) ||
             (strcmp(stringValue,"d") == 0) ||
             (strcmp(stringValue,"u") == 0))
         {
            numEdges++;
         }
      }
      fscanf(graphInputFilePtr,"%s",stringValue);
   }
   fclose(graphInputFilePtr);

   ULONG max_edges = 0;
   max_edges = numVertices * numVertices;
   density = (double)numEdges / (double)max_edges;

   return density;
}


//******************************************************************************
// NAME: AddEntryToGPFile
//
// INPUTS: 
//
// RETURN: 
//
// PURPOSE: 
//******************************************************************************

void AddEntryToGPFile(ULONG partition, double value, const char *fileName)
{
   FILE *filePtr = fopen(fileName,"a");

   fprintf(filePtr,"%lu %lf\n",partition,value);

   fclose(filePtr);
}


//******************************************************************************
// NAME: CalculateMeanFromGPFile
//
// INPUTS: 
//
// RETURN: 
//
// PURPOSE: 
//******************************************************************************

double CalculateMeanFromGPFile(const char *fileName)
{
   FILE *filePtr = fopen(fileName,"r");
   if (filePtr == NULL)
      return 0;

   double mean;
   double sum = 0.0;
   ULONG partition;
   double value;
   ULONG num = 0;

   while (fscanf(filePtr,"%lu %lf",&partition,&value) != EOF)
   {
      sum = sum + value;
      num++;
   }

   fclose(filePtr);

   mean = (sum/num);

   return mean;
}


//******************************************************************************
// NAME: CalculateStandardDeviationFromGPFile
//
// INPUTS: 
//
// RETURN: 
//
// PURPOSE: 
//******************************************************************************

double CalculateStandardDeviationFromGPFile(double mean,const char *fileName)
{
   FILE *filePtr = fopen(fileName,"r");
   if (filePtr == NULL)
      return 0;

   double stddev;
   double sumDeviation = 0.0;
   ULONG partition;
   double value;
   ULONG num = 0;

   while (fscanf(filePtr,"%lu %lf",&partition,&value) != EOF)
   {
      sumDeviation = sumDeviation + (value-mean)*(value-mean);
      num++;
   }
   fclose(filePtr);

   stddev = sqrt(sumDeviation/num);

   return stddev;
}


//******************************************************************************
// NAME: UpdateGPFile
//
// INPUTS: 
//
// RETURN: 
//
// PURPOSE: 
//
// NOTE:  This assumes that there is at least one entry in the file.
//******************************************************************************

void UpdateGPFile(Configuration *configuration, ULONG partition, double value,
                  const char *fileName)
{
   FILE *filePtr = fopen(fileName,"r");
   if (filePtr == NULL)
   {
      printf("*** ERROR:  Unable to open %s file -- EXITING PLADS\n",fileName);
      exit(-1);
   }

   gpNode entries[configuration->NUM_PARTITIONS];

   int num = 0;
   while (fscanf(filePtr,"%lu %lf",&entries[num].partition,&entries[num].value) != EOF)
   {
      num++;
   }
   fclose(filePtr);

   int status;
   status = remove(fileName);
   if (status != 0)
      printf("UpdateGPFile:  Unable to remove %s\n",fileName);

   // save old entries
   int j;
   for (j=1; j<num; j++)  // starting at 1 because old partition value needs to roll off...
      AddEntryToGPFile(entries[j].partition,entries[j].value,fileName);

   // add new entry to end
   AddEntryToGPFile(partition,value,fileName);

}


//******************************************************************************
// NAME: CreateVerticesAndEdgesFiles
//
// INPUTS: (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//         (ULONG uniqNumber) - number assigned to uniquely identify file(s)
//
// RETURN: (void)
//
// PURPOSE: Create vertices_#.txt and edges_#.txt file from graph input file.
//          These files are needed by multiple change detection scripts.
//******************************************************************************

void CreateVerticesAndEdgesFiles(char * entry, char * sourceDirectory, 
                                 ULONG uniqNumber)
{
   char fullSourceName[FILE_NAME_LEN];
   char fullVertexName[FILE_NAME_LEN];
   char fullEdgeName[FILE_NAME_LEN];
   char label[80];
   char marker[2];
   int v1, v2;
   FILE *graphInputFilePtr;

   strcpy(fullSourceName,sourceDirectory);
   strcat(fullSourceName,entry);

   graphInputFilePtr = fopen(fullSourceName,"r"); // read mode
   if (graphInputFilePtr == NULL)
   {
      perror("Error opening graph input file -- exiting PLADS.\n");
      exit(-1);
   }

   //
   // Read in graph input file
   //
   // NOTE:  For now, this is done file-based - obviously it would be faster
   //        to do it in memory.
   //
   // Loop over lines, skipping lines starting with "//", until end of file
   FILE *vertexFilePtr;
   sprintf(fullVertexName,"vertices_%lu.txt",uniqNumber);
   vertexFilePtr = fopen(fullVertexName,"w");
   FILE *edgeFilePtr;
   sprintf(fullEdgeName,"edges_%lu.txt",uniqNumber);
   edgeFilePtr = fopen(fullEdgeName,"w");
   while (fscanf(graphInputFilePtr,"%s",marker) != EOF)
   {
      if (strcmp(marker,"v") == 0)
      {
         fscanf(graphInputFilePtr,"%d",&v1);
         fprintf(vertexFilePtr,"%d\n",v1);
         fscanf(graphInputFilePtr,"%s",label);
      }
      else
      {   
         if ((strcmp(marker,"e") == 0) ||
             (strcmp(marker,"d") == 0) ||
             (strcmp(marker,"u") == 0))
         {
            fscanf(graphInputFilePtr,"%d",&v1);
            fscanf(graphInputFilePtr,"%d",&v2);
            fprintf(edgeFilePtr,"%d %d\n",v1,v2);
            fscanf(graphInputFilePtr,"%s",label);
         }
      }
   }
   fclose(graphInputFilePtr);
   fclose(vertexFilePtr);
   fclose(edgeFilePtr);
}


//******************************************************************************
// NAME: CalculateConnectedness
//
// INPUTS: (Configuration * configuration) - PLADS configuration
//         (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//         (ULONG uniqNumber) - number assigned to uniquely identify file(s)
//
// RETURN: double - connectedness
//
// PURPOSE: Calculate the connectedness of the specified graph input file.
//******************************************************************************

double CalculateConnectedness(Configuration *configuration, char * entry,
                             char * sourceDirectory, ULONG uniqNumber)
{
   char fullSourceName[FILE_NAME_LEN];
   double connectedness = 0.0;
   FILE *filePtr;

   CreateVerticesAndEdgesFiles(entry, sourceDirectory, uniqNumber);

   //
   // Run connectedness script
   //
   int status;
   char connectCommand[128];
   const char* connectednessExecutable = configuration->CONNECTEDNESS_EXECUTABLE;
   sprintf(connectCommand, "%s %lu", connectednessExecutable, uniqNumber);
   status = system(connectCommand);
   if (status == 0)
   {
      //
      // Read results and return connectedness value
      //
      sprintf(fullSourceName,"connectedness_%lu.txt",uniqNumber);
      filePtr = fopen(fullSourceName,"r");
      if (fscanf(filePtr,"%lf",&connectedness) == EOF)
      {
         printf("*** ERROR - unable to retrieve connectedness -- exiting PLADS\n");
         fflush(stdout);
         exit(-1);
      }
   }
   else
   {
      printf("*** ERROR - unable to calculate connectedness -- exiting PLADS\n");
      fflush(stdout);
      exit(-1);
   }

   // Remove temporary files
   char rmCommand[FILE_NAME_LEN];
   sprintf(rmCommand,"rm vertices_%lu.txt",uniqNumber);
   system(rmCommand);
   sprintf(rmCommand,"rm edges_%lu.txt",uniqNumber);
   system(rmCommand);
   sprintf(rmCommand,"rm connectedness_%lu.txt",uniqNumber);
   system(rmCommand);

   return (connectedness*1000);   // to create a number with higher precision for later comparisons
}


//******************************************************************************
// NAME: CalculateClusteringCoefficient
//
// INPUTS: (Configuration * configuration) - PLADS configuration
//         (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//         (ULONG uniqNumber) - number assigned to uniquely identify file(s)
//
// RETURN: double - clustering coefficient
//
// PURPOSE: Calculate the clustering coefficient of the specified graph input 
//          file.
//******************************************************************************

double CalculateClusteringCoefficient(Configuration *configuration, char * entry,
                                      char * sourceDirectory, ULONG uniqNumber)
{
   char fullSourceName[FILE_NAME_LEN];
   double clusteringCoefficient = 0.0;
   FILE *filePtr;

   CreateVerticesAndEdgesFiles(entry, sourceDirectory, uniqNumber);

   //
   // Run clustering coefficient script
   //
   int status;
   char clusterCommand[128];
   const char* clusteringCoefficientExecutable = configuration->CLUSTERING_EXECUTABLE;
   sprintf(clusterCommand, "%s %lu", clusteringCoefficientExecutable, uniqNumber);
   status = system(clusterCommand);
   if (status == 0)
   {
      //
      // Read results and return clustering coefficient value
      //
      sprintf(fullSourceName,"clustering_%lu.txt",uniqNumber);
      filePtr = fopen(fullSourceName,"r");
      if (fscanf(filePtr,"%lf",&clusteringCoefficient) == EOF)
      {
         printf("*** ERROR - unable to retrieve clustering coefficient -- exiting PLADS\n");
         fflush(stdout);
         exit(-1);
      }
   }
   else
   {
      printf("*** ERROR - unable to calculate clustering coefficient -- exiting PLADS\n");
      fflush(stdout);
      exit(-1);
   }

   // Remove temporary files
   char rmCommand[FILE_NAME_LEN];
   sprintf(rmCommand,"rm vertices_%lu.txt",uniqNumber);  // created from call to CreateVerticesAndEdgesFiles
   system(rmCommand);
   sprintf(rmCommand,"rm edges_%lu.txt",uniqNumber);     // created from call to CreateVerticesAndEdgesFiles
   system(rmCommand);
   sprintf(rmCommand,"rm clustering_%lu.txt",uniqNumber);
   system(rmCommand);

   return (clusteringCoefficient);
}


//******************************************************************************
// NAME: CalculateEigenvalue
//
// INPUTS: (Configuration * configuration) - PLADS configuration
//         (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//         (ULONG uniqNumber) - number assigned to uniquely identify file(s)
//
// RETURN: double - eigenvalue
//
// PURPOSE: Calculate the eigenvalue of the specified graph input 
//          file.
//******************************************************************************

double CalculateEigenvalue(Configuration *configuration, char * entry,
                           char * sourceDirectory, ULONG uniqNumber)
{
   char fullSourceName[FILE_NAME_LEN];
   double eigenvalue = 0.0;
   FILE *filePtr;

   CreateVerticesAndEdgesFiles(entry, sourceDirectory, uniqNumber);

   //
   // Run eigenvalue script
   //
   int status;
   char eigenvalueCommand[128];
   const char* eigenvalueExecutable = configuration->EIGENVALUE_EXECUTABLE;
   sprintf(eigenvalueCommand, "%s %lu", eigenvalueExecutable, uniqNumber);
   status = system(eigenvalueCommand);
   if (status == 0)
   {
      //
      // Read results and return eigenvalue value
      //
      sprintf(fullSourceName,"eigenvalue_%lu.txt",uniqNumber);
      filePtr = fopen(fullSourceName,"r");
      if (fscanf(filePtr,"%lf",&eigenvalue) == EOF)
      {
         printf("*** ERROR - unable to retrieve eigenvalue -- exiting PLADS\n");
         fflush(stdout);
         exit(-1);
      }
   }
   else
   {
      printf("*** ERROR - unable to calculate eigenvalue -- exiting PLADS\n");
      fflush(stdout);
      exit(-1);
   }

   // Remove temporary files
   char rmCommand[FILE_NAME_LEN];
   sprintf(rmCommand,"rm vertices_%lu.txt",uniqNumber);  // created from call to CreateVerticesAndEdgesFiles
   system(rmCommand);
   sprintf(rmCommand,"rm edges_%lu.txt",uniqNumber);     // created from call to CreateVerticesAndEdgesFiles
   system(rmCommand);
   sprintf(rmCommand,"rm eigenvalue_%lu.txt",uniqNumber);
   system(rmCommand);

   return (eigenvalue);
}


//******************************************************************************
// NAME: CalculateCommunity
//
// INPUTS: (Configuration * configuration) - PLADS configuration
//         (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//         (ULONG uniqNumber) - number assigned to uniquely identify file(s)
//
// RETURN: double - community
//
// PURPOSE: Calculate the community of the specified graph input 
//          file.
//******************************************************************************

double CalculateCommunity(Configuration *configuration, char * entry,
                          char * sourceDirectory, ULONG uniqNumber)
{
   char fullSourceName[FILE_NAME_LEN];
   double community = 0.0;
   FILE *filePtr;

   CreateVerticesAndEdgesFiles(entry, sourceDirectory, uniqNumber);

   //
   // Run community script
   //
   int status;
   char communityCommand[128];
   const char* communityExecutable = configuration->COMMUNITY_EXECUTABLE;
   sprintf(communityCommand, "%s %lu", communityExecutable, uniqNumber);
   status = system(communityCommand);
   if (status == 0)
   {
      //
      // Read results and return community value
      //
      sprintf(fullSourceName,"community_%lu.txt",uniqNumber);
      filePtr = fopen(fullSourceName,"r");
      if (fscanf(filePtr,"%lf",&community) == EOF)
      {
         printf("*** ERROR - unable to retrieve community -- exiting PLADS\n");
         fflush(stdout);
         exit(-1);
      }
   }
   else
   {
      printf("*** ERROR - unable to calculate community -- exiting PLADS\n");
      fflush(stdout);
      exit(-1);
   }

   // Remove temporary files
   char rmCommand[FILE_NAME_LEN];
   sprintf(rmCommand,"rm vertices_%lu.txt",uniqNumber);  // created from call to CreateVerticesAndEdgesFiles
   system(rmCommand);
   sprintf(rmCommand,"rm edges_%lu.txt",uniqNumber);     // created from call to CreateVerticesAndEdgesFiles
   system(rmCommand);
   sprintf(rmCommand,"rm community_%lu.txt",uniqNumber);
   system(rmCommand);

   return (community);
}


//******************************************************************************
// NAME: CalculateTriangles
//
// INPUTS: (Configuration * configuration) - PLADS configuration
//         (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//         (ULONG uniqNumber) - number assigned to uniquely identify file(s)
//
// RETURN: double - triangles
//
// PURPOSE: Calculate the triangles of the specified graph input 
//          file.
//******************************************************************************

double CalculateTriangles(Configuration *configuration, char * entry,
                          char * sourceDirectory, ULONG uniqNumber)
{
   char fullSourceName[FILE_NAME_LEN];
   double triangles = 0.0;
   FILE *filePtr;

   CreateVerticesAndEdgesFiles(entry, sourceDirectory, uniqNumber);

   //
   // Run triangles script
   //
   int status;
   char trianglesCommand[128];
   const char* trianglesExecutable = configuration->TRIADS_EXECUTABLE;
   sprintf(trianglesCommand, "%s %lu", trianglesExecutable, uniqNumber);
   status = system(trianglesCommand);
   if (status == 0)
   {
      //
      // Read results and return triangles value
      //
      sprintf(fullSourceName,"triangles_%lu.txt",uniqNumber);
      filePtr = fopen(fullSourceName,"r");
      if (fscanf(filePtr,"%lf",&triangles) == EOF)
      {
         printf("*** ERROR - unable to retrieve triangles -- exiting PLADS\n");
         fflush(stdout);
         exit(-1);
      }
   }
   else
   {
      printf("*** ERROR - unable to calculate triangles -- exiting PLADS\n");
      fflush(stdout);
      exit(-1);
   }

   // Remove temporary files
   char rmCommand[FILE_NAME_LEN];
   sprintf(rmCommand,"rm vertices_%lu.txt",uniqNumber);  // created from call to CreateVerticesAndEdgesFiles
   system(rmCommand);
   sprintf(rmCommand,"rm edges_%lu.txt",uniqNumber);     // created from call to CreateVerticesAndEdgesFiles
   system(rmCommand);
   sprintf(rmCommand,"rm triangles_%lu.txt",uniqNumber);
   system(rmCommand);

   return (triangles);
}


//******************************************************************************
// NAME: CreateEdgesCSVFile
//
// INPUTS: (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//         (ULONG uniqNumber) - number assigned to uniquely identify file
//
// RETURN: (void)
//
// PURPOSE: Create edges_#.csv file from graph input file.
//******************************************************************************

void CreateEdgesCSVFile(ULONG number, char * entry, char * sourceDirectory)
{
   char fullSourceName[FILE_NAME_LEN];
   char fullEdgeName[FILE_NAME_LEN];
   char label[80];
   char marker[2];
   int v1, v2;
   FILE *graphInputFilePtr;

   fullSourceName[0] = '\0';
   strcpy(fullSourceName,sourceDirectory);
   strcat(fullSourceName,entry);

   graphInputFilePtr = fopen(fullSourceName,"r"); // read mode
   if (graphInputFilePtr == NULL)
   {
      perror("Error opening graph input file -- exiting PLADS.\n");
      exit(-1);
   }

   //
   // Read in graph input file
   //
   // NOTE:  For now, this is done file-based - obviously it would be faster
   //        to do it in memory.
   //
   // Loop over lines, skipping lines starting with "//", until end of file
   FILE *edgeFilePtr;
   sprintf(fullEdgeName,"edges_%lu.csv",number);

   edgeFilePtr = fopen(fullEdgeName,"w");

   while (fscanf(graphInputFilePtr,"%s",marker) != EOF)
   {
      if ((strcmp(marker,"e") == 0) ||
          (strcmp(marker,"d") == 0) ||
          (strcmp(marker,"u") == 0))
      {
         fscanf(graphInputFilePtr,"%d",&v1);
         fscanf(graphInputFilePtr,"%d",&v2);
         fprintf(edgeFilePtr,"%d,%d\n",v1,v2);
         fscanf(graphInputFilePtr,"%s",label);
      }
   }

   fclose(graphInputFilePtr);
   fclose(edgeFilePtr);

   partitionGP = number;  // NOTE:  Setting this is a kludge to fix a memory leak

//printf("... returning partitionGP = %lu\n",partitionGP);
//fflush(stdout);
}


//******************************************************************************
// NAME: CalculateEntropy
//
// INPUTS: (Configuration * configuration) - PLADS configuration
//         (char * entry) - name of file
//         (char * sourceDirectory) - location of file
//         (ULONG uniqNumber) - number assigned to uniquely identify file(s)
//
// RETURN: double - entropy
//
// PURPOSE: Calculate the entropy of the specified graph input 
//          file.
//
// NOTE:  This function assumes that the entropy executable is an R script, and
//        as such, needs to be initiated with the "Rscript" command.
//******************************************************************************

double CalculateEntropy(ULONG uniqNumber, Configuration *configuration, 
                        char * entry, char * sourceDirectory)
{
   double entropy = 0.0;
   char str[15];
   sprintf(str, "%lu", uniqNumber);

   CreateEdgesCSVFile(uniqNumber, entry, sourceDirectory);

   // NOTE:  The following is a kludge to fix a memory leak.
   //        partitionGP is set in CreateEdgesCSVFile - for some
   //        reason, uniqNumber was getting overwritten...
   uniqNumber = partitionGP;

//printf("setting uniqNumber = %lu\n",uniqNumber);
//fflush(stdout);

//printf("Calling entropy script with the following command: %s ...\n",entropyCommand);
//fflush(stdout);

   //
   // Setup entropy script
   //
   int status;
   char entropyCommand[256];
   const char* entropyExecutable = configuration->ENTROPY_EXECUTABLE;

   entropyCommand[0] = '\0';
   strcpy(entropyCommand, "Rscript ");
   strcat(entropyCommand, entropyExecutable);
   strcat(entropyCommand, " ");
   strcat(entropyCommand, str);

//printf("entropyCommand: %s ...\n",entropyCommand);
//fflush(stdout);

   //
   // Run entropy script
   //
   status = system(entropyCommand);

   if (status == 0)
   {
      //
      // Read results and return entropy value
      //
      char fullSourceName[FILE_NAME_LEN];
      FILE *filePtr;
      sprintf(fullSourceName,"entropy_%lu.txt",uniqNumber);
      filePtr = fopen(fullSourceName,"r");
      if (fscanf(filePtr,"%lf",&entropy) == EOF)
      {
         printf("*** ERROR - unable to retrieve entropy -- exiting PLADS\n");
         fflush(stdout);
         exit(-1);
      }
      fclose(filePtr);
   }
   else
   {
      printf("*** ERROR - unable to calculate entropy -- exiting PLADS\n");
      fflush(stdout);
      exit(-1);
   }

   // Remove temporary files
   char rmCommand[FILE_NAME_LEN];
   sprintf(rmCommand,"rm edges_%lu.csv",uniqNumber);     // created from call to CreateEdgesCSVFile
   system(rmCommand);
   sprintf(rmCommand,"rm entropy_%lu.txt",uniqNumber);
   system(rmCommand);

   //return (entropy);
   return (entropy*100);   // to create a number with higher precision for later comparisons
}
