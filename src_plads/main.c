//***********************************************************
// main.c
//
// PLADS, version 1.0
//
// Date      Name       Description
// ========  =========  =======================================
// 08/07/17  Eberle     Initial version.
//
//*************************************************************

#include "plads.h"

// Function prototypes

int main(int, char **);
Parameters *GetParameters(int, char **);
void PrintParameters(Parameters *);
Configuration *GetConfiguration();
void PrintConfiguration(Configuration *);

//***********************************************************
// NAME:    main
//
// INPUTS:  (int argc) - number of arguments to program
//          (char **argv) - array of strings of arguments to 
//          program
//
// RETURN:  (int) - 0 if all is well
//
// PURPOSE: Main PLADS function that processes command-line 
//          arguments, reads in any configuration settings, 
//          and controls the entire PLADS process.
//***********************************************************

int main(int argc, char **argv)
{
   Parameters *parameters;
   Configuration *configuration;
   struct dirent * entry;
   time_t stepStartTime;
   time_t stepEndTime;
   char fileName[FILE_NAME_LEN];
   FILE *changeDetectionTimeFile;
   struct timeval t0;
   struct timeval t1;
   float currentChangeDetectionTime;
   long int changeDetectionTotalTime = 0;

   printf("PLADS running ...\n\n\n");
   fflush(stdout);

   // Read command-line parameters
   parameters = GetParameters(argc, argv);
   PrintParameters(parameters);

   // Read configuration file
   configuration = GetConfiguration();
   PrintConfiguration(configuration);

   ULONG firstPartition = 1;
   ULONG lastPartition = configuration->NUM_PARTITIONS;

   ////////////////////////////////////////////////////////////////////////////
   //
   // Step 1:  Process N partitions in parallel.
   //
   ////////////////////////////////////////////////////////////////////////////

   time_t fullStartTime;
   time_t fullEndTime;

   printf("\n************************************************\n");
   printf("\n\nStep 1:  Process N partitions in parallel...\n\n");
   printf("\n************************************************\n");
   fflush(stdout);

   fullStartTime = time(NULL);
   stepStartTime = time(NULL);

   //
   // Step 1a:  Each partition discovers top M normative patterns
   //

   ProcessPartitionsInParallel(configuration);

   //
   // Step 1b:  Each partition waits for all partitions to 
   //           discover their normative patterns

   printf("\n\nWaiting for initial N partitions to START processing...\n\n");
   fflush(stdout);
   sleep(configuration->NUM_PARTITIONS);   // NOTE:  This wait value is arbitrary

   WaitingForProcessesToFinish(configuration);

   stepEndTime = time(NULL);
   printf("\nStep 1: (elapsed CPU time = %lu seconds)\n",
          (stepEndTime - stepStartTime));
   fflush(stdout);

   ////////////////////////////////////////////////////////////////////////////
   //
   // Step 2:  Determine best normative pattern, P, among NM 
   //          possibilities
   //
   ////////////////////////////////////////////////////////////////////////////

   printf("\n************************************************\n");
   printf("\n\nStep 2:  Determine best normative pattern among all partitions...\n\n");
   printf("\n************************************************\n");
   fflush(stdout);
   
   char moveFileFrom[FILE_NAME_LEN];
   char moveFileTo[FILE_NAME_LEN];
   ULONG partition, numBestSub;
   ULONG normScore = 0;

   //
   // Save the norm_#_# files that have been processed
   //
   //for (partition = 1; partition <= configuration->NUM_PARTITIONS; partition++)
   for (partition = firstPartition; partition <= lastPartition; partition++)
   {
      for (numBestSub = 1; numBestSub <= configuration->NUM_NORMATIVE_PATTERNS; numBestSub++)
      {
          // Move normative substructure files
          sprintf(moveFileFrom,"./norm_%lu_%lu",partition,numBestSub);
          sprintf(moveFileTo,"%snorm_%lu_%lu",configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                  partition,numBestSub);
          rename(moveFileFrom,moveFileTo);
      }
   }

   stepStartTime = time(NULL);

   // Initialize list of normative patterns
   NormativePatternList *normativePatternList = NULL;
   normativePatternList = AllocateNormativePatternList();

   normScore = FindBestNormativePattern(configuration,
                                        firstPartition,
                                        lastPartition,
                                        normativePatternList);

   //
   // Output best normative pattern at this point
   //
   // NOTE:  This assumes bestSub.g exists - if it doesn't, something bad
   //        happened above OR there were no normative patterns in any
   //        of the N runs...
   //
   printf("Normative Pattern:\n\n");
   char buff[256];
   FILE *fp = fopen( "bestSub.g", "r" );
   while ( fgets( buff, 256, fp ) != NULL )
      fputs( buff, stdout );
   fclose( fp );
   printf("\n");

   stepEndTime = time(NULL);
   printf("\nStep 2: (elapsed CPU time = %lu seconds)\n",
          (stepEndTime - stepStartTime));
   fflush(stdout);

   ////////////////////////////////////////////////////////////////////////////
   //
   // Step 3:  Each partition discovers anomalous substructures based upon P
   //
   ////////////////////////////////////////////////////////////////////////////

   printf("\n************************************************\n");
   printf("\n\nStep 3:  Each partition discovers anomalous substructures based upon P...\n\n");
   printf("\n************************************************\n");
   fflush(stdout);

   stepStartTime = time(NULL);

   ProcessPartitionsForAnomalyDetectionInParallel(configuration,
                                                  normativePatternList,
                                                  normScore);
   WaitingForProcessesToFinish(configuration);

   // get number of anomalies...
   ULONG numAnomalousInstances = 0;
   FILE *numAnomaliesFilePtr = fopen("numanom.txt","r");
   if (numAnomaliesFilePtr == NULL)
   {
      perror("Error opening the numanom.txt file -- no anomalies exist at this point (Step 3).\n");
      //exit(-1);
   }
   else
   {
      printf("Opening numanom.txt file...\n");
      fscanf(numAnomaliesFilePtr,"%lu",&numAnomalousInstances);
      fclose(numAnomaliesFilePtr);
      remove("numanom.txt");
   }

   FreeNormativePatternList(normativePatternList);

   // Move best substructure file to holding place
   sprintf(moveFileFrom,"./bestSub.g");
   sprintf(moveFileTo,"%sbestSub.g",configuration->BEST_NORMATIVE_PATTERN_DIR);
   rename(moveFileFrom,moveFileTo);

   stepEndTime = time(NULL);
   printf("\nStep 3: (elapsed CPU time = %lu seconds)\n",
          (stepEndTime - stepStartTime));
   fflush(stdout);


   ////////////////////////////////////////////////////////////////////////////
   //
   // Step 4:  Evaluate anomalous substructures across partitions and report 
   //          most anomalous substructure(s).
   //
   ////////////////////////////////////////////////////////////////////////////

   printf("\n************************************************\n");
   printf("\n\nStep 4:  Evaluating %lu anomalous substructures across partitions and report most anomalous substructure(s)...\n\n",numAnomalousInstances);
   printf("\n************************************************\n");
   fflush(stdout);
   
   stepStartTime = time(NULL);

   // Initialize list of anomalous substructures
   AnomalousSubstructureList *mostAnomalousSubstructureList = NULL;
   mostAnomalousSubstructureList = AllocateAnomalousSubstructureList();

   double anomScore = DBL_MAX;

   anomScore = FindMostAnomalousSubstructures(configuration, 
                                              1,
                                              numAnomalousInstances,
                                              mostAnomalousSubstructureList);

   //
   // Output most anomalous substructure(s) at this point
   //
   if (mostAnomalousSubstructureList != NULL)
   {
      AnomalousSubstructureListNode *mostAnomalousSubstructureListNode = NULL;
      char anomSubFileName[FILE_NAME_LEN];
      char buff[256];
      mostAnomalousSubstructureListNode = mostAnomalousSubstructureList->head;
      if (mostAnomalousSubstructureListNode != NULL)
         printf("Most Anomalous Substructures (Step 4):\n\n");
      int countAnomSubs = 0;
      while (mostAnomalousSubstructureListNode != NULL)
      {
         // The reason for the following unusual comparison is that there is
         // not a good way to compare two floating type variables.  I am
         // using this for now, BUT, if the precision needs to go out beyond
         // 6 digits to the right, this will not work.
         if (fabs(mostAnomalousSubstructureListNode->anomalousSubstructure->score - anomScore) < 0.000001)
         {
            printf("(partition %lu and anomalous number %lu)\n",
                   mostAnomalousSubstructureListNode->anomalousSubstructure->partitionNumber,
                   mostAnomalousSubstructureListNode->anomalousSubstructure->anomalousNumber);
            sprintf(anomSubFileName,"%sanomInst_%lu_%lu",
                    configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR,
                    mostAnomalousSubstructureListNode->anomalousSubstructure->partitionNumber,
                    mostAnomalousSubstructureListNode->anomalousSubstructure->anomalousNumber);
            FILE *fp = fopen(anomSubFileName,"r");
            while ( fgets( buff, 256, fp ) != NULL )
               fputs( buff, stdout );
            fclose( fp );
            printf("\n");
            countAnomSubs++;
         }
         mostAnomalousSubstructureListNode = mostAnomalousSubstructureListNode->next;
      }
      if (countAnomSubs > 0)
      {
         printf("\n(Number of anomalous substructures reported: %d)\n",countAnomSubs);
         fflush(stdout);
      }
      else
      {
         printf("No anomalous substructures to report.\n");
         fflush(stdout);
      }
   }
   else
   {
      printf("No anomalous substructures to report.\n");
      fflush(stdout);
   }
   printf("\n");

   stepEndTime = time(NULL);
   printf("\nStep 4: elapsed CPU time = %lu seconds)\n",
          (stepEndTime - stepStartTime));
   fflush(stdout);

   ////////////////////////////////////////////////////////////////////////////
   //
   // Step 5:  Process new partition...
   //
   ////////////////////////////////////////////////////////////////////////////

   ULONG oldestPartition = firstPartition;
   ULONG currentPartition = lastPartition;
   int status;
   char line[128];
   char gmCommand[COMMAND_LEN];
   char gbadCommand[COMMAND_LEN];
   char normFileName[FILE_NAME_LEN];
   char graphInputFileName[FILE_NAME_LEN];
   FILE *normFile;

   double value = 0.0;
   double averageGP = 0.0;
   double stddevGP = 0.0;
   double valueConnected = 0.0;
   double valueDensity = 0.0;
   double valueCluster = 0.0;
   double valueEigen = 0.0;
   double valueCommunity = 0.0;
   double valueTriangle = 0.0;
   double valueEntropy = 0.0;

   double averageGPConnected = 0.0;
   double averageGPDensity = 0.0;
   double averageGPCluster = 0.0;
   double averageGPEigen = 0.0;
   double averageGPCommunity = 0.0;
   double averageGPTriangle = 0.0;
   double averageGPEntropy = 0.0;

   double stddevGPConnected = 0.0;
   double stddevGPDensity = 0.0;
   double stddevGPCluster = 0.0;
   double stddevGPEigen = 0.0;
   double stddevGPCommunity = 0.0;
   double stddevGPTriangle = 0.0;
   double stddevGPEntropy = 0.0;

   // Loop (indefinitely)
   while (TRUE)
   {

      //
      // Step 5a:  Remove older partition(s) from further processing.
      //
      // NOTE:  For now, not checking date of graph input files - just
      //        sliding the "partition numbers" (i.e., if there is another
      //        graph input file, exclude the one at the beginning of the
      //        partition list from being considered).
      //
      entry = FindOldestFile
                 (configuration->GRAPH_INPUT_FILES_DIR);
      if (entry)
      {
         printf("next (oldest) graph input file to process: %s\n", entry->d_name);
         fflush(stdout);

         oldestPartition++;
         currentPartition++;

         // If there are anomalous and normative substructure files that are no longer
         // needed, remove them from their holding directories
         //
         // (NOTE:  This is needed because in Unix there is a limit to how many
         //         files can be removed, moved, or copied from a directory.)
         //
         RemoveNoLongerNeededFiles(configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR,
                                   (oldestPartition-1),
                                   (oldestPartition-1));
         RemoveNoLongerNeededFiles(configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                                   (oldestPartition-1),
                                   (oldestPartition-1));

         printf("\n************************************************\n");
         printf("\n\nStep 5:  Process new partition (%lu) for entry = %s ...\n\n",
                currentPartition,entry->d_name);
         printf("\n************************************************\n");
         fflush(stdout);

         stepStartTime = time(NULL);

         sprintf(fileName,"%s",entry->d_name);

         // Calculate user-specified graph property
         value = 0.0;
         averageGP = 0.0;
         stddevGP = 0.0;
         valueConnected = 0.0;
         valueDensity = 0.0;
         valueCluster = 0.0;
         valueEigen = 0.0;
         valueCommunity = 0.0;
         valueTriangle = 0.0;
         valueEntropy = 0.0;
        
         averageGPConnected = 0.0;
         averageGPDensity = 0.0;
         averageGPCluster = 0.0;
         averageGPEigen = 0.0;
         averageGPCommunity = 0.0;
         averageGPTriangle = 0.0;
         averageGPEntropy = 0.0;

         stddevGPConnected = 0.0;
         stddevGPDensity = 0.0;
         stddevGPCluster = 0.0;
         stddevGPEigen = 0.0;
         stddevGPCommunity = 0.0;
         stddevGPTriangle = 0.0;
         stddevGPEntropy = 0.0;

         if (configuration->CHANGE_DETECTION_APPROACH > 0)
         {
            changeDetectionTimeFile = fopen("changeDetectionValue.txt","a+");
            if (changeDetectionTimeFile != NULL)
               fscanf(changeDetectionTimeFile,"%ld", &changeDetectionTotalTime);
            else
               changeDetectionTotalTime = 0;
            fclose(changeDetectionTimeFile);
            gettimeofday(&t0, 0);
            //
            // Step 5b:  Calculate and store graph property metric GP' for new partition.
            //
            printf("\n************************************************\n");
            printf("\n\nStep 5b:  Calculate and store graph property metric GP' for new partition (%lu) for entry = %s ...\n\n",
                   currentPartition,entry->d_name);
            printf("\n************************************************\n");
            fflush(stdout);
         }

         if (configuration->CHANGE_DETECTION_APPROACH == 1 || configuration->CHANGE_DETECTION_APPROACH == 9)
         {
            valueConnected = CalculateConnectedness(configuration,fileName,
                                                    configuration->GRAPH_INPUT_FILES_DIR,
                                                    currentPartition);
            printf("--- connectedness (value) for partition %lu (%s) = %.17g\n",
                   currentPartition,fileName,valueConnected);

            UpdateGPFile(configuration, currentPartition, valueConnected,"gp_connected.txt");

            averageGPConnected = CalculateMeanFromGPFile("gp_connected.txt");
            printf("At partition %lu for fileName = %s: GP average (connected) = %.17g\n\n",
                   currentPartition,fileName,averageGPConnected);
            fflush(stdout);
   
            stddevGPConnected = CalculateStandardDeviationFromGPFile(averageGPConnected,"gp_connected.txt");
            printf("At partition %lu for fileName = %s: GP standard deviation (connected) = %.17g\n",
                   currentPartition,fileName,stddevGPConnected);
            fflush(stdout);

            // save for situation where this is the only metric used... 
            value = valueConnected;
            averageGP = averageGPConnected;
            stddevGP = stddevGPConnected;
         }
         
         if (configuration->CHANGE_DETECTION_APPROACH == 2 || configuration->CHANGE_DETECTION_APPROACH == 9)
         {
            valueDensity = CalculateDensity(configuration,fileName,
                                            configuration->GRAPH_INPUT_FILES_DIR);
            printf("--- density (value) for partition %lu (%s) = %.17g\n",
                   currentPartition,fileName,valueDensity);

            UpdateGPFile(configuration, currentPartition, valueDensity,"gp_density.txt");

            averageGPDensity = CalculateMeanFromGPFile("gp_density.txt");
            printf("At partition %lu for fileName = %s: GP average (density) = %.17g\n\n",
                   currentPartition,fileName,averageGPDensity);
            fflush(stdout);
   
            stddevGPDensity = CalculateStandardDeviationFromGPFile(averageGPDensity,"gp_density.txt");
            printf("At partition %lu for fileName = %s: GP standard deviation (density) = %.17g\n",
                   currentPartition,fileName,stddevGPDensity);
            fflush(stdout);

            // save for situation where this is the only metric used... 
            value = valueDensity;
            averageGP = averageGPDensity;
            stddevGP = stddevGPDensity;
         }
         
         if (configuration->CHANGE_DETECTION_APPROACH == 3 || configuration->CHANGE_DETECTION_APPROACH == 9)
         {
            valueCluster = CalculateClusteringCoefficient(configuration,fileName,
                                                          configuration->GRAPH_INPUT_FILES_DIR,
                                                          currentPartition);
            printf("--- clustering coefficient (value) for partition %lu (%s) = %.17g\n",
                   currentPartition,fileName,valueCluster);

            UpdateGPFile(configuration, currentPartition, valueCluster,"gp_cluster.txt");

            averageGPCluster= CalculateMeanFromGPFile("gp_cluster.txt");
            printf("At partition %lu for fileName = %s: GP average (cluster) = %.17g\n\n",
                   currentPartition,fileName,averageGPCluster);
            fflush(stdout);
   
            stddevGPCluster= CalculateStandardDeviationFromGPFile(averageGPCluster,"gp_cluster.txt");
            printf("At partition %lu for fileName = %s: GP standard deviation (cluster) = %.17g\n",
                   currentPartition,fileName,stddevGPCluster);
            fflush(stdout);
          
            // save for situation where this is the only metric used... 
            value = valueCluster;
            averageGP = averageGPCluster;
            stddevGP = stddevGPCluster;
         }
         
         if (configuration->CHANGE_DETECTION_APPROACH == 4 || configuration->CHANGE_DETECTION_APPROACH == 9)
         {
            valueEigen = CalculateEigenvalue(configuration,fileName,
                                             configuration->GRAPH_INPUT_FILES_DIR,
                                             currentPartition);
            printf("--- eigenvalue (value) for partition %lu (%s) = %.17g\n",
                   currentPartition,fileName,valueEigen);

            UpdateGPFile(configuration, currentPartition, valueEigen, "gp_eigen.txt");

            averageGPEigen = CalculateMeanFromGPFile("gp_eigen.txt");
            printf("At partition %lu for fileName = %s: GP average (eigen) = %.17g\n\n",
                   currentPartition,fileName,averageGPEigen);
            fflush(stdout);
   
            stddevGPEigen= CalculateStandardDeviationFromGPFile(averageGPEigen,"gp_eigen.txt");
            printf("At partition %lu for fileName = %s: GP standard deviation (eigen) = %.17g\n",
                   currentPartition,fileName,stddevGPEigen);
            fflush(stdout);
          
            // save for situation where this is the only metric used... 
            value = valueEigen;
            averageGP = averageGPEigen;
            stddevGP = stddevGPEigen;
         }
         
         if (configuration->CHANGE_DETECTION_APPROACH == 5 || configuration->CHANGE_DETECTION_APPROACH == 9)
         {
            valueCommunity = CalculateCommunity(configuration,fileName,
                                                configuration->GRAPH_INPUT_FILES_DIR,
                                                currentPartition);
            printf("--- community (value) for partition %lu (%s) = %.17g\n",
                   currentPartition,fileName,valueCommunity);

            UpdateGPFile(configuration, currentPartition, valueCommunity, "gp_community.txt");

            averageGPCommunity = CalculateMeanFromGPFile("gp_community.txt");
            printf("At partition %lu for fileName = %s: GP average (community) = %.17g\n\n",
                   currentPartition,fileName,averageGPCommunity);
            fflush(stdout);
   
            stddevGPCommunity= CalculateStandardDeviationFromGPFile(averageGPCommunity,"gp_community.txt");
            printf("At partition %lu for fileName = %s: GP standard deviation (community) = %.17g\n",
                   currentPartition,fileName,stddevGPCommunity);
            fflush(stdout);
          
            // save for situation where this is the only metric used... 
            value = valueCommunity;
            averageGP = averageGPCommunity;
            stddevGP = stddevGPCommunity;
         }
         
         if (configuration->CHANGE_DETECTION_APPROACH == 6 || configuration->CHANGE_DETECTION_APPROACH == 9)
         {
            valueTriangle = CalculateTriangles(configuration,fileName,
                                               configuration->GRAPH_INPUT_FILES_DIR,
                                               currentPartition);
            printf("--- triangles (value) for partition %lu (%s) = %.17g\n",
                   currentPartition,fileName,valueTriangle);

            UpdateGPFile(configuration, currentPartition, valueTriangle, "gp_triangle.txt");

            averageGPTriangle = CalculateMeanFromGPFile("gp_triangle.txt");
            printf("At partition %lu for fileName = %s: GP average (triangle) = %.17g\n\n",
                   currentPartition,fileName,averageGPTriangle);
            fflush(stdout);
   
            stddevGPTriangle= CalculateStandardDeviationFromGPFile(averageGPTriangle,"gp_triangle.txt");
            printf("At partition %lu for fileName = %s: GP standard deviation (triangle) = %.17g\n",
                   currentPartition,fileName,stddevGPTriangle);
            fflush(stdout);
          
            // save for situation where this is the only metric used... 
            value = valueTriangle;
            averageGP = averageGPTriangle;
            stddevGP = stddevGPTriangle;
         }
         
         if (configuration->CHANGE_DETECTION_APPROACH == 7 || configuration->CHANGE_DETECTION_APPROACH == 9)
         {
            valueEntropy = CalculateEntropy(currentPartition,
                                            configuration,fileName,
                                            configuration->GRAPH_INPUT_FILES_DIR);
            printf("--- entropy (value) for partition %lu (%s) = %.17g\n",
                   currentPartition,fileName,valueEntropy);

            UpdateGPFile(configuration, currentPartition, valueEntropy, "gp_entropy.txt");

            averageGPEntropy = CalculateMeanFromGPFile("gp_entropy.txt");
            printf("At partition %lu for fileName = %s: GP average (entropy) = %.17g\n\n",
                   currentPartition,fileName,averageGPEntropy);
            fflush(stdout);
   
            stddevGPEntropy= CalculateStandardDeviationFromGPFile(averageGPEntropy,"gp_entropy.txt");
            printf("At partition %lu for fileName = %s: GP standard deviation (entropy) = %.17g\n",
                   currentPartition,fileName,stddevGPEntropy);
            fflush(stdout);
          
            // save for situation where this is the only metric used... 
            value = valueEntropy;
            averageGP = averageGPEntropy;
            stddevGP = stddevGPEntropy;
         }
         
         if (configuration->CHANGE_DETECTION_APPROACH > 0)
         {
            //
            // Step 5c:  Calculate mean and standard deviation based on graph property metrics in current window
            //

            printf("\n************************************************\n");
            printf("\n\nStep 5c:  Calculate mean and standard deviation based on graph property metrics in current window for fileName = %s\n",
                   fileName);
            printf("\n************************************************\n");
            fflush(stdout);

            gettimeofday(&t1, 0);
            currentChangeDetectionTime = ((t1.tv_sec * 1000000) + t1.tv_usec) - ((t0.tv_sec * 1000000) + t0.tv_usec);
            changeDetectionTimeFile = fopen("changeDetectionValue.txt","w");
            changeDetectionTotalTime = changeDetectionTotalTime + currentChangeDetectionTime;
            fprintf(changeDetectionTimeFile,"%ld",changeDetectionTotalTime);
            fclose(changeDetectionTimeFile);
            printf("changeDetectionTotalTime (in microseconds) = %ld\n",changeDetectionTotalTime);
            fflush(stdout);
         }
   
         ULONG numberMetricsTooHigh = 0;
         // All metrics
         if (configuration->CHANGE_DETECTION_APPROACH == 9)
         {
            if ((valueConnected - averageGPConnected) > stddevGPConnected)
            {
               numberMetricsTooHigh++;
               printf("         ... connected metric deviation is too high\n");
               fflush(stdout);
            }
            if ((valueDensity - averageGPDensity) > stddevGPDensity)
            {
               numberMetricsTooHigh++;
               printf("         ... density metric deviation is too high\n");
               fflush(stdout);
            }
            if ((valueCluster - averageGPCluster) > stddevGPCluster)
            {
               numberMetricsTooHigh++;
               printf("         ... cluster metric deviation is too high\n");
               fflush(stdout);
            }
            if ((valueEigen - averageGPEigen) > stddevGPEigen)
            {
               numberMetricsTooHigh++;
               printf("         ... eigen metric deviation is too high\n");
               fflush(stdout);
            }
            if ((valueCommunity - averageGPCommunity) > stddevGPCommunity)
            {
               numberMetricsTooHigh++;
               printf("         ... community metric deviation is too high\n");
               fflush(stdout);
            }
            if ((valueTriangle - averageGPTriangle) > stddevGPTriangle)
            {
               numberMetricsTooHigh++;
               printf("         ... triangle metric deviation is too high\n");
               fflush(stdout);
            }
            if ((valueEntropy - averageGPEntropy) > stddevGPEntropy)
            {
               numberMetricsTooHigh++;
               printf("         ... entropy metric deviation is too high\n");
               fflush(stdout);
            }
         }
         // Single metrics
         if ((configuration->CHANGE_DETECTION_APPROACH > 0) && (configuration->CHANGE_DETECTION_APPROACH < 8))
         {
            if ((value - averageGP) > stddevGP)
               numberMetricsTooHigh = 4;  // set to value that will cause re-evaluation
                                          // of normative pattern (below)
         }

         if ((numberMetricsTooHigh >= configuration->THRESHOLD_FOR_NUM_EXCEEDED_METRICS) || 
             (configuration->CHANGE_DETECTION_APPROACH == 0))
         {
            //printf("*** Deviation is too high - need to discover the normative pattern for partition %lu ***\n",currentPartition);
            //fflush(stdout);
            //
            // Step 5d:  |GP' - mean| > standard deviation OR no change detection approach specified...
            //

            printf("\n************************************************\n");
            printf("\n\nStep 5d:  |GP' - mean| > standard deviation OR no change detection approach specified\n");
            printf("\n************************************************\n");
            fflush(stdout);

            //
            // Step 5d(i):  Discover top M normative patterns from new partition.
            //

            printf("\n************************************************\n");
            printf("\n\nStep 5d(i):  Discover top M normative patterns from new partition (%lu) for fileName = %s...\n\n",
                   currentPartition,fileName);
            printf("\n************************************************\n");
            fflush(stdout);

            RunGBADForNormativePatterns(configuration,currentPartition,fileName);

            //
            // Step 5d(ii):  Determine best normative pattern, P', among all active 
            //               partitions.
            //
            // First, save the norm_#_# files that have been created
            //

            printf("\n************************************************\n");
            printf("\n\nStep 5d(ii):  Determine best normative pattern P'...\n\n");
            printf("\n************************************************\n");
            fflush(stdout);

            for (numBestSub = 1; numBestSub <= configuration->NUM_NORMATIVE_PATTERNS; numBestSub++)
            {
                // Move normative substructure files
                sprintf(moveFileFrom,"./norm_%lu_%lu",currentPartition,numBestSub);
                sprintf(moveFileTo,"%snorm_%lu_%lu",configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                        currentPartition,numBestSub);
                rename(moveFileFrom,moveFileTo);
            }

            // Initialize list of normative patterns
            NormativePatternList *normativePatternList = NULL;
            normativePatternList = AllocateNormativePatternList();

            printf("\nCurrent Partition: %lu\n",currentPartition);
            printf("\nOldest Partition: %lu\n",oldestPartition);
            fflush(stdout);
    
            normScore = FindBestNormativePattern(configuration,
                                                 oldestPartition,
                                                 currentPartition,
                                                 normativePatternList);

            if (normScore != 0)
            {
               //
               // What is the current best normative pattern?
               //
               // NOTE:  This assumes bestSub.g exists - if it doesn't, something bad
               //        happened above...
               //
               printf("\nCurrent Normative Pattern:\n\n");
               char buff[256];
               FILE *fp = fopen( "bestSub.g", "r" );
               while ( fgets( buff, 256, fp ) != NULL )
                  fputs( buff, stdout );
               fclose( fp );

               //
               // Step 5d(iii):  If (P' <> P), each partition discovers new anomalous 
               //                substructures based upon P'.
               //

               // build and execute gm tool command to see if new substructure
               // is different than previous best substructure...
               const char* gmExecutable = configuration->GM_EXECUTABLE;
               sprintf(gmCommand, "%s bestSub.g %sbestSub.g",
                       gmExecutable,
                       configuration->BEST_NORMATIVE_PATTERN_DIR);
               status = system(gmCommand);
               if (status != 0)
               {
                  printf("\n(This is a NEW normative pattern)\n");

                  printf("\n*********************************************************************************************\n");
                  printf("\n\nStep 5d(iii):  New normative pattern across the partitions - need to search for new anomalies...\n\n");
                  printf("\n*********************************************************************************************\n");
                  fflush(stdout);
   
                  ProcessPartitionsForAnomalyDetectionInParallel(configuration,
                                                                 normativePatternList,
                                                                 normScore);
                  WaitingForProcessesToFinish(configuration);
   
                  // Replace old best substructure with this new one
                  sprintf(moveFileFrom,"./bestSub.g");
                  sprintf(moveFileTo,"%sbestSub.g",configuration->BEST_NORMATIVE_PATTERN_DIR);
                  rename(moveFileFrom,moveFileTo);
   
                  // get number of anomalies...
                  numAnomalousInstances = 0;
                  FILE *numAnomaliesFilePtr = fopen("numanom.txt","r");
                  if (numAnomaliesFilePtr == NULL)
                  {
                     perror("No anomalies to report (Step 5d).\n");
                  }
                  else
                  {
                     printf("Opening numanom.txt file...\n");
                     fscanf(numAnomaliesFilePtr,"%lu",&numAnomalousInstances);
                     fclose(numAnomaliesFilePtr);
                     remove("numanom.txt");
                  }
               }
               else
               {
                  //
                  // Step 5d(iv):  Else, only new partition discovers anomalous 
                  //           substructure(s).
                  //
                  // NOTE:  The following could be modularized as it is very similar
                  //        to what happens in ProcessPartitionsForAnomalyDetection
                  //
                  printf("\n************************************************************************************************************\n");
                  printf("\n\nStep 5d(iv):  No change in normative pattern - only new partition needs to discover anomalous substructures...\n\n");
                  printf("\n************************************************************************************************************\n");
                  fflush(stdout);

                  // build appropriate GBAD command
   
                  // Need to determine which best substructure for this graph input
                  // file matches the best substructure from among all of the
                  // partitions in the current "window".
                  //
                  // NOTE:  I also do this logic in ProcessPartitionsForAnomalyDetectionInParallel
                  //        so we should consider modularizing
                  //
                  ULONG normativePatternNumber = 0;
                  for (numBestSub = 1; numBestSub <= configuration->NUM_NORMATIVE_PATTERNS; numBestSub++)
                  {
                     // first, see if file even exists...
                     sprintf(normFileName,"%snorm_%lu_%lu",
                             configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                             currentPartition,
                             numBestSub);
                     normFile = fopen(normFileName,"r");
                     if (normFile != NULL)
                     {
                        const char* gmExecutable = configuration->GM_EXECUTABLE;
                        sprintf(gmCommand, "%s bestSub.g %snorm_%lu_%lu",
                                gmExecutable,
                                configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                                currentPartition,
                                numBestSub);
                        status = system(gmCommand);
                        if (status == 0)
                        {
                           normativePatternNumber = numBestSub;
                           break;
                        }
                     }
                  }
                  
                  // if there is at least one normative pattern....
                  if (normativePatternNumber > 0)
                  {
                     // open normative pattern file to find name of graph input file
                     sprintf(normFileName,"%snorm_%lu_%lu",
                             configuration->NORM_SUBSTRUCTURE_FILES_DIR,
                             currentPartition,
                             normativePatternNumber);
                     normFile = fopen(normFileName,"r");
                     if (normFile == NULL)
                     {
                        printf("ERROR: opening file - %s -- exiting PLADS\n",normFileName);
                        exit(1);
                     }
                     // get name of graph input file (first line)
                     fscanf(normFile,"%s %s",line, graphInputFileName);
                     fclose(normFile);
   
                     const char* gbadExecutable = configuration->GBAD_EXECUTABLE;
                     sprintf(gbadCommand, "%s -norm %lu -partition %lu -plads %s %f -nsubs %i %s %s %s %s %s %s %s %s %s %s%s > %sgbadOutput_%lu.out",
                             gbadExecutable,
                             normativePatternNumber,
                             currentPartition,
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
                             graphInputFileName,
                             configuration->ANOMALOUS_OUTPUT_FILES_DIR,
                             currentPartition);

                     status = system(gbadCommand);
                     if (status != 0)
                     {
                        printf("ERROR:  Unable to execute GBAD ... exiting\n");
                        exit(1);
                     }
                     printf("... finished anomaly detection on %s\n",graphInputFileName);
                     fflush(stdout);
                  }
                  else
                  {
                     printf("\nNormative pattern not discovered in current partition (%lu) ...  no anomaly detection performed\n\n",
                            currentPartition);
                     fflush(stdout);
                  }

                  // Move input graph file from processing area to processed area
                  sprintf(moveFileFrom,"%s%s",configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR,
                          graphInputFileName);
                  sprintf(moveFileTo,"%s%s",configuration->PROCESSED_INPUT_FILES_DIR,
                          graphInputFileName);
                  rename(moveFileFrom,moveFileTo);
                  // move all anomalous substructure files to directory for processing
                  // in next step
                  numAnomalousInstances = 
                     MoveAnomalousSubstructureFiles(currentPartition,
                                                    configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR);
   
                  //
                  // Need to calculate the total number of anomalous instance
                  // files to be processed in the next step
                  //
                  numAnomalousInstances = 
                     CountNumberOfAnomalousInstances(oldestPartition,
                                                     currentPartition,
                                                     configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR);
               }

               FreeNormativePatternList(normativePatternList);
            }
         }
         else
         {
            //printf("*** Deviation is low - do NOT need to discover the normative pattern for partition %lu ***\n",currentPartition);
            //
            // Step 5e:  New partition discovers anomalous substructures from previously discovered normative pattern...
            //
            // NOTE:  The following could be modularized as it is very similar
            //        to what happens in ProcessPartitionsForAnomalyDetection
            //        and in ProcessPartitionsForAnomalyDetectionInParallel
            //
            printf("\n************************************************************************************************************\n");
            printf("\n\nStep 5e:  New partition (file: %s) discovers anomalous substructures from previously discovered normative pattern...\n\n",
                    fileName);
            printf("\n************************************************************************************************************\n");
            fflush(stdout);

            // Move graph input file to area for anomaly detection (i.e., 
            // skipping the "processed" area because we are not searching for 
            // the normative pattern)
            printf("Moving graph input file (%s) to processing area for anomaly detection...\n", fileName);
            fflush(stdout);
            status = MoveFile
                        (fileName,
                         configuration->GRAPH_INPUT_FILES_DIR,
                         configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR);
            if (status != 0)
            {
               printf("ERROR:  Unable to move file %s from %s to %s - exiting PLADS (from Main)\n",
                      fileName,
                      configuration->GRAPH_INPUT_FILES_DIR,
                      configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR);
               fflush(stdout);
               exit(1);
            }

            // build gbad command
            const char* gbadExecutable = configuration->GBAD_EXECUTABLE;
            sprintf(gbadCommand, "%s -bs %sbestSub.g -partition %lu -plads %s %f -nsubs %i %s %s %s %s %s %s %s %s %s %s%s > %sgbadOutput_%lu.out",
                    gbadExecutable,
                    configuration->BEST_NORMATIVE_PATTERN_DIR,
                    currentPartition,
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
                    fileName,
                    configuration->ANOMALOUS_OUTPUT_FILES_DIR,
                    currentPartition);

            status = system(gbadCommand);
            if (status != 0)
            {
               printf("ERROR:  Unable to execute GBAD ... exiting\n");
               exit(1);
            }
            printf("... finished anomaly detection on %s\n",fileName);
            fflush(stdout);
      
            // Move input graph file from processing area to processed area
            sprintf(moveFileFrom,"%s%s",configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR,
                    fileName);
            sprintf(moveFileTo,"%s%s",configuration->PROCESSED_INPUT_FILES_DIR,
                    fileName);
            rename(moveFileFrom,moveFileTo);
            // move all anomalous substructure files to directory for processing
            // in next step
            numAnomalousInstances = 
               MoveAnomalousSubstructureFiles(currentPartition,
                                              configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR);
   
            //
            // Need to calculate the total number of anomalous instance
            // files to be processed in the next step
            //
            numAnomalousInstances = 
               CountNumberOfAnomalousInstances(oldestPartition,
                                               currentPartition,
                                               configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR);
         }

         //
         // Step 5f:  Evaluate anomalous substructures across partitions; 
         //           report most anomalous substructure(s).
         //
         printf("\n*****************************************************************\n");
         printf("\n\nStep 5f:  Evaluate anomalous substructures across partitions...\n\n");
         printf("\n*****************************************************************\n");
         fflush(stdout);

         // Initialize list of anomalous substructures
         AnomalousSubstructureList *mostAnomalousSubstructureList = NULL;
         mostAnomalousSubstructureList = AllocateAnomalousSubstructureList();

         double anomScore = DBL_MAX;

         anomScore = FindMostAnomalousSubstructures(configuration,
                                                    oldestPartition,
                                                    numAnomalousInstances,
                                                    mostAnomalousSubstructureList);

         //
         // Output most anomalous substructure(s) at this point
         //
         // NOTE:  If the anomalous substructures have not changed (i.e., are the
         //        same as the previous iteration), currently, this will repeat
         //        the display of the same anomalous substructures
         //
         if (mostAnomalousSubstructureList != NULL)
         {
            AnomalousSubstructureListNode *mostAnomalousSubstructureListNode = NULL;
            char anomSubFileName[FILE_NAME_LEN];
            char buff[256];
            mostAnomalousSubstructureListNode = mostAnomalousSubstructureList->head;
            int countAnomSubs = 0;
            if (mostAnomalousSubstructureListNode != NULL)
               printf("Most Anomalous Substructures:\n\n");
            else
            {
               printf("No anomalous substructures to report.\n");
               fflush(stdout);
            }
            while (mostAnomalousSubstructureListNode != NULL)
            {
               // The reason for the following unusual comparison is that there is
               // not a good way to compare two floating type variables.  I am
               // using this for now, BUT, if the precision needs to go out beyond
               // 6 digits to the right, this will not work.
               if (fabs(mostAnomalousSubstructureListNode->anomalousSubstructure->score - anomScore) < 0.000001)
               {
                  printf("(partition %lu and anomalous number %lu)\n",
                         mostAnomalousSubstructureListNode->anomalousSubstructure->partitionNumber,
                         mostAnomalousSubstructureListNode->anomalousSubstructure->anomalousNumber);
                  fflush(stdout);
                  sprintf(anomSubFileName,"%sanomInst_%lu_%lu",
                          configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR,
                          mostAnomalousSubstructureListNode->anomalousSubstructure->partitionNumber,
                          mostAnomalousSubstructureListNode->anomalousSubstructure->anomalousNumber);
                  FILE *fp = fopen(anomSubFileName,"r");
                  while ( fgets( buff, 256, fp ) != NULL )
                     fputs( buff, stdout );
                  fclose( fp );
                  printf("\n");
                  countAnomSubs++;
               }
               mostAnomalousSubstructureListNode = mostAnomalousSubstructureListNode->next;
            }
            if (countAnomSubs > 0)
            {
               printf("\n(Number of anomalous substructures reported: %d)\n",countAnomSubs);
               fflush(stdout);
            }
            else
               {
               printf("No anomalous substructures to report.\n");
               fflush(stdout);
            }
         }
         else
            {
            printf("No anomalous substructures to report.\n");
               fflush(stdout);
            }
         printf("\n");
         fflush(stdout);

         if (numAnomalousInstances > 0)
         {
            FreeAnomalousSubstructureList(mostAnomalousSubstructureList);
         }

         stepEndTime = time(NULL);
         printf("\nStep 5: elapsed CPU time (partition %lu) = %lu seconds)\n",
                currentPartition,(stepEndTime - stepStartTime));
         fflush(stdout);

         fullEndTime = time(NULL);
         printf("\n                                                (Total running time so far = %lu seconds)\n",
                (fullEndTime - fullStartTime));
         fflush(stdout);
      }
      else
      {
         fullEndTime = time(NULL);
         printf("\nPLADS finished processing all available graph partitions (elapsed CPU time = %lu seconds)\n",
          (fullEndTime - fullStartTime));
         fflush(stdout);

         // Wait TIME_BETWEEN_FILE_CHECK
         printf("Waiting for new graph partition...\n");
         fflush(stdout);
         sleep(configuration->TIME_BETWEEN_FILE_CHECK);
      } 
   }

   // Should never reach this point (i.e., the program should run continuously)
   free(parameters);

   printf("\n... exiting PLADS");
   return 0;
}


//******************************************************************************
// NAME: GetConfiguration
//
// INPUTS: none
//
// RETURN: (Configuration *)
//
// PURPOSE: Initialize configuration entries that will be used throughout the
//          execution of PLADS.
//******************************************************************************

Configuration *GetConfiguration(int argc, char *argv[])
{
   Configuration *configuration;

   FILE *configFilePtr;
   char configParam[CONFIGURATION_SETTING_NAME_LEN];
   char configStringValue[FILE_NAME_LEN];
   int  configIntSetting;
   float  configFloatSetting;
   char skipChar;
 
   configuration = (Configuration *) malloc(sizeof(Configuration));
   if (configuration == NULL)
      OutOfMemoryError("configuration");

   configFilePtr = fopen("plads.cfg","r"); // read mode
 
   if (configFilePtr == NULL)
   {
      perror("Error opening the plads.cfg file -- exiting PLADS.\n");
      exit(-1);
   }
   else
      printf("Opening plads.cfg file...\n");
   fflush(stdout);
 
   // default values for some parameters
   configuration->CHANGE_DETECTION_APPROACH = 0;

   // Loop over lines, skipping lines starting with "//", until end of file
   fscanf(configFilePtr,"%s",configParam);
   while (!feof(configFilePtr))
   {
      if (strcmp(configParam,"//") == 0)
      {
         while ((skipChar = getc(configFilePtr)) != '\n')
         {
            // skip rest of line
         }
      }
      else
      {
         if (strcmp(configParam,"GRAPH_INPUT_FILES_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GRAPH_INPUT_FILES_DIR,configStringValue);
         }
         if (strcmp(configParam,"FILES_BEING_PROCESSED_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->FILES_BEING_PROCESSED_DIR,configStringValue);
         }
         if (strcmp(configParam,"PROCESSED_INPUT_FILES_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->PROCESSED_INPUT_FILES_DIR,configStringValue);
         }
         if (strcmp(configParam,"INITIAL_FILES_FOR_ANOM_DETECTION_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR,configStringValue);
         }
         if (strcmp(configParam,"BEST_NORMATIVE_PATTERN_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->BEST_NORMATIVE_PATTERN_DIR,configStringValue);
         }
         if (strcmp(configParam,"ANOMALOUS_SUBSTRUCTURE_FILES_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR,configStringValue);
         }
         if (strcmp(configParam,"NORM_SUBSTRUCTURE_FILES_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->NORM_SUBSTRUCTURE_FILES_DIR,configStringValue);
         }
         if (strcmp(configParam,"NUM_PARTITIONS") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);   // get value for this parameter
            configIntSetting = atoi(configStringValue);     // convert to integer value
            configuration->NUM_PARTITIONS = configIntSetting;
         }
         if (strcmp(configParam,"THRESHOLD_FOR_NUM_EXCEEDED_METRICS") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);   // get value for this parameter
            configIntSetting = atoi(configStringValue);     // convert to integer value
            configuration->THRESHOLD_FOR_NUM_EXCEEDED_METRICS = configIntSetting;
         }
         if (strcmp(configParam,"NUM_NORMATIVE_PATTERNS") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);   // get value for this parameter
            configIntSetting = atoi(configStringValue);     // convert to integer value
            configuration->NUM_NORMATIVE_PATTERNS = configIntSetting;
         }
         if (strcmp(configParam,"GBAD_EXECUTABLE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_EXECUTABLE,configStringValue);
         }
         if (strcmp(configParam,"GM_EXECUTABLE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GM_EXECUTABLE,configStringValue);
         }
         if (strcmp(configParam,"GBAD_ALGORITHM") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_ALGORITHM,configStringValue);
         }
         if (strcmp(configParam,"GBAD_THRESHOLD") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            //strcpy(configuration->GBAD_THRESHOLD,configStringValue);
            configFloatSetting = atof(configStringValue);     // convert to float value
            configuration->GBAD_THRESHOLD = configFloatSetting;
         }
         if (strcmp(configParam,"GBAD_PARAMETER_1") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_1,configStringValue);
         }
         if (strcmp(configParam,"GBAD_PARAMETER_VALUE_1") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_VALUE_1,configStringValue);
         }
         if (strcmp(configParam,"GBAD_PARAMETER_2") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_2,configStringValue);
         }
         if (strcmp(configParam,"GBAD_PARAMETER_VALUE_2") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_VALUE_2,configStringValue);
         }
         if (strcmp(configParam,"GBAD_PARAMETER_3") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_3,configStringValue);
         }
         if (strcmp(configParam,"GBAD_PARAMETER_VALUE_3") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_VALUE_3,configStringValue);
         }
         if (strcmp(configParam,"GBAD_PARAMETER_4") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_4,configStringValue);
         }
         if (strcmp(configParam,"GBAD_PARAMETER_VALUE_4") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_VALUE_4,configStringValue);
         }
         if (strcmp(configParam,"GBAD_PARAMETER_WITH_NO_VALUE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->GBAD_PARAMETER_WITH_NO_VALUE,configStringValue);
         }
         if (strcmp(configParam,"OUTPUT_FILES_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->OUTPUT_FILES_DIR,configStringValue);
         }
         if (strcmp(configParam,"ANOMALOUS_OUTPUT_FILES_DIR") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->ANOMALOUS_OUTPUT_FILES_DIR,configStringValue);
         }
         if (strcmp(configParam,"TIME_BETWEEN_FILE_CHECK") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);   // get value for this parameter
            configIntSetting = atoi(configStringValue);           // convert to integer value
            configuration->TIME_BETWEEN_FILE_CHECK = configIntSetting;
         }
         if (strcmp(configParam,"CHANGE_DETECTION_APPROACH") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);   // get value for this parameter
            configIntSetting = atoi(configStringValue);           // convert to integer value
            configuration->CHANGE_DETECTION_APPROACH = configIntSetting;
         }
         if (strcmp(configParam,"CONNECTEDNESS_EXECUTABLE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->CONNECTEDNESS_EXECUTABLE,configStringValue);
         }
         if (strcmp(configParam,"CLUSTERING_EXECUTABLE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->CLUSTERING_EXECUTABLE,configStringValue);
         }
         if (strcmp(configParam,"EIGENVALUE_EXECUTABLE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->EIGENVALUE_EXECUTABLE,configStringValue);
         }
         if (strcmp(configParam,"COMMUNITY_EXECUTABLE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->COMMUNITY_EXECUTABLE,configStringValue);
         }
         if (strcmp(configParam,"TRIADS_EXECUTABLE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->TRIADS_EXECUTABLE,configStringValue);
         }
         if (strcmp(configParam,"ENTROPY_EXECUTABLE") == 0)
         {
            fscanf(configFilePtr,"%s",configStringValue);  // get value for this parameter
            strcpy(configuration->ENTROPY_EXECUTABLE,configStringValue);
         }
      }
      fscanf(configFilePtr,"%s",configParam);
   }
   fclose(configFilePtr);

   return configuration;
}


//******************************************************************************
// NAME: PrintConfiguration
//
// INPUTS: (Configuration *configuration)
//
// RETURN: (void)
//
// PURPOSE: Print selected parameters.
//******************************************************************************

void PrintConfiguration(Configuration *configuration)
{
   printf("Configuration:\n");
   printf("GRAPH_INPUT_FILES_DIR:                 %s\n",configuration->GRAPH_INPUT_FILES_DIR);
   printf("TIME_BETWEEN_FILE_CHECK:               %i\n",configuration->TIME_BETWEEN_FILE_CHECK);
   printf("NUM_PARTITIONS:                        %i\n",configuration->NUM_PARTITIONS);
   printf("FILES_BEING_PROCESSED_DIR:             %s\n",configuration->FILES_BEING_PROCESSED_DIR);
   printf("PROCESSED_INPUT_FILES_DIR:             %s\n",configuration->PROCESSED_INPUT_FILES_DIR);
   printf("INITIAL_FILES_FOR_ANOM_DETECTION_DIR:  %s\n",configuration->INITIAL_FILES_FOR_ANOM_DETECTION_DIR);
   printf("BEST_NORMATIVE_PATTERN_DIR:            %s\n",configuration->BEST_NORMATIVE_PATTERN_DIR);
   printf("ANOMALOUS_SUBSTRUCTURE_FILES_DIR:      %s\n",configuration->ANOMALOUS_SUBSTRUCTURE_FILES_DIR);
   printf("NORM_SUBSTRUCTURE_FILES_DIR:           %s\n",configuration->NORM_SUBSTRUCTURE_FILES_DIR);
   printf("NUM_PARTITIONS:                        %i\n",configuration->NUM_PARTITIONS);
   printf("NUM_NORMATIVE_PATTERNS                 %i\n",configuration->NUM_NORMATIVE_PATTERNS);
   printf("GBAD_EXECUTABLE:                       %s\n",configuration->GBAD_EXECUTABLE);
   printf("GM_EXECUTABLE:                         %s\n",configuration->GM_EXECUTABLE);
   printf("GBAD_ALGORITHM:                        %s\n",configuration->GBAD_ALGORITHM);
   printf("GBAD_THRESHOLD:                        %f\n",configuration->GBAD_THRESHOLD);
   printf("GBAD_PARAMETER_1:                      %s\n",configuration->GBAD_PARAMETER_1);
   printf("GBAD_PARAMETER_VALUE_1:                %s\n",configuration->GBAD_PARAMETER_VALUE_1);
   printf("GBAD_PARAMETER_2:                      %s\n",configuration->GBAD_PARAMETER_2);
   printf("GBAD_PARAMETER_VALUE_2:                %s\n",configuration->GBAD_PARAMETER_VALUE_2);
   printf("GBAD_PARAMETER_3:                      %s\n",configuration->GBAD_PARAMETER_3);
   printf("GBAD_PARAMETER_VALUE_3:                %s\n",configuration->GBAD_PARAMETER_VALUE_3);
   printf("GBAD_PARAMETER_4:                      %s\n",configuration->GBAD_PARAMETER_4);
   printf("GBAD_PARAMETER_VALUE_4:                %s\n",configuration->GBAD_PARAMETER_VALUE_4);
   printf("GBAD_PARAMETER_WITH_NO_VALUE:          %s\n",configuration->GBAD_PARAMETER_WITH_NO_VALUE);
   printf("OUTPUT_FILES_DIR:                      %s\n",configuration->OUTPUT_FILES_DIR);
   printf("ANOMALOUS_OUTPUT_FILES_DIR:            %s\n",configuration->ANOMALOUS_OUTPUT_FILES_DIR);
   printf("CHANGE_DETECTION_APPROACH:             %i\n",configuration->CHANGE_DETECTION_APPROACH);
   printf("THRESHOLD_FOR_NUM_EXCEEDED_METRICS:    %i\n",configuration->THRESHOLD_FOR_NUM_EXCEEDED_METRICS);
   printf("CONNECTEDNESS_EXECUTABLE:              %s\n",configuration->CONNECTEDNESS_EXECUTABLE);
   printf("CLUSTERING_EXECUTABLE:                 %s\n",configuration->CLUSTERING_EXECUTABLE);
   printf("EIGENVALUE_EXECUTABLE:                 %s\n",configuration->EIGENVALUE_EXECUTABLE);
   printf("COMMUNITY_EXECUTABLE:                  %s\n",configuration->COMMUNITY_EXECUTABLE);
   printf("TRIADS_EXECUTABLE:                     %s\n",configuration->TRIADS_EXECUTABLE);
   printf("ENTROPY_EXECUTABLE:                    %s\n",configuration->ENTROPY_EXECUTABLE);
   fflush(stdout);
}


//******************************************************************************
// NAME: GetParameters
//
// INPUTS: (int argc) - number of command-line arguments
//         (char *argv[]) - array of command-line argument strings
//
// RETURN: (Parameters *)
//
// PURPOSE: Initialize parameters structure and process command-line
//          options.
//******************************************************************************

Parameters *GetParameters(int argc, char *argv[])
{
   Parameters *parameters;

   parameters = NULL;

   return parameters;
}


//******************************************************************************
// NAME: PrintParameters
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Print selected parameters.
//******************************************************************************

void PrintParameters(Parameters *parameters)
{
   printf("Parameters:\n");
   fflush(stdout);
}
