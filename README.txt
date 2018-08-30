PATTERN LEARNING AND ANOMALY DETECTION IN STREAMS (PLADS)
=========================================================

Pattern Learning and Anomaly Detection in Streams (or PLADS) is a novel approach that focuses on streaming data that represents the relationships and transactions between entities (people, organizations, etc.) by discovering and analyzing a graph structure of their social and behavioral network. Through the analysis of the structure surrounding actions, such as can be found in communications or data access, our approach first searches for the normative patterns of activity, then discovers those small, unusual deviations and presents the patterns and anomalies to an analyst. This approach uses novel algorithms that can be applied to the dynamic discovery of anomalies in a variety of areas, including the detection of insider threats, terrorist activity, fraud, and cybercrime.

Authors: 
Dr. William Eberle, Department of Computer Science, Tennessee Tech University, email:  weberle@tntech.edu.
Dr. Larry Holder, School of Electrical Engineering and Computer Science, Washington State University, email: holder@wsu.edu. 

Support: This material is based upon work supported by the National Science Foundation under Grant Nos. 1318957 and 1646640.


Steps to Installing and Running PLADS
=====================================

VERSION

1.0  (08/07/17)

REQUIRED THIRD-PARTY SOFTWARE

- gCC
- python (including snap package/library)
- R (including sna package)

INSTALLATION

1. Unzip plads.tar file into targeted directory.

2. Create needed subdirectories by running create_directories script.
   NOTE: You can name these subdirectories whatever you want - they just need
   to match what is specified in the plads.cfg file.

3. Compile PLADS (run "make" in /src_plads/ directory) and install PLADS 
   executable (run "make install" in /src_plads/ directory)

4. Compile GBAD (run "make" in /src_gbad/ directory) and install GBAD 
   (modified to work for PLADS) executable (run "make install" in 
   /src_gbad/ directory)

5. Edit plads.cfg file to match root directory structure of where PLADS is 
   installed.  NOTE:  See #2 above if you want different subdirectory names.

RUNNING

1. Put graph input files to be processed in the /graph_input_files/ directory.

2. Run plads, redirecting output to a file (e.g., ./bin/plads > plads.out).  
   NOTE: If processing a large number of files (or large files), might want 
   to run it in the background.

   NOTE: Graphs will be processed in date-time order (i.e., oldest file first).  
   If two files have the same date-time, it will pick the next file to be 
   processed (somewhat) arbitrarily.

3. PLADS will run indefinitely, monitoring the "graph_input_files" directory 
   for new graph input files.  (How long it waits between checking for new 
   files is specified in plads.cfg.) If you want PLADS to stop processing 
   files, you will need to manually kills PLADS (e.g., kill -9 <PID>).

4. Each time PLADS has completed running, you can reset the run-time area by 
   running the "plads_reset" script.  This will result in temporary files being 
   removed, files being moved back to the directories where they started, etc.

NOTES

- The first line in each graph input file (not including comments), must start
  at "XP # 1" (i.e., can't just divide an existing graph input file into 
  multiple files without having each graph input file start over in the 
  numbering sequence. (See GBAD documentation for more information about the
  graph input file format.)

- Do not specify more partitions (in plads.cfg) than CPUs available.

- The "change detection" option (if not set to "NONE" in plads.cfg) helps to 
  speed up the processing by determining whether or not to search for a new
  normative pattern IF the specified graph property metric for the new partition
  exceeds a threshold. If none is specified, then a new normative pattern is
  generated with each new partition.

KNOWN ISSUES

- If the chosen normative pattern across the N partitions does not exist for a 
  particular partition, it will throw an error for that partition (but will 
  continue processing).

