//******************************************************************************
// plads.h
//
// Data type and prototype definitions for the PLADS system.
//
// Date      Name       Description
// ========  =========  ========================================================
// 08/07/17  Eberle     Initial version.
//
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <float.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>

#ifndef PLADS_H
#define PLADS_H

#define PLADS_VERSION "0.1"

//******************************************************************************
// Constants
//******************************************************************************

#define SPACE ' '
#define TAB   '\t'
#define NEWLINE '\n'
#define DOUBLEQUOTE '\"'
#define CARRIAGERETURN '\r'

#define FALSE 0
#define TRUE  1

#define FILE_NAME_LEN 256                  // maximum length of file names
#define COMMAND_LEN 512                    // maximum length of a command
#define CONFIGURATION_SETTING_NAME_LEN 32  // maximum length of a configuration
                                           // setting name
#define MAX_TIME_STAMP LONG_MAX            // maximum time stamp

//******************************************************************************
// Type Definitions
//******************************************************************************

typedef unsigned char UCHAR;
typedef unsigned char BOOLEAN;
typedef unsigned long ULONG;

typedef struct
{
} Parameters;

typedef struct
{
   char GRAPH_INPUT_FILES_DIR[FILE_NAME_LEN];      // directory for storage of  
                                                   // graph input files

   char FILES_BEING_PROCESSED_DIR[FILE_NAME_LEN];  // directory for storage of  
                                                   // graph input files that
                                                   // are currently being
                                                   // processed

   char PROCESSED_INPUT_FILES_DIR[FILE_NAME_LEN];  // directory for storage of  
                                                   // graph input files that
                                                   // have been processed

   int NUM_PARTITIONS;                             // number of partitions (N)
                                                   // NOTE:  This should probably
                                                   //        NOT be more than the
                                                   //        number of CPUs

   int NUM_NORMATIVE_PATTERNS;                     // number of normative 
                                                   // patterns (M)

   char INITIAL_FILES_FOR_ANOM_DETECTION_DIR[FILE_NAME_LEN];  // directory
                                                   // of initial files to be
                                                   // analyzes for anomalies

   char BEST_NORMATIVE_PATTERN_DIR[FILE_NAME_LEN]; // directory for best 
                                                   // normative pattern

   char ANOMALOUS_SUBSTRUCTURE_FILES_DIR[FILE_NAME_LEN]; // directory for 
                                                   // anomalous substructure files

   char NORM_SUBSTRUCTURE_FILES_DIR[FILE_NAME_LEN]; // directory for 
                                                   // normative substructure files

   char GBAD_EXECUTABLE[FILE_NAME_LEN];            // location and name of GBAD
                                                   // executable

   char GM_EXECUTABLE[FILE_NAME_LEN];              // location and name of gm
                                                   // tool

   char GBAD_ALGORITHM[5];                         // GBAD algorithm to be 
                                                   // applied to input graph

   float GBAD_THRESHOLD;                           // Threshold for specified
                                                   // GBAD algorithm

   char GBAD_PARAMETER_1[24];                      // other GBAD parameter

   char GBAD_PARAMETER_VALUE_1[8];                 // other GBAD parameter value

   char GBAD_PARAMETER_2[24];                      // another GBAD parameter

   char GBAD_PARAMETER_VALUE_2[8];                 // another GBAD parameter value

   char GBAD_PARAMETER_3[24];                      // and another GBAD parameter

   char GBAD_PARAMETER_VALUE_3[8];                 // and another GBAD parameter value

   char GBAD_PARAMETER_4[24];                      // and and another GBAD parameter

   char GBAD_PARAMETER_VALUE_4[8];                 // and and another GBAD parameter value

   char GBAD_PARAMETER_WITH_NO_VALUE[24];          // GBAD parameter with no value

   char OUTPUT_FILES_DIR[FILE_NAME_LEN];           // location of output files

   char ANOMALOUS_OUTPUT_FILES_DIR[FILE_NAME_LEN]; // location of anomalous 
                                                   // output files

   int TIME_BETWEEN_FILE_CHECK;                    // time between checking for 
                                                   // files
   int CHANGE_DETECTION_APPROACH;                  // method for detection change

   char CONNECTEDNESS_EXECUTABLE[FILE_NAME_LEN];   // location and name of connectedness
                                                   // executable

   char CLUSTERING_EXECUTABLE[FILE_NAME_LEN];      // location and name of clustering
                                                   // coefficient executable

   char EIGENVALUE_EXECUTABLE[FILE_NAME_LEN];      // location and name of eigenvalue
                                                   // executable

   char COMMUNITY_EXECUTABLE[FILE_NAME_LEN];       // location and name of community
                                                   // executable

   char TRIADS_EXECUTABLE[FILE_NAME_LEN];          // location and name of triads
                                                   // executable

   char ENTROPY_EXECUTABLE[FILE_NAME_LEN];         // location and name of entropy
                                                   // executable

   int THRESHOLD_FOR_NUM_EXCEEDED_METRICS;         // number of graph metrics that
                                                   // exceed standard deviation

} Configuration;


// Normative Pattern
typedef struct _normative_pattern
{
   ULONG partitionNumber;          // partition number of this normative pattern
   ULONG bestSubNumber;            // best substructure number in this partition
   ULONG score;                    // score of "normalousness"
   char graphInputFileName[FILE_NAME_LEN];    // name of source input file
   struct _normative_pattern *parentNormativePattern;  // pointer to parent 
                                              // normative pattern
} NormativePattern;

// NormativePatternListNode: node in singly-linked list of normative patterns
typedef struct _normative_pattern_list_node 
{
   NormativePattern *normativePattern;
   struct _normative_pattern_list_node *next;
} NormativePatternListNode;

// NormativePatternList: singly-linked list of normative patterns
typedef struct 
{
   NormativePatternListNode *head;
} NormativePatternList;


// Anomalous Substructure
typedef struct _anomalous_substructure
{
   ULONG partitionNumber;          // partition number of this anomalous sub
   ULONG anomalousNumber;          // number of anomaly in this partition
   //ULONG score;                    // score of "anomalousness"
   double score;                    // score of "anomalousness"
   struct _anomalous_substructure *parentAnomalousSubstructure;  // pointer to parent 
                                              // anomalous substructure
} AnomalousSubstructure;

// AnomalousSubstructureListNode: node in singly-linked list of anomalous subs
typedef struct _anomalous_substructure_list_node 
{
   AnomalousSubstructure *anomalousSubstructure;
   struct _anomalous_substructure_list_node *next;
} AnomalousSubstructureListNode;

// AnomalousSubstructureList: singly-linked list of anomalous subs
typedef struct 
{
   AnomalousSubstructureListNode *head;
} AnomalousSubstructureList;


// Linked list of PIDS
typedef struct _pid_info
{
   pid_t PID;
} PidInfo;

// PidInfoListNode: node in singly-linked list of pids
typedef struct _pid_info_list_node 
{
   PidInfo *pidInfo;
   struct _pid_info_list_node *next;
} PidInfoListNode;

// PidInfoList: singly-linked list of pids
typedef struct 
{
   PidInfoListNode *head;
} PidInfoList;

typedef struct 
{
   ULONG partition;
   double value;
} gpNode;


//******************************************************************************
// Global Variables
//******************************************************************************

//double *partitionGP;
ULONG partitionGP;

//******************************************************************************
// Function Prototypes
//******************************************************************************

// plads.c
void ProcessPartitionsInParallel(Configuration *);
ULONG FindBestNormativePattern(Configuration *, ULONG, ULONG, 
                               NormativePatternList *);
ULONG ProcessPartitionsForAnomalyDetection(Configuration *,
                                           const NormativePatternList *, ULONG);
void ProcessPartitionsForAnomalyDetectionInParallel(Configuration *,
                                                    const NormativePatternList *, 
                                                    ULONG);
void WaitingForProcessesToFinish(Configuration *);
double FindMostAnomalousSubstructures(Configuration *, ULONG, int, 
                                      AnomalousSubstructureList *);
//void RunGBADForNormativePatterns(Configuration *, ULONG, struct dirent *);
void RunGBADForNormativePatterns(Configuration *, ULONG, char *);


// utility.c
void OutOfMemoryError(char *);
void PrintBoolean(BOOLEAN);
int MoveFile(char *,char *,char *);
struct dirent * FindOldestFile(char *);
ULONG MoveAnomalousSubstructureFiles(ULONG, char *);
NormativePatternList *AllocateNormativePatternList(void);
NormativePattern *AllocateNormativePattern(ULONG , ULONG );
void NormativePatternListInsert(NormativePattern *, NormativePatternList *);
NormativePatternListNode *AllocateNormativePatternListNode(NormativePattern *);

AnomalousSubstructureList *AllocateAnomalousSubstructureList(void);
AnomalousSubstructure *AllocateAnomalousSubstructure(ULONG , ULONG);
void AnomalousSubstructureListInsert(AnomalousSubstructure *, 
                                     AnomalousSubstructureList *);
AnomalousSubstructureListNode *AllocateAnomalousSubstructureListNode(AnomalousSubstructure *);

PidInfoList *AllocatePidInfoList(void);
PidInfo *AllocatePidInfo(pid_t);
void PidInfoListInsert(PidInfo *, PidInfoList *);
PidInfoListNode *AllocatePidInfoListNode(PidInfo *);
void PidInfoListDelete(pid_t, PidInfoList *);

void FreeNormativePattern(NormativePattern *);
void FreeNormativePatternListNode(NormativePatternListNode *);
void FreeNormativePatternList(NormativePatternList *);

void FreeAnomalousSubstructure(AnomalousSubstructure *);
void FreeAnomalousSubstructureListNode(AnomalousSubstructureListNode *);
void FreeAnomalousSubstructureList(AnomalousSubstructureList *);

void FreePidInfo(PidInfo *);
void FreePidInfoListNode(PidInfoListNode *);
void FreePidInfoList(PidInfoList *);

ULONG CountNumberOfAnomalousInstances(ULONG, ULONG, char *);

void RemoveNoLongerNeededFiles(char *, ULONG, ULONG);

void CreateVerticesAndEdgesFiles(char *, char *, ULONG);
void CreateEdgesCSVFile(ULONG, char *, char *);

double CalculateDensity(Configuration *, char *, char *);
double CalculateConnectedness(Configuration *, char *, char *, ULONG);
double CalculateClusteringCoefficient(Configuration *, char *, char *, ULONG);
double CalculateEigenvalue(Configuration *, char *, char *, ULONG);
double CalculateCommunity(Configuration *, char *, char *, ULONG);
double CalculateTriangles(Configuration *, char *, char *, ULONG);
double CalculateEntropy(ULONG, Configuration *, char *, char *);

void AddEntryToGPFile(ULONG, double, const char *);
double CalculateMeanFromGPFile(const char *);
double CalculateStandardDeviationFromGPFile(double,const char *);
void UpdateGPFile(Configuration *, ULONG, double, const char *);

//
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif

#endif
