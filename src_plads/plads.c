//***********************************************************
// plads.c
//
// PLADS, version 1.0
//
// Date      Name       Description
// ========  =========  =======================================
// 08/07/17  Eberle     Initial version.
//
//*************************************************************

#include "plads.h"

//*****************************************************************************
// NAME:    ProcessPartitionsInParallel
//
// INPUTS:  (Configuration *configuration) - PLADS configuration
//          (PidInfoList *pidInfoList) - PID information list
//
// RETURN:  void
//
// PURPOSE: Step 1a of the PLADS algorithm:  process initial
//          N partitions in parallel.
//
// OUTSTANDING ISSUES/QUESTIONS:
//  - what if there are not enough initial files to process?
//
//*****************************************************************************

void ProcessPartitionsInParallel(Configuration *configuration)
{
   struct dirent * entry;
   FILE *pidsFilePtr = fopen("pids.txt","a");
   int numPartition;
   char fileName[FILE_NAME_LEN];

   for (numPartition = 1; numPartition <= configuration->NUM_PARTITIONS; ++numPartition)
   {
      // processing is happening too fast - wait for a bit
      // ... especially if there are a lot of files to process when plads starts
      sleep(2);

      switch (fork())
      {
         case 0:  // child process
            // Find oldest file in source data files directory
            //
            // NOTE:  Depending upon how fast files are generated,
            //        many files could have the same timestamp.
            //        In the future, way want to decide on a
            //        standard file-naming convention where the
            //        name indicates an "order to be processed".
            //
            entry = FindOldestFile
                       (configuration->GRAPH_INPUT_FILES_DIR);
            if (entry)
            {
               printf("processing oldest graph input file: %s\n", entry->d_name);
               fflush(stdout);

               // Calculate and store user specified graph property for each of the initial
               // partitions
               //
               // NOTE:  Since this function is only called once (Step 1), only need
               //        to call AddEntryToGPFile; also no need to calculate mean
               //        or standard deviation yet...
               // Calculate user-specified graph property
               double value = 0.0;
               sprintf(fileName,"%s",entry->d_name);
               if ((configuration->CHANGE_DETECTION_APPROACH == 1) ||
                   (configuration->CHANGE_DETECTION_APPROACH == 9))
               {
                  value = CalculateConnectedness(configuration,fileName,
                                                 configuration->GRAPH_INPUT_FILES_DIR,
                                                 numPartition);
                  AddEntryToGPFile(numPartition, value, "gp_connected.txt");
               }
               if ((configuration->CHANGE_DETECTION_APPROACH == 2) ||
                   (configuration->CHANGE_DETECTION_APPROACH == 9))
               {
                  value = CalculateDensity(configuration,fileName,
                                           configuration->GRAPH_INPUT_FILES_DIR);
                  AddEntryToGPFile(numPartition, value, "gp_density.txt");
               }
               if ((configuration->CHANGE_DETECTION_APPROACH == 3) ||
                   (configuration->CHANGE_DETECTION_APPROACH == 9))
               {
                  value = CalculateClusteringCoefficient(configuration,fileName,
                                                         configuration->GRAPH_INPUT_FILES_DIR,
                                                         numPartition);
                  AddEntryToGPFile(numPartition, value, "gp_cluster.txt");
               }
               if ((configuration->CHANGE_DETECTION_APPROACH == 4) ||
                   (configuration->CHANGE_DETECTION_APPROACH == 9))
               {
                  value = CalculateEigenvalue(configuration,fileName,
                                              configuration->GRAPH_INPUT_FILES_DIR,
                                              numPartition);
                  AddEntryToGPFile(numPartition, value, "gp_eigen.txt");
               }
               if ((configuration->CHANGE_DETECTION_APPROACH == 5) ||
                   (configuration->CHANGE_DETECTION_APPROACH == 9))
               {
                  value = CalculateCommunity(configuration,fileName,
                                             configuration->GRAPH_INPUT_FILES_DIR,
                                             numPartition);
                  AddEntryToGPFile(numPartition, value, "gp_community.txt");
               }
               if ((configuration->CHANGE_DETECTION_APPROACH == 6) ||
                   (configuration->CHANGE_DETECTION_APPROACH == 9))
               {
                  value = CalculateTriangles(configuration,fileName,
                                             configuration->GRAPH_INPUT_FILES_DIR,
                                             numPartition);
                  AddEntryToGPFile(numPartition, value, "gp_triangle.txt");
               }
               if ((configuration->CHANGE_DETECTION_APPROACH == 7) ||
                   (configuration->CHANGE_DETECTION_APPROACH == 9))
               {
                  value = CalculateEntropy(numPartition,
                                           configuration,fileName,
                                           configuration->GRAPH_INPUT_FILES_DIR);
                  AddEntryToGPFile(numPartition, value, "gp_entropy.txt");
               }
               pid_t getPID = getpid();
               fprintf(pidsFilePtr,"%d\n",(int)getPID);
               printf("\n   *** getPID = %d for processing graph input file (%s)\n", 
                      (int)getPID,fileName);
               fflush(stdout);
               fclose(pidsFilePtr);
               RunGBADForNormativePatterns(configuration,numPartition,fileName);
            }
         case -1:
            exit(1);
         default:
            break;
      }
   }
}


//*****************************************************************************
// NAME:    FindBestNormativePattern
//
// INPUTS:  (Configuration *configuration) - PLADS configuration
//          (NormativePatternList *normativePatternList) - return list of
//                                                       normative patterns
//
// RETURN:  (ULONG maxScore) - score of most normative pattern
//
// PURPOSE: Determine best normative pattern, P, among NM possibilities.
//
// OUTSTANDING ISSUES/QUESTIONS:
// - What if two different normative patterns have the same score?
//
//*****************************************************************************

ULONG FindBestNormativePattern(Configuration *configuration, 
                               ULONG firstPartition, ULONG lastPartition,
                               NormativePatternList *normativePatternList)
{
   ULONG maxScore = 0;
   ULONG partition, numBestSub, nextPartition, nextNumBestSub;
   ULONG subSize, otherSubSize, subNumInstances, otherSubNumInstances;
   FILE *normFile, *otherNormFile, *maxScoreFile, *bestSubFile;
   char normFileName[FILE_NAME_LEN], otherNormFileName[FILE_NAME_LEN],
        maxScoreFileName[FILE_NAME_LEN];
   char line[COMMAND_LEN];
   BOOLEAN normativePatternExists = FALSE;

   NormativePattern *normativePattern = NULL;

   char graphinputfile[FILE_NAME_LEN];
   char gmCommand[COMMAND_LEN];
   int status;

   // loop over all partitions, finding matches and scoring
   // their "normalousness"

   for (partition = firstPartition; partition <= lastPartition; partition++)
   {
      for (numBestSub = 1; numBestSub <= configuration->NUM_NORMATIVE_PATTERNS; numBestSub++)
      {
         // open normative pattern file
         sprintf(normFileName,"%snorm_%lu_%lu",
                 configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                 partition,
                 numBestSub);
         normFile = fopen(normFileName,"r");
         if (normFile == NULL)
         {
            printf("ERROR: opening file (FindBestNormativePattern) - %s\n",normFileName);
            break;
         }
         normativePatternExists = TRUE;

         // initialize normative pattern
         normativePattern = AllocateNormativePattern(partition,numBestSub);
         normativePattern->partitionNumber = partition;
         normativePattern->bestSubNumber = numBestSub;

         // skip first line (name of graph input file)
         fscanf(normFile,"%s %s",line, normativePattern->graphInputFileName);

         // get size and number of instances of sub to calculate initial score
         fscanf(normFile,"%s %lu %lu\n",line, &subSize, &subNumInstances);
         normativePattern->score = subSize * subNumInstances;

         // Now loop over other partitions to see if any matches to their
         // normative patterns
         for (nextPartition = firstPartition; nextPartition <= lastPartition; nextPartition++)
         {
            for (nextNumBestSub = 1; nextNumBestSub <= configuration->NUM_NORMATIVE_PATTERNS; nextNumBestSub++)
            {
               if (partition != nextPartition)
               {
                  // first, see if the normative pattern going to compare to
                  // exists, before making call to gm tool
                  sprintf(otherNormFileName,"%snorm_%lu_%lu",
                          configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                          nextPartition,nextNumBestSub);
                  otherNormFile = fopen(otherNormFileName,"r");
                  if (otherNormFile == NULL)
                  {
                     // skip comparing as it doesn't exist
                     break;
                  }
                  // build appropriate gm tool command
                  const char* gmExecutable = configuration->GM_EXECUTABLE;
                  sprintf(gmCommand, "%s %snorm_%lu_%lu %snorm_%lu_%lu",
                          gmExecutable,
                          configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                          partition,numBestSub,
                          configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                          nextPartition,nextNumBestSub);
                  status = system(gmCommand);
                  //printf("status from gm call (%s) = %d\n",gmCommand,status);
                  if (status == 0)
                  {
                      // skip first line (name of graph input file)
                      fscanf(otherNormFile,"%s %s",line, graphinputfile);

                      // get size and number of instances of second sub
                      fscanf(otherNormFile,"%s %lu %lu\n",line, &otherSubSize, &otherSubNumInstances);

                      // update score for normative pattern
                      normativePattern->score = normativePattern->score + 
                                                (otherSubSize * otherSubNumInstances);

                      nextNumBestSub = configuration->NUM_NORMATIVE_PATTERNS; // no need to process rest...
                  }
                  fclose(otherNormFile);
               }
            }
         }

         NormativePatternListInsert(normativePattern,normativePatternList);

         // close file
         fclose(normFile);

         // maximum score so far?
         if (normativePattern->score > maxScore)
         {
            maxScore = normativePattern->score;
            sprintf(maxScoreFileName,"%snorm_%lu_%lu",
                    configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                    partition,numBestSub);
         }
      }
   }

   // take substructure with the best normative score and write graph (only) to
   // bestSub.g file
   if (normativePatternExists)
   {
      bestSubFile = fopen("bestSub.g","w");
      if (bestSubFile == NULL)
      {
         printf("ERROR: opening file - bestSub.g -- exiting PLADS (from FindBestNormativePattern)\n");
         exit(1);
      }

      maxScoreFile = fopen(maxScoreFileName,"r");
      if (maxScoreFile == NULL)
      {
         printf("ERROR: opening file - %s -- exiting PLADS (from FindBestNormativePattern)\n",maxScoreFileName);
         exit(1);
      }

      // skip first few lines...
      fgets(line,sizeof(line),maxScoreFile);
      fgets(line,sizeof(line),maxScoreFile);
      // ... print remaining graph
      while (fgets(line,sizeof(line),maxScoreFile) != NULL )
         fputs(line,bestSubFile);

      fclose(bestSubFile);
      fclose(maxScoreFile);
   }

   return maxScore;
}


//*****************************************************************************
// NAME:    FindMostAnomalousSubstructures
//
// INPUTS:  (Configuration *configuration) - PLADS configuration
//          (ULONG firstPartition) - first partition number
//          (int numAnomalousInstances) - number of anomalous instances
//          (AnomalousSubstructureList *anomalousSubstructureList) 
//                         - return list of anomalous substructure(s)
//
// RETURN:  (double minScore) - score of most anomalous substructure(s)
//
// PURPOSE: Step 4 of the PLADS algorithm: Evaluate anomalous substructures 
//          across partitions and report most anomalous substructure(s).
//
//*****************************************************************************

double FindMostAnomalousSubstructures(Configuration *configuration,
                                      ULONG firstPartition,
                                      int numAnomalousInstances,
                                      AnomalousSubstructureList *mostAnomalousSubstructureList)
{
   int status;
   double minScore = DBL_MAX, tempScore = 0.0;
   double nextPartitionScore = 0.0;
   ULONG partition, nextPartition, numAnomSub, nextNumAnomSub;
   char anomFileName[FILE_NAME_LEN], otherAnomFileName[FILE_NAME_LEN];
   char line[128];
   char gmCommand[COMMAND_LEN];
   FILE *anomFile, *otherAnomFile;
   ULONG i,j;

   //
   // NOTE:  These dynamically created arrays should be changed to 
   //        linked lists...
   //

   // dynamically create two-dimensional array of scores
   double **anomScore;
   anomScore = (double **)malloc(sizeof(double *) * (configuration->NUM_PARTITIONS+1));
   for (i = 0; i <= configuration->NUM_PARTITIONS; i++)
      anomScore[i] = (double *)malloc(sizeof(double) * (numAnomalousInstances+1));

   // dynamically create two-dimensional array of flags to keep track
   // of which anomalous partitions and substructures to output
   BOOLEAN **anomSubsSoFar;
   anomSubsSoFar = (BOOLEAN **)malloc(sizeof(BOOLEAN *) * (configuration->NUM_PARTITIONS+1));
   for (i = 0; i <= configuration->NUM_PARTITIONS; i++)
      anomSubsSoFar[i] = (BOOLEAN *)malloc(sizeof(BOOLEAN) * (numAnomalousInstances+1));

   // go through all anomalous substructure files, finding matches and scoring
   // their "anomalousness" across the partitions
   ULONG relativePartition;
   ULONG relativeNextPartition;
   for (partition = 1; partition <= configuration->NUM_PARTITIONS; partition++)
   {
      relativePartition = partition + (firstPartition - 1);
      for (numAnomSub = 1; numAnomSub <= numAnomalousInstances; numAnomSub++)
      {
         // open anomalous instance file
         sprintf(anomFileName,"%sanom_%lu_%lu",
                 configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR,
                 relativePartition,numAnomSub);
         anomFile = fopen(anomFileName,"r");
         // may not be anomalous instances for every partition
         if (anomFile != NULL)
         {
            // reset anomSubsSoFar
            for (i = 0; i <= configuration->NUM_PARTITIONS; i++)
               for (j = 0; j <= numAnomalousInstances; j++)
                  anomSubsSoFar[i][j] = FALSE;

            // read score from first line of file and store
            fscanf(anomFile,"%%%s",line);
            sscanf(line, "%lf", &tempScore);
            anomScore[partition][numAnomSub] = tempScore;

            anomSubsSoFar[partition][numAnomSub] = TRUE;

            // now see if this graph matches other anomalous graphs
            for (nextPartition = 1; nextPartition <= configuration->NUM_PARTITIONS; nextPartition++)
            {
               relativeNextPartition = nextPartition + (firstPartition - 1);
               for (nextNumAnomSub = 1; nextNumAnomSub <= numAnomalousInstances; nextNumAnomSub++)
               {
                  if ((partition != nextPartition) || (numAnomSub != nextNumAnomSub))
                  {
                     // see if the other anomalous instance file exists
                     sprintf(otherAnomFileName,"%sanom_%lu_%lu",
                             configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR,
                             relativeNextPartition,nextNumAnomSub);
                     otherAnomFile = fopen(otherAnomFileName,"r");
                     if (otherAnomFile != NULL)
                     {
                        // read score from first line of file and store
                        fscanf(otherAnomFile,"%%%s",line);
                        sscanf(line, "%lf", &nextPartitionScore);
                        // build appropriate gm tool command
                        const char* gmExecutable = configuration->GM_EXECUTABLE;
                        sprintf(gmCommand, "%s %sanom_%lu_%lu %sanom_%lu_%lu",
                                gmExecutable,
                                configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR,
                                relativePartition,numAnomSub,
                                configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR,
                                relativeNextPartition,nextNumAnomSub);
                        status = system(gmCommand);
                        //printf("status from gm call on anomalous substructures (%s) = %d\n",gmCommand,status);
                        if (status == 0)
                        {
                           // Because the normative pattern is the same for all
                           // runs looking for anomalous substructures, if the
                           // substructures have the same graph structure, their
                           // scores will be the same
                              anomScore[partition][numAnomSub] = anomScore[partition][numAnomSub] +
                                                                 nextPartitionScore;
                        }
                        fclose(otherAnomFile);
                     }
                  }
               }
            }
            // close file
            fclose(anomFile);

            // lowest score so far (i.e., most anomalous)?
            if (anomScore[partition][numAnomSub] <= minScore)
            {
               minScore = anomScore[partition][numAnomSub];

               // create mostAnomalousSubstructureList
               for (i = 1; i <= configuration->NUM_PARTITIONS; i++)
               {
                  for (j = 1; j <= numAnomalousInstances; j++)
                  {
                     if (anomSubsSoFar[i][j])
                     {
                        AnomalousSubstructure *mostAnomalousSubstructure = NULL;
                        mostAnomalousSubstructure = AllocateAnomalousSubstructure((i+(firstPartition-1)),j);

                        mostAnomalousSubstructure->score = minScore;

                        AnomalousSubstructureListInsert(mostAnomalousSubstructure,
                                                        mostAnomalousSubstructureList);
                     }
                  }
               }
            }
         }
      }
   }

   // free memory
   free(anomSubsSoFar);
   free(anomScore);

   return minScore;
}


//*****************************************************************************
// NAME:    RunGBADForNormativePatterns
//
// INPUTS:  (dirent * entry) - file directory and name
//          (ULONG numPartition) - partition number
//          (Configuration *configuration) - PLADS configuration
//
// RETURN:  void
//
// PURPOSE: Run GBAD for discovery of top M normative patterns.
//
// OUTSTANDING ISSUES/QUESTIONS:
// - None.
//*****************************************************************************

void RunGBADForNormativePatterns(Configuration *configuration, 
//                                 ULONG numPartition, struct dirent * entry)
                                 ULONG numPartition, char * fileName)
{
   int status;
   //char fileName[FILE_NAME_LEN];
   char gbadCommand[COMMAND_LEN];

   // Move graph input file to processing area
   //sprintf(fileName,"%s",entry->d_name); 
   printf("Moving graph input file (%s) to processing area...\n", fileName);
   status = MoveFile
               (fileName,
                configuration->GRAPH_INPUT_FILES_DIR,
                configuration->FILES_BEING_PROCESSED_DIR);
   if (status != 0)
   {
      printf("ERROR:  Unable to move file %s from %s to %s - exiting -- exiting PLADS (from RunGBADForNormativePatterns)\n",
             fileName,
             configuration->GRAPH_INPUT_FILES_DIR,
             configuration->FILES_BEING_PROCESSED_DIR);
      fflush(stdout);
      exit(1);
   }

   // build appropriate GBAD command
   const char* gbadExecutable = configuration->GBAD_EXECUTABLE;
   sprintf(gbadCommand, "%s -partition %lu -graph %s -nsubs %i %s %s %s %s %s %s %s %s %s %s%s > %s%s.out", 
           gbadExecutable,
           numPartition,
           fileName,
           configuration->NUM_NORMATIVE_PATTERNS,
           configuration->GBAD_PARAMETER_1,
           configuration->GBAD_PARAMETER_VALUE_1,
           configuration->GBAD_PARAMETER_WITH_NO_VALUE,
           configuration->GBAD_PARAMETER_2,
           configuration->GBAD_PARAMETER_VALUE_2,
           configuration->GBAD_PARAMETER_3,
           configuration->GBAD_PARAMETER_VALUE_3,
           configuration->GBAD_PARAMETER_4,
           configuration->GBAD_PARAMETER_VALUE_4,
           configuration->FILES_BEING_PROCESSED_DIR,
           fileName,
           configuration->OUTPUT_FILES_DIR,
           fileName);

   printf("Executing the following (partition = %lu): %s ...\n",
          numPartition,gbadCommand);
   fflush(stdout);

   FILE *pf;

   // Setup our pipe for reading and executing our command.
   pf = popen(gbadCommand,"r"); 
   if (!pf)
   {
      //fprintf(stderr, "Could not open pipe for call to GBAD.\n");
      //exit(1);
      perror("popen failed -- exiting PLADS (from RunGBADForNormativePatterns)");
      exit(errno);
   }
   if (pclose(pf) != 0)
   {
      //fprintf(stderr," Error: Failed to close GBAD command stream. \n");
      //exit(1);
      perror("pclose failed -- exiting PLADS (from RunGBADForNormativePatterns)");
      exit(errno);
   }
   printf("... finished processing %s\n",fileName);
   fflush(stdout);

   // Move from processing area to processed area
   // for eventual anomaly detection
   printf("moving from processing area to area for eventual anomaly detection...\n\n");
   fflush(stdout);
   status = MoveFile
                     (fileName,
                     configuration->FILES_BEING_PROCESSED_DIR,
                     configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR);
   if (status != 0)
   {
      printf("ERROR:  Unable to move file from %s to %s - exiting PLADS (from ProcessPartitionsForAnomalyDetection)\n",
             configuration->FILES_BEING_PROCESSED_DIR,
             configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR);
      fflush(stdout);
      exit(1);
   }

}


//*****************************************************************************
// NAME:    ProcessPartitionsForAnomalyDetectionInParallel
//
// INPUTS:  TBD
//
// RETURN:  void
//
// PURPOSE: TBD
//
// OUTSTANDING ISSUES/QUESTIONS:
// - None.
//*****************************************************************************
void ProcessPartitionsForAnomalyDetectionInParallel(Configuration *configuration,
                                                    const NormativePatternList *normativePatternList,
                                                    ULONG bestNormScore)
{
   int status;
   char gmCommand[COMMAND_LEN];
   char gbadCommand[COMMAND_LEN];
   char moveFileFrom[FILE_NAME_LEN];
   char moveFileTo[FILE_NAME_LEN];
   NormativePatternListNode *normativePatternListNode;
   ULONG numAnomalousInstances;
   pid_t getPID;

   FILE *pidsFilePtr = fopen("pids.txt","a");
   if (pidsFilePtr == NULL)
   {
      printf("*** ERROR opening pids.txt file -- exiting PLADS (from ProcessPartitionsForAnomalyDetectionInParallel)\n");
      exit(-1);
   }

   if (normativePatternList != NULL)
   {
      normativePatternListNode = normativePatternList->head;
      ULONG currentPartitionNumber = 0;
      while (normativePatternListNode != NULL)
      {
         // Need to determine which best substructure for this graph input
         // file matches the best substructure from among all of the
         // partitions in the current "window".
         const char* gmExecutable = configuration->GM_EXECUTABLE;
         sprintf(gmCommand, "%s bestSub.g %snorm_%lu_%lu",
                 gmExecutable,
                 configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                 normativePatternListNode->normativePattern->partitionNumber,
                 normativePatternListNode->normativePattern->bestSubNumber);
         status = system(gmCommand);
         if ((currentPartitionNumber != normativePatternListNode->normativePattern->partitionNumber) && (status == 0))
         {  
            currentPartitionNumber = normativePatternListNode->normativePattern->partitionNumber;
            // processing is happening too fast - wait for 3 seconds
            // NOTE: may want to set up the second value below as a configuration setting in plads.cfg
            sleep(3);

            switch (fork())
            {
               case 0:  // child process
                  //
                  getPID = getpid();
                  fprintf(pidsFilePtr,"%d\n",(int)getPID);
                  printf("\n   *** getPID = %d for performing anomaly detection (using normative pattern %lu and partition %lu) on the following graph input file:  %s\n", 
                         (int)getPID,
                         normativePatternListNode->normativePattern->bestSubNumber,
                         normativePatternListNode->normativePattern->partitionNumber,
                         normativePatternListNode->normativePattern->graphInputFileName);
                  fflush(stdout);
                  fclose(pidsFilePtr);

                  //
                  // Move previously processed graph input file back to
                  // directory for processing
                  //
                  sprintf(moveFileFrom,"%s%s",configuration->PROCESSED_INPUT_FILES_DIR,
                          normativePatternListNode->normativePattern->graphInputFileName);
                  sprintf(moveFileTo,"%s%s",configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR,
                          normativePatternListNode->normativePattern->graphInputFileName);
                  rename(moveFileFrom,moveFileTo);

                  // build appropriate GBAD command
                  const char* gbadExecutable = configuration->GBAD_EXECUTABLE;
                  sprintf(gbadCommand, "%s -norm %lu -partition %lu -plads %s %f -nsubs %i %s %s %s %s %s %s %s %s %s %s%s > %sgbadOutput_%lu.out",
                          gbadExecutable,
                          normativePatternListNode->normativePattern->bestSubNumber,
                          normativePatternListNode->normativePattern->partitionNumber,
                          configuration->GBAD_ALGORITHM,
                          configuration->GBAD_THRESHOLD,
                          configuration->NUM_NORMATIVE_PATTERNS,
                          configuration->GBAD_PARAMETER_1,
                          configuration->GBAD_PARAMETER_VALUE_1,
                          configuration->GBAD_PARAMETER_WITH_NO_VALUE,
                          configuration->GBAD_PARAMETER_2,
                          configuration->GBAD_PARAMETER_VALUE_2,
                          configuration->GBAD_PARAMETER_3,
                          configuration->GBAD_PARAMETER_VALUE_3,
                          configuration->GBAD_PARAMETER_4,
                          configuration->GBAD_PARAMETER_VALUE_4,
                          configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR,
                          normativePatternListNode->normativePattern->graphInputFileName,
                          configuration->ANOMALOUS_OUTPUT_FILES_DIR,
                          normativePatternListNode->normativePattern->partitionNumber);
                  printf("Performing anomaly detection using the following command: %s ...\n",
                         gbadCommand);
                  fflush(stdout);
                  status = system(gbadCommand);
                  if (status != 0)
                  {
                     printf("ERROR:  Unable to execute GBAD ... exiting PLADS (from ProcessPartitionsForAnomalyDetection)\n");
                     exit(1);
                  }
                  printf("... finished anomaly detection on %s\n",normativePatternListNode->normativePattern->graphInputFileName);
                  fflush(stdout);

                  // Move input graph file from processing area to processed area
                  sprintf(moveFileFrom,"%s%s",configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR,
                          normativePatternListNode->normativePattern->graphInputFileName);
                  sprintf(moveFileTo,"%s%s",configuration->PROCESSED_INPUT_FILES_DIR,
                          normativePatternListNode->normativePattern->graphInputFileName);
                  rename(moveFileFrom,moveFileTo);
                  FILE *numAnomFilePtr = fopen("numanom.txt","r");
                  if (numAnomFilePtr == NULL)
                  {
                     numAnomalousInstances = 0;
                     printf("   Creating numanom.txt ...\n");
                     fflush(stdout);
                  }
                  else
                  {
                     fscanf(numAnomFilePtr,"%lu",&numAnomalousInstances);
                     fclose(numAnomFilePtr);
                     remove("numanom.txt");
                  }
                  numAnomalousInstances = numAnomalousInstances +
                                          MoveAnomalousSubstructureFiles(normativePatternListNode->normativePattern->partitionNumber,
                                                                         configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR);
                  numAnomFilePtr = fopen("numanom.txt","a");
                  fprintf(numAnomFilePtr,"%lu\n",numAnomalousInstances);
                  fclose(numAnomFilePtr);
               case -1:
                  //printf("\nERROR:  Unable to procur PID for process\n");
                  //printf("... exiting\n");
                  exit(1);
               default:
                  break;
                  //printf("\nwaiting for processes to complete...\n");
            }
         }
         normativePatternListNode = normativePatternListNode->next;
      }
   }
   else
   {
      printf("ERROR:  no normative patterns - exiting PLADS (from ProcessPartitionsForAnomalyDetection).\n");
      exit(1);
   }
}


//
// Loop through PIDs of running processes until all processes have
// completed processing.
//
void WaitingForProcessesToFinish(Configuration *configuration)
{
   PidInfoList *pidInfoList = NULL;
   pidInfoList = AllocatePidInfoList();
   FILE *pidsFilePtr;
   int pid;

   pidsFilePtr = fopen("pids.txt","r");
   if (pidsFilePtr == NULL)
   {
      perror("Error opening the pids.txt file. -- exiting PLADS (from WaitingForProcessesToFinish)\n");
      exit(-1);
   }
   else
   {
      printf("Opening pids.txt file...\n");
      fflush(stdout);
      while (fscanf(pidsFilePtr,"%d",&pid) != EOF)
      {
         PidInfo *pidInfo = AllocatePidInfo(pid);
         PidInfoListInsert(pidInfo, pidInfoList);
      }
      remove("pids.txt");
      fclose(pidsFilePtr);
   }
   if (pidInfoList != NULL)
   {
      PidInfoListNode *pidInfoListNode = pidInfoList->head;
      ULONG numProcessesRunning = configuration->NUM_PARTITIONS;
      pid_t endID;
      int status;
      while ((numProcessesRunning > 0) && (pidInfoListNode != NULL))
      {
         endID = waitpid(pidInfoListNode->pidInfo->PID, &status, WNOHANG|WUNTRACED);
         if (endID == -1)
         {
            perror("waitpid error");
            fflush(stdout);
            exit(EXIT_FAILURE);
         }
         else if (endID == 0)
         {
            // child process still running
         }
         else if (endID == pidInfoListNode->pidInfo->PID)   // process ended
         {
            printf("*** PROCESS %d HAS FINISHED ***\n",pidInfoListNode->pidInfo->PID);
            fflush(stdout);
            PidInfoListDelete(pidInfoListNode->pidInfo->PID,pidInfoList);
            numProcessesRunning--;
         }
         sleep(1);   // wait a second - to keep it from "spinning"...
         pidInfoListNode = pidInfoListNode->next;
         if (pidInfoListNode == NULL)
            pidInfoListNode = pidInfoList->head;  // loop back through if not done...
      }
   }
   if (pidInfoList != NULL)
      FreePidInfoList(pidInfoList);
}
