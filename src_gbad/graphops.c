//******************************************************************************
// graphops.c
//
// Graph allocation, deallocation, input and output functions.
//
//
// Date      Name       Description
// ========  =========  ========================================================
// 08/12/09  Eberle     Initial version, taken from SUBDUE 5.2.1
// 12/17/09  Graves     Added GUI coloring logic
//////////////////////////////////////////////////// WFE - PLADS
// 02/27/14  Eberle     Added WriteNormGraphToFile to provide normative pattern
//                      and information for use in the PLADS approach.  Added
//                      ReadGraph for added gm tool.
//////////////////////////////////////////////////// WFE - PLADS
//
//******************************************************************************

#include "gbad.h"

//******************************************************************************
// NAME: ReadInputFile
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Reads in the graph input file (SUBDUE format), which currently only
// consists of positive graphs (no negative graphs), which are collected into
// the positive graph fields of the parameters.  Each example in the input file 
// is prefaced by the appropriate token defined in gbad.h.  The first graph in 
// the file is assumed positive.  Each graph is assumed to begin at vertex #1 
// and therefore examples are not connected to one another.
//******************************************************************************

void ReadInputFile(Parameters *parameters)
{
   ULONG index;
   
   Graph_Info info;
   info.graph = parameters->posGraph;
   info.labelList = parameters->labelList;
   info.preSubs = NULL;
   info.numPreSubs = 0;
   info.numPosEgs = parameters->numPosEgs;
   info.posEgsVertexIndices = parameters->posEgsVertexIndices;
   info.directed = parameters->directed;
   
   info.posGraphVertexListSize = parameters->posGraphVertexListSize;
   info.posGraphEdgeListSize = parameters->posGraphEdgeListSize;
   info.vertexOffset = (parameters->posGraph == NULL) ? 0 : parameters->posGraph->numVertices;
   
   info.xp_graph = TRUE;
   
   GP_read_graph(&info, parameters->inputFileName);
  
   parameters->posGraph = info.graph;
   parameters->labelList = info.labelList;
   parameters->numPosEgs = info.numPosEgs;
   parameters->posEgsVertexIndices = info.posEgsVertexIndices;
   
   parameters->posGraphVertexListSize = info.posGraphVertexListSize;
   parameters->posGraphEdgeListSize = info.posGraphEdgeListSize;
   
	
   // GUI coloring
   if (parameters->posGraph == NULL)
      parameters->originalPosGraph = NULL;
   else
      parameters->originalPosGraph = CopyGraph(parameters->posGraph);
   parameters->originalLabelList = AllocateLabelList();
   for (index=0; index != parameters->labelList->numLabels; index++)
      StoreLabel(&(parameters->labelList->labels[index]), parameters->originalLabelList);
}


//******************************************************************************
// NAME: AddVertexIndex
//
// INPUTS: (ULONG *vertexIndices) - array of indices to augment
//         (ULONG n) - size of new array
//         (ULONG index) - index to add to nth element of array
//
// RETURN: (ULONG *) - new vertex index array
//
// PURPOSE: Reallocate the given vertex index array and store the new
// index in the nth element of the array.  This is used to build the
// array of indices into the positive examples graphs.
//******************************************************************************

ULONG *AddVertexIndex(ULONG *vertexIndices, ULONG n, ULONG index)
{
   vertexIndices = (ULONG *) realloc(vertexIndices, sizeof(ULONG) * n);
   if (vertexIndices == NULL)
      OutOfMemoryError("AddVertexIndex:vertexIndices");
   vertexIndices[n - 1] = index;
   return vertexIndices;
}


//******************************************************************************
// NAME: ReadPredefinedSubsFile
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Reads in one or more graphs from the given file and stores
// these on the predefined substructure list in parameters.  Each
// graph is prefaced by the predefined substructure token defined in
// gbad.h.
//
// Right now, these substructures will be used to compress the graph,
// if present, and therefore any labels not present in the input graph
// will be discarded during compression.  If the predefined
// substructures are ever simply put on the discovery queue, then care
// should be taken to not include labels that do not appear in the
// input graph, as this would bias the MDL computation. (*****)
//******************************************************************************

void ReadPredefinedSubsFile(Parameters *parameters)
{
   Graph_Info info;
   info.graph = NULL;
   info.labelList = parameters->labelList;
   info.preSubs = parameters->preSubs;
   info.numPreSubs = parameters->numPreSubs;
   info.numPosEgs = 0;
   info.posEgsVertexIndices = 0;
   info.directed = parameters->directed;
   
   info.posGraphVertexListSize = 0;
   info.posGraphEdgeListSize = 0;
   info.vertexOffset = 0;
   
   info.xp_graph = FALSE;
   
   GP_read_graph(&info, parameters->psInputFileName);
  
   parameters->labelList = info.labelList;
   parameters->preSubs = info.preSubs;
   parameters->numPreSubs = info.numPreSubs;
}

//******************************************************************************
// NAME: AddVertex
//
// INPUTS: (Graph *graph) - graph to add vertex
//         (ULONG labelIndex) - index into label list of vertex's label
//         (ULONG *vertexListSize) - pointer to size of graph's allocated
//                                   vertex array
//         (ULONG sourceVertex) - vertex index to be saved for possible
//                                information after future compression
//
// RETURN: (void)
//
// PURPOSE: Add vertex information to graph. AddVertex also changes the
// size of the currently-allocated vertex array, which increases by
// LIST_SIZE_INC (instead of just 1) when exceeded.
//******************************************************************************

void AddVertex(Graph *graph, ULONG labelIndex, ULONG *vertexListSize, 
               ULONG sourceVertex)
{
   Vertex *newVertexList;
   ULONG numVertices;

   numVertices = graph->numVertices;
   // make sure there is enough room for another vertex
   if (*vertexListSize == graph->numVertices) 
   {
      *vertexListSize += LIST_SIZE_INC;
      newVertexList = (Vertex *) realloc(graph->vertices, 
                                         (sizeof(Vertex) * (*vertexListSize)));
      if (newVertexList == NULL)
         OutOfMemoryError("vertex list");
      graph->vertices = newVertexList;
   }

   // store information in vertex
   graph->vertices[numVertices].label = labelIndex;
   graph->vertices[numVertices].numEdges = 0;
   graph->vertices[numVertices].edges = NULL;
   graph->vertices[numVertices].map = VERTEX_UNMAPPED;
   graph->vertices[numVertices].used = FALSE;
   graph->vertices[numVertices].sourceVertex = sourceVertex;
   graph->vertices[numVertices].sourceExample = 0;   // will set later...

   // GUI coloring
   graph->vertices[numVertices].originalVertexIndex = numVertices;
   graph->vertices[numVertices].color = VERTEX_DEFAULT;
   graph->vertices[numVertices].anomalousValue = 2.0;

   graph->numVertices++;
}


//******************************************************************************
// NAME: AddEdge
//
// INPUTS: (Graph *graph) - graph to add edge to
//         (ULONG sourceVertexIndex) - index of edge's source vertex
//         (ULONG targetVertexIndex) - index of edge's target vertex
//         (BOOLEAN directed) - TRUE is edge is directed
//         (ULONG labelIndex) - index of edge's label in label list
//         (ULONG *edgeListSize) - pointer to size of graph's allocated
//                                 edge array
//         (ULONG spansIncrement)
//
// RETURN: (void)
//
// PURPOSE: Add edge information to graph. AddEdge also changes the
// size of the currently-allocated edge array, which increases by
// LIST_SIZE_INC (instead of just 1) when exceeded.
//******************************************************************************

void AddEdge(Graph *graph, ULONG sourceVertexIndex, ULONG targetVertexIndex,
             BOOLEAN directed, ULONG labelIndex, ULONG *edgeListSize,
             BOOLEAN spansIncrement)
{
   Edge *newEdgeList;

   // make sure there is enough room for another edge in the graph
   if (*edgeListSize == graph->numEdges) 
   {
      *edgeListSize += LIST_SIZE_INC;
      newEdgeList = (Edge *) realloc(graph->edges,
                                     (sizeof(Edge) * (*edgeListSize)));
      if (newEdgeList == NULL)
         OutOfMemoryError("AddEdge:newEdgeList");
      graph->edges = newEdgeList;
   }

   // add edge to graph
   graph->edges[graph->numEdges].vertex1 = sourceVertexIndex;
   graph->edges[graph->numEdges].vertex2 = targetVertexIndex;
   graph->edges[graph->numEdges].label = labelIndex;
   graph->edges[graph->numEdges].directed = directed;
   graph->edges[graph->numEdges].used = FALSE;
   graph->edges[graph->numEdges].spansIncrement = spansIncrement;
   graph->edges[graph->numEdges].validPath = TRUE;
   //
   // GBAD-P: Initialize anomalous flag and source vertices
   //
   graph->edges[graph->numEdges].anomalous = FALSE;
   graph->edges[graph->numEdges].sourceVertex1 = graph->vertices[sourceVertexIndex].sourceVertex;
   graph->edges[graph->numEdges].sourceVertex2 = graph->vertices[targetVertexIndex].sourceVertex;
   graph->edges[graph->numEdges].sourceExample = 0;   // will set later...
   //

   // add index to edge in edge index array of both vertices
   AddEdgeToVertices(graph, graph->numEdges);

   // GUI coloring
   graph->edges[graph->numEdges].originalEdgeIndex = graph->numEdges;
   graph->edges[graph->numEdges].color = EDGE_DEFAULT;
   graph->edges[graph->numEdges].anomalousValue = 2.0;

   graph->numEdges++;
}


//******************************************************************************
// NAME: StoreEdge
//
// INPUTS: (Edge *overlapEdges) - edge array where edge is stored
//         (ULONG edgeIndex) - index into edge array where edge is stored
//         (ULONG v1) - vertex1 of edge
//         (ULONG v2) - vertex2 of edge
//         (ULONG label) - edge label index
//         (BOOLEAN directed) - edge directedness
//         (BOOLEAN spansIncrement) - edge crossing increment boundary
//
// RETURN: (void)
//
// PURPOSE: Procedure to store an edge in given edge array.
//******************************************************************************

void StoreEdge(Edge *overlapEdges, ULONG edgeIndex,
               ULONG v1, ULONG v2, ULONG label, BOOLEAN directed, 
               BOOLEAN spansIncrement)
{
   overlapEdges[edgeIndex].vertex1 = v1;
   overlapEdges[edgeIndex].vertex2 = v2;
   overlapEdges[edgeIndex].label = label;
   overlapEdges[edgeIndex].directed = directed;
   overlapEdges[edgeIndex].used = FALSE;
   overlapEdges[edgeIndex].spansIncrement = spansIncrement;
}


//******************************************************************************
// NAME: AddEdgeToVertices
//
// INPUTS: (Graph *graph) - graph containing edge and vertices
//         (ULONG edgeIndex) - edge's index into graph edge array
//
// RETURN: (void)
//
// PURPOSE: Add edge index to the edge array of each of the two
// vertices involved in the edge.  If a self-edge, then only add once.
//******************************************************************************

void AddEdgeToVertices(Graph *graph, ULONG edgeIndex)
{
   ULONG v1, v2;
   Vertex *vertex;
   ULONG *edgeIndices;

   v1 = graph->edges[edgeIndex].vertex1;
   v2 = graph->edges[edgeIndex].vertex2;
   vertex = & graph->vertices[v1];
   edgeIndices = (ULONG *) realloc(vertex->edges,
                                   sizeof(ULONG) * (vertex->numEdges + 1));
   if (edgeIndices == NULL)
      OutOfMemoryError("AddEdgeToVertices:edgeIndices1");
   edgeIndices[vertex->numEdges] = edgeIndex;
   vertex->edges = edgeIndices;
   vertex->numEdges++;

   if (v1 != v2) 
   { // don't add a self edge twice
      vertex = & graph->vertices[v2];
      edgeIndices = (ULONG *) realloc(vertex->edges,
                                      sizeof(ULONG) * (vertex->numEdges + 1));
      if (edgeIndices == NULL)
         OutOfMemoryError("AddEdgeToVertices:edgeIndices2");
      edgeIndices[vertex->numEdges] = edgeIndex;
      vertex->edges = edgeIndices;
      vertex->numEdges++;
   }
}


//******************************************************************************
// NAME:    AllocateGraph
//
// INPUTS:  (ULONG v) - number of vertices in graph
//          (ULONG e) - number of edges in graph
//
// RETURN:  (Graph *) - pointer to newly-allocated graph
//
// PURPOSE: Allocate memory for new graph containing v vertices and e
// edges.
//******************************************************************************

Graph *AllocateGraph(ULONG v, ULONG e)
{
   Graph *graph;

   graph = (Graph *) malloc(sizeof(Graph));
   if (graph == NULL)
      OutOfMemoryError("AllocateGraph:graph");

   graph->numVertices = v;
   graph->numEdges = e;
   graph->vertices = NULL;
   graph->edges = NULL;
   if (v > 0) 
   {
      graph->vertices = (Vertex *) malloc(sizeof(Vertex) * v);
      if (graph->vertices == NULL)
         OutOfMemoryError("AllocateGraph:graph->vertices");
   }
   if (e > 0) 
   {
      graph->edges = (Edge *) malloc(sizeof(Edge) * e);
      if (graph->edges == NULL)
         OutOfMemoryError("AllocateGraph:graph->edges");
   }

   return graph;
}

//******************************************************************************
// NAME:    CopyGraph
//
// INPUTS:  (Graph *g) - graph to be copied
//
// RETURN:  (Graph *) - pointer to copy of graph
//
// PURPOSE: Create and return a copy of the given graph.
//******************************************************************************

Graph *CopyGraph(Graph *g)
{
   Graph *gCopy;
   ULONG nv;
   ULONG ne;
   ULONG v;
   ULONG e;
   ULONG numEdges;

   nv = g->numVertices;
   ne = g->numEdges;

   // allocate graph
   gCopy = AllocateGraph(nv, ne);

   // copy vertices; allocate and copy vertex edge arrays
   for (v = 0; v < nv; v++) 
   {
      gCopy->vertices[v].label = g->vertices[v].label;
      gCopy->vertices[v].map = g->vertices[v].map;
      gCopy->vertices[v].used = g->vertices[v].used;
      numEdges = g->vertices[v].numEdges;
      gCopy->vertices[v].numEdges = numEdges;
      gCopy->vertices[v].edges = NULL;
      gCopy->vertices[v].sourceVertex = g->vertices[v].sourceVertex;
      gCopy->vertices[v].sourceExample = g->vertices[v].sourceExample;
      if (numEdges > 0) 
      {
          gCopy->vertices[v].edges = (ULONG *) malloc(numEdges * sizeof(ULONG));
          if (gCopy->vertices[v].edges == NULL)
             OutOfMemoryError("CopyGraph:edges");
          for (e = 0; e < numEdges; e++)
             gCopy->vertices[v].edges[e] = g->vertices[v].edges[e];
      }
      // GUI coloring
      gCopy->vertices[v].originalVertexIndex = g->vertices[v].originalVertexIndex;
      gCopy->vertices[v].color = g->vertices[v].color;
      gCopy->vertices[v].anomalousValue = g->vertices[v].anomalousValue;
   }

   // copy edges
   for (e = 0; e < ne; e++) 
   {
      gCopy->edges[e].vertex1 = g->edges[e].vertex1;
      gCopy->edges[e].vertex2 = g->edges[e].vertex2;
      gCopy->edges[e].label = g->edges[e].label;
      gCopy->edges[e].directed = g->edges[e].directed;
      gCopy->edges[e].used = g->edges[e].used;
      gCopy->edges[e].sourceVertex1 = g->edges[e].sourceVertex1;
      gCopy->edges[e].sourceVertex2 = g->edges[e].sourceVertex2;
      gCopy->edges[e].sourceExample = g->edges[e].sourceExample;
      // GUI coloring
      gCopy->edges[e].originalEdgeIndex = g->edges[e].originalEdgeIndex;
      gCopy->edges[e].color = g->edges[e].color;
      gCopy->edges[e].anomalousValue = g->edges[e].anomalousValue;
   }

   return gCopy;
}


//******************************************************************************
// NAME:    FreeGraph
//
// INPUTS:  (Graph *graph) - graph to be freed
//
// RETURN:  void
//
// PURPOSE: Free memory used by given graph, including the vertices array
// and the edges array for each vertex.
//******************************************************************************

void FreeGraph(Graph *graph)
{
   ULONG v;

   if (graph != NULL) 
   {
      for (v = 0; v < graph->numVertices; v++)
         free(graph->vertices[v].edges);
      free(graph->edges);
      free(graph->vertices);
      free(graph);
   }
}


//******************************************************************************
// NAME:    PrintGraph
//
// INPUTS:  (Graph *graph) - graph to be printed
//          (LabelList *labelList) - indexed list of vertex and edge labels
//
// RETURN:  void
//
// PURPOSE: Print the vertices and edges of the graph to stdout.
//******************************************************************************

void PrintGraph(Graph *graph, LabelList *labelList)
{
   ULONG v;
   ULONG e;

   if (graph != NULL) 
   {
      printf("  Graph(%luv,%lue):\n", graph->numVertices, graph->numEdges);
      // print vertices
      for (v = 0; v < graph->numVertices; v++) 
      {
         printf("    ");
         PrintVertex(graph, v, labelList);
      }
      // print edges
      for (e = 0; e < graph->numEdges; e++) 
      {
         printf("    ");
         PrintEdge(graph, e, labelList);
      }
   }
}


//******************************************************************************
// NAME: PrintVertex
//
// INPUTS: (Graph *graph) - graph containing vertex
//         (ULONG vertexIndex) - index of vertex to print
//         (LabelList *labelList) - labels in graph
//
// RETURN: (void)
//
// PURPOSE: Print a vertex.
//******************************************************************************

void PrintVertex(Graph *graph, ULONG vertexIndex, LabelList *labelList)
{
   printf("v %lu ", vertexIndex + 1);
   PrintLabel(graph->vertices[vertexIndex].label, labelList);
   printf("\n");
}


//******************************************************************************
// NAME: PrintEdge
//
// INPUTS: (Graph *graph) - graph containing edge
//         (ULONG edgeIndex) - index of edge to print
//         (LabelList *labelList) - labels in graph
//
// RETURN: (void)
//
// PURPOSE: Print an edge.
//******************************************************************************

void PrintEdge(Graph *graph, ULONG edgeIndex, LabelList *labelList)
{
   Edge *edge = & graph->edges[edgeIndex];

   if (edge->directed)
      printf("d");
   else 
      printf("u");
   printf(" %lu %lu ", edge->vertex1 + 1, edge->vertex2 + 1);
   PrintLabel(edge->label, labelList);
   printf("\n");
}


//******************************************************************************
// NAME:    WriteGraphToFile
//
// INPUTS:  (FILE *outFile) - file stream to write graph
//          (Graph *graph) - graph to be written
//          (LabelList *labelList) - indexed list of vertex and edge labels
//          (ULONG vOffset) - vertex offset for compressed increments
//          (ULONG start) - beginning of vertex range to print
//          (ULONG finish) - end of vertex range to print
//          (BOOLEAN printPS) - flag indicating output is for predefined sub
//
// RETURN:  void
//
// PURPOSE: Write the vertices and edges of the graph to the given
//          file, prefaced by the SUB_TOKEN defined in gbad.h
//          (when WriteSubToken is TRUE).
//******************************************************************************

void WriteGraphToFile(FILE *outFile, Graph *graph, LabelList *labelList,
                      ULONG vOffset, ULONG start, ULONG finish, BOOLEAN printPS)
{
   ULONG v;
   ULONG e;
   Edge *edge = NULL;

   if (graph != NULL) 
   {
      if (printPS)
         fprintf(outFile, "%s\n", SUB_TOKEN);
      // write vertices
      for (v = start; v < finish; v++)
      {
         fprintf(outFile, "v %lu ", (v + 1 + vOffset - start));
         WriteLabelToFile(outFile, graph->vertices[v].label, labelList, FALSE);
         fprintf(outFile, "\n");
      }
      // write edges
      for (e = 0; e < graph->numEdges; e++)
      {
         edge = &graph->edges[e];
         if ((edge->vertex1 >= start) && (edge->vertex1 < finish))
         {
            if (edge->directed)
               fprintf(outFile, "d");
            else
               fprintf(outFile, "u");
            fprintf(outFile, " %lu %lu ",
               (edge->vertex1 + 1 + vOffset - start),
               (edge->vertex2 + 1 + vOffset - start));
            WriteLabelToFile(outFile, edge->label, labelList, FALSE);
            fprintf(outFile, "\n");
         }
      }
      if (printPS)
         fprintf(outFile, "\n");
   }
}

//////////////////////////////////////////////////// WFE - PLADS //////////////
//******************************************************************************
// NAME:    WriteNormGraphToFile
//
// INPUTS:  (ULONG bestSubNum) - number of best substructure
//          (Graph *graph) - graph to be written
//          (LabelList *labelList) - indexed list of vertex and edge labels
//          (ULONG vOffset) - vertex offset for compressed increments
//          (ULONG start) - beginning of vertex range to print
//          (ULONG finish) - end of vertex range to print
//          (ULONG numInstances) - number of instances of the graph
//          (ULONG partitionNum) - partition number
//          (ULONG bestNum) - number of best (normative) substructure
//          (char *graphInputFile) - name of graph input file
//
// RETURN:  void
//
// PURPOSE: Write the name of the input graph file, the size of the graph, the
//          the number of instances of the normative pattern, and the vertices 
//          and edges of the graph to the given file.
//******************************************************************************

void WriteNormGraphToFile(ULONG bestSubNum, Graph *graph, LabelList *labelList,
                          ULONG vOffset, ULONG start, ULONG finish, 
                          ULONG numInstances, ULONG partitionNum, ULONG bestNum,
                          char *graphInputFile)
{
   ULONG v;
   ULONG e;
   Edge *edge = NULL;
   FILE *outFile;
   char outFileName[FILE_NAME_LEN];     // file for normative graph

   // create output file name based upon partition number and best sub number
   char charNum[FILE_NAME_LEN];
   sprintf(charNum, "%lu", partitionNum);
   strcpy(outFileName, "norm_");
   strcat(outFileName,charNum);
   strcat(outFileName,"_");
   sprintf(charNum, "%lu", bestNum);
   strcat(outFileName,charNum);

   outFile = fopen(outFileName, "w");
   if (outFile == NULL)
   {
      printf("WARNING: unable to write normative pattern to output file %s,", 
             outFileName);
      exit(1);
   }

   if (graph != NULL) 
   {
      fprintf(outFile,"%% %s\n",graphInputFile);
      fprintf(outFile,"%% %lu %lu\n",(graph->numVertices + graph->numEdges),
              numInstances);
      // write vertices
      for (v = start; v < finish; v++)
      {
         fprintf(outFile, "v %lu ", (v + 1 + vOffset - start));
         WriteLabelToFile(outFile, graph->vertices[v].label, labelList, FALSE);
         fprintf(outFile, "\n");
      }
      // write edges
      for (e = 0; e < graph->numEdges; e++)
      {
         edge = &graph->edges[e];
         if ((edge->vertex1 >= start) && (edge->vertex1 < finish))
         {
            if (edge->directed)
               fprintf(outFile, "d");
            else
               fprintf(outFile, "u");
            fprintf(outFile, " %lu %lu ",
               (edge->vertex1 + 1 + vOffset - start),
               (edge->vertex2 + 1 + vOffset - start));
            WriteLabelToFile(outFile, edge->label, labelList, FALSE);
            fprintf(outFile, "\n");
         }
      }
   }

   fclose(outFile);
}


//******************************************************************************
// NAME:    WriteAnomGraphToFile
//
// INPUTS:  (Graph *graph) - anomalous graph to be written
//          (LabelList *labelList) - indexed list of vertex and edge labels
//          (ULONG vOffset) - vertex offset for compressed increments
//          (ULONG start) - beginning of vertex range to print
//          (ULONG finish) - end of vertex range to print
//          (ULONG partitionNum) - partition number
//          (ULONG instanceNum) - anomalous instance number in graph
//          (double anomScore) - score of anomalous instance
//
// RETURN:  void
//
// PURPOSE: Write the anomalous score and graph definition of the anomalous
//          instance to a file called "anom_<p>_<n>", where <p> is the graph
//          partition number and <n> is the instance number.
//******************************************************************************

void WriteAnomGraphToFile(Graph *graph, LabelList *labelList,
                          ULONG vOffset, ULONG start, ULONG finish, 
                          ULONG partitionNum, ULONG instanceNum,
                          double anomScore)
{
   ULONG v;
   ULONG e;
   Edge *edge = NULL;
   FILE *outFile;
   char outFileName[FILE_NAME_LEN];     // file for normative graph

   sprintf(outFileName, "anom_%lu_%lu", partitionNum, instanceNum);
   outFile = fopen(outFileName, "w");
   if (outFile == NULL)
   {
      printf("WARNING: unable to write anomalous instance (graph) to output file %s,",
             outFileName);
      exit(1);
   }

   if (graph != NULL) 
   {
      fprintf(outFile,"%% %f\n",anomScore);
      // write vertices
      for (v = start; v < finish; v++)
      {
         fprintf(outFile, "v %lu ", (v + 1 + vOffset - start));
         WriteLabelToFile(outFile, graph->vertices[v].label, labelList, FALSE);
         fprintf(outFile, "\n");
      }
      // write edges
      for (e = 0; e < graph->numEdges; e++)
      {
         edge = &graph->edges[e];
         if ((edge->vertex1 >= start) && (edge->vertex1 < finish))
         {
            if (edge->directed)
               fprintf(outFile, "d");
            else
               fprintf(outFile, "u");
            fprintf(outFile, " %lu %lu ",
               (edge->vertex1 + 1 + vOffset - start),
               (edge->vertex2 + 1 + vOffset - start));
            WriteLabelToFile(outFile, edge->label, labelList, FALSE);
            fprintf(outFile, "\n");
         }
      }
   }

   fclose(outFile);
}


//
// NOTE:  The following functions are needed by the gm tool.
//

//---------------------------------------------------------------------------
// NAME:    ReadGraph
//
// INPUTS:  (char *filename) - graph file to read from
//          (LabelList *labelList) - list of labels to be added to from graph
//          (BOOLEAN directed) - TRUE if 'e' edges should be directed
//
// RETURN:  (Graph *) - graph read from file
//
// PURPOSE: Parses graph file, checking for formatting errors, and builds
// all necessary structures for the graph, which is returned.  labelList
// is destructively changed to hold any new labels.
//---------------------------------------------------------------------------

Graph *ReadGraph(char *filename, LabelList *labelList, BOOLEAN directed)
{
   Graph *graph;
   FILE *graphFile;
   ULONG lineNo;             // Line number counter for graph file
   char token[TOKEN_LEN];
   ULONG vertexListSize = 0; // Size of currently-allocated vertex array
   ULONG edgeListSize = 0;   // Size of currently-allocated edge array
   ULONG vertexOffset = 0;   // Dummy argument to ReadVertex and ReadEdge

   // Allocate graph
   graph = AllocateGraph(0,0);

   // Open graph file
   graphFile = fopen(filename,"r");
   if (graphFile == NULL)
   {
      fprintf(stderr, "Unable to open graph file %s.\n", filename);
      exit(1);
   }

   // Parse graph file
   lineNo = 1;
   while (ReadToken(token, graphFile, &lineNo) != 0)
   {
      if (strcmp(token, "v") == 0)         // read vertex
         ReadVertex(graph, graphFile, labelList, &vertexListSize, &lineNo,
                    vertexOffset);

      else if (strcmp(token, "e") == 0)    // read 'e' edge
         ReadEdge(graph, graphFile, labelList, &edgeListSize, &lineNo, directed,
                  vertexOffset);

      else if (strcmp(token, "u") == 0)    // read undirected edge
         ReadEdge(graph, graphFile, labelList, &edgeListSize, &lineNo, FALSE,
                  vertexOffset);

      else if (strcmp(token, "d") == 0)    // read directed edge
         ReadEdge(graph, graphFile, labelList, &edgeListSize, &lineNo, TRUE,
                  vertexOffset);

      else
      {
         fclose(graphFile);
         FreeGraph(graph);
         fprintf(stderr, "Unknown token %s in line %lu of graph file %s.\n",
                 token, lineNo, filename);
         exit(1);
      }
   }
   fclose(graphFile);

   //***** trim vertex, edge and label lists

   return graph;
}


//---------------------------------------------------------------------------
// NAME:    ReadLabel
//
// INPUTS:  (FILE *fp) - file pointer from which label is read
//          (LabelList *labelList) - list of vertex and edge labels
//          (ULONG *pLineNo) - pointer to line counter in calling function
//
// RETURN:  (ULONG) - index of read label in global label list.
//
// PURPOSE: Reads a label (string or numeric) from the given file and
// stores the label in the given label list if not already there.
// Returns the label's index in the label list.
//---------------------------------------------------------------------------

ULONG ReadLabel(FILE *fp, LabelList *labelList, ULONG *pLineNo)
{
   char token[TOKEN_LEN];
   char *endptr;
   Label label;

   ReadToken(token, fp, pLineNo);
   label.labelType = NUMERIC_LABEL;
   label.labelValue.numericLabel = strtod(token, &endptr);
   if (*endptr != '\0')
   {
      label.labelType = STRING_LABEL;
      label.labelValue.stringLabel = token;
   }
   return StoreLabel(&label, labelList);
}


//---------------------------------------------------------------------------
// NAME:    ReadInteger
//
// INPUTS:  (FILE *fp) - file pointer from which number is read
//          (ULONG *pLineNo) - pointer to line counter in calling function
//
// RETURN:  (ULONG) - integer read
//
// PURPOSE: Read an unsigned long integer from the given file.
//---------------------------------------------------------------------------

ULONG ReadInteger(FILE *fp, ULONG *pLineNo)
{
   ULONG i;
   char token[TOKEN_LEN];
   char *endptr;

   ReadToken(token, fp, pLineNo);
   i = strtoul(token, &endptr, 10);
   if (*endptr != '\0')
   {
      fprintf(stderr, "Error: expecting integer in line %lu.\n",
              *pLineNo);
      exit(1);
   }
   return i;
}


//---------------------------------------------------------------------------
// NAME:    ReadToken
//
// INPUTS:  (char *token) - string into which token is copied
//          (FILE *fp) - file pointer from which token is read
//          (ULONG *pLineNo) - pointer to line counter in calling function
//
// RETURN:  (int) - length of token read
//
// PURPOSE: Read the next token from the given file.  A token is
// defined as a string of non-whitespace characters, where whitespace
// includes spaces, tabs, newlines, comments, and EOF.
//---------------------------------------------------------------------------

int ReadToken(char *token, FILE *fp, ULONG *pLineNo)
{
   char ch;
   int i = 0;

   // skip whitespace and comments
   ch = fgetc(fp);
   while ((ch == SPACE) || (ch == TAB) || (ch == CARRIAGERETURN) ||
    (ch == NEWLINE) || (ch == COMMENT))
   {
      if (ch == NEWLINE)
         (*pLineNo)++;
      if (ch == COMMENT)
      {
         while ((ch != NEWLINE) && (ch != (char)EOF))  // skip to end of line
            ch = fgetc(fp);
         if (ch == NEWLINE)
            (*pLineNo)++;
      }
      if (ch != (char)EOF)
         ch = fgetc(fp);
   }

   // read token
   if (ch == DOUBLEQUOTE)
   { // read until reaching another double quote
      do
      {
         token[i++] = ch;
         ch = fgetc(fp);
      } while ((ch != (char)EOF) && (ch != DOUBLEQUOTE));
      if (ch == DOUBLEQUOTE)
         token[i++] = ch;
      ch = fgetc(fp);
   }
   else
   { // read until reaching whitespace
      while ((ch != (char)EOF) && (ch != SPACE) && (ch != TAB) &&
             (ch != CARRIAGERETURN) && (ch != NEWLINE) && (ch != COMMENT))
      {
         token[i++] = ch;
         ch = fgetc(fp);
      }
   }
   token[i] = '\0';

   // if token ended by NEWLINE, increment lineNo
   if (ch == NEWLINE)
      (*pLineNo)++;

   // if token ended by comment, go ahead and skip comment
   if (ch == COMMENT)
   {
      while ((ch != NEWLINE) && (ch != (char)EOF))
         ch = fgetc(fp);
      if (ch == NEWLINE)
         (*pLineNo)++;
   }

   return i;
}


//---------------------------------------------------------------------------
// NAME:    ReadVertex
//
// INPUTS:  (Graph *graph) - pointer to graph being constructed
//          (FILE *fp) - pointer to graph file stream
//          (LabelList *labelList) - list of vertex and edge labels
//          (ULONG *vertexListSize) - pointer to size of graph's allocated
//                                    vertex array
//          (ULONG *pLineNo) - pointer to line counter in calling function
//          (ULONG vertexOffset) - offset to add to vertex numbers
//
// RETURN:  (void)
//
// PURPOSE: Read and check the vertex number and label, store label in
// given label list, and add vertex to graph.  A non-zero vertexOffset
// indicates this vertex is part of a graph beyond the first.
//--------------------------------------------------------------------------

void ReadVertex(Graph *graph, FILE *fp, LabelList *labelList,
                ULONG *vertexListSize, ULONG *pLineNo, ULONG vertexOffset)
{
   ULONG vertexID;
   ULONG labelIndex;

   // read and check vertex number
   vertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if (vertexID != (graph->numVertices + 1))
   {
      fprintf(stderr, "Error: invalid vertex number at line %lu.\n",
              *pLineNo);
      exit(1);
   }
   // read label
   labelIndex = ReadLabel(fp, labelList, pLineNo);

   AddVertex(graph, labelIndex, vertexListSize, 0);
}


//---------------------------------------------------------------------------
// NAME:    ReadEdge
//
// INPUTS:  (Graph *graph) - pointer to graph being constructed
//          (FILE *fp) - pointer to graph file stream
//          (LabelList *labelList) - list of vertex and edge labels
//          (ULONG *edgeListSize) - pointer to size of graph's allocated
//                                  edge array
//          (ULONG *pLineNo) - pointer to line counter in calling function
//          (BOOLEAN directed) - TRUE if edge is directed
//          (ULONG vertexOffset) - offset to add to vertex numbers
//
// RETURN:  (void)
//
// PURPOSE: Read and check the vertex numbers and label, store label in
// given label list, and add edge to graph.  A non-zero vertexOffset
// indicates the edge's vertices are part of a graph beyond the first.
//---------------------------------------------------------------------------

void ReadEdge(Graph *graph, FILE *fp, LabelList *labelList,
              ULONG *edgeListSize, ULONG *pLineNo, BOOLEAN directed,
              ULONG vertexOffset)
{
   ULONG sourceVertexID;
   ULONG targetVertexID;
   ULONG sourceVertexIndex;
   ULONG targetVertexIndex;
   ULONG labelIndex;

   // read and check vertex numbers
   sourceVertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if (sourceVertexID > graph->numVertices)
   {
      fprintf(stderr,
              "Error: reference to undefined vertex number at line %lu.\n",
              *pLineNo);
      exit(1);
   }
   targetVertexID = ReadInteger(fp, pLineNo) + vertexOffset;
   if (targetVertexID > graph->numVertices)
   {
      fprintf(stderr,
              "Error: reference to undefined vertex number at line %lu.\n",
              *pLineNo);
      exit(1);
   }
   sourceVertexIndex = sourceVertexID - 1;
   targetVertexIndex = targetVertexID - 1;

   // read and store label
   labelIndex = ReadLabel(fp, labelList, pLineNo);

   AddEdge(graph, sourceVertexIndex, targetVertexIndex, directed,
           labelIndex, edgeListSize, FALSE);
}


//******************************************************************************
// NAME: WriteAnomInstanceToFile
//
// INPUTS: (Instance *instance) - anomalous instance to print
//         (Graph *graph) - graph containing anomalous instance
//         (Parameters *parameters) - parameters from graph
//         (ULONG instanceNum) - anomalous instance number
//
// RETURN: (void)
//
// PURPOSE: Print given anomalous instance.
//******************************************************************************

void WriteAnomInstanceToFile(Instance *instance, Graph *graph, 
                             Parameters *parameters, ULONG instanceNum)
{
   FILE *outFile;
   char outFileName[FILE_NAME_LEN];     // file for normative graph
   LabelList *labelList = parameters->labelList;
   ULONG vertexIndex, edgeIndex;
   ULONG partitionNum = parameters->partitionNum;

   sprintf(outFileName, "anomInst_%lu_%lu", partitionNum, instanceNum);
   outFile = fopen(outFileName, "w");
   if (outFile == NULL)
   {
      printf("WARNING: unable to write anomalous instance to output file %s,",
             outFileName);
      exit(1);
   }

   if (instance != NULL)
   {
      ULONG i,j;
      for (i = 0; i < instance->numVertices; i++)
      {
         vertexIndex = instance->vertices[i];
         fprintf(outFile,"v %lu ", vertexIndex + 1);
         PrintLabelToFile(outFile,graph->vertices[vertexIndex].label, labelList);
         for (j = 0; j < instance->numAnomalousVertices; j++)
         {
            if (instance->anomalousVertices[j] == vertexIndex)
            {
               // Don't print the word "anomaly" next to every vertex when using
               // the MPS algorithm because the entire structure is anomalous
               if (!parameters->mps)
               {
                  fprintf(outFile," <-- anomaly");
                  // If original example has a value of 0, that means it is the
                  // first iteration and was never set (so output it as 1)
                  if (graph->vertices[vertexIndex].sourceExample == 0)
                     fprintf(outFile," (original vertex: %lu , in original example 1)",
                            graph->vertices[vertexIndex].sourceVertex);
                  else
                     fprintf(outFile," (original vertex: %lu , in original example %lu)",
                            graph->vertices[vertexIndex].sourceVertex,
                            graph->vertices[vertexIndex].sourceExample);
               }
               break;
            }
         }
         fprintf(outFile,"\n");
      }
      for (i = 0; i < instance->numEdges; i++)
      {
         edgeIndex = instance->edges[i];
         Edge *edge = & graph->edges[edgeIndex];

         if (edge->directed)
            fprintf(outFile,"d");
         else 
            fprintf(outFile,"u");
         fprintf(outFile," %lu %lu ", edge->vertex1 + 1, edge->vertex2 + 1);
         PrintLabelToFile(outFile,edge->label, labelList);

         for (j = 0; j < instance->numAnomalousEdges; j++)
         {
            if (instance->anomalousEdges[j] == edgeIndex)
            {
               // Don't print the word "anomaly" next to every vertex when using
               // the MPS algorithm because the entire structure is anomalous
               if (!parameters->mps)
               {
                  fprintf(outFile," <-- anomaly");
                  // If original example has a value of 0, that means it is the
                  // first iteration and was never set (so output it as 1)
                  if (graph->edges[edgeIndex].sourceExample == 0)
                     fprintf(outFile," (original edge vertices: %lu -- %lu, in original example 1)",
                            graph->edges[edgeIndex].sourceVertex1,
                            graph->edges[edgeIndex].sourceVertex2);
                  else
                     fprintf(outFile," (original edge vertices: %lu -- %lu, in original example %lu)",
                            graph->edges[edgeIndex].sourceVertex1,
                            graph->edges[edgeIndex].sourceVertex2,
                            graph->edges[edgeIndex].sourceExample);
               }
               break;
            }
         }
         fprintf(outFile,"\n");
      }
   }
   fclose(outFile);
}
//////////////////////////////////////////////////// WFE - PLADS //////////////
