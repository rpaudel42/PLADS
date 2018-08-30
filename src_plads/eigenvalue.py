#!/usr/bin/env python

#
# Using SNAP tool to generate MAXIMUM EIGENVALUE for an undirected graph
#

from snap import *
import snap

import sys

# create a graph PUNGraph (undirected graph)
G1 = TUNGraph.New()

# read in vertices
numVertices = 0
textFile = open('vertices_' + str(sys.argv[1]) + '.txt')
lines = textFile.readlines()
for line in lines:
    stripped_line = line.rstrip('\n')
    G1.AddNode(int(stripped_line))
    numVertices = numVertices + 1

# read in edges
with open('edges_' + str(sys.argv[1]) + '.txt') as f:
    for line in f:
        int_list = [int(i) for i in line.split()]
        G1.AddEdge(int_list[0],int_list[1])

EigVals = 1
PEigV = snap.TFltV()
snap.GetEigVals(G1, EigVals, PEigV)
for item in PEigV:
    strValue = str.format("{0:.10f}", item)

f = open('eigenvalue_' + str(sys.argv[1]) + '.txt', 'w')
f.write(str(strValue))
