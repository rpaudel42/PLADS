#!/usr/bin/env python

#
# Using SNAP tool to generate CLUSTERING COEFFICIENT for an undirected graph
#

from snap import *
import snap

import sys

# create a graph PUNGraph (undirected graph)
G1 = TUNGraph.New()

# read in vertices
textFile = open('vertices_' + str(sys.argv[1]) + '.txt')
lines = textFile.readlines()
for line in lines:
    stripped_line = line.rstrip('\n')
    G1.AddNode(int(stripped_line))

# read in edges
with open('edges_' + str(sys.argv[1]) + '.txt') as f:
    for line in f:
        int_list = [int(i) for i in line.split()]
        G1.AddEdge(int_list[0],int_list[1])

GraphClustCoeff = snap.GetClustCf(G1, -1)

strValue = str.format("{0:.10f}", GraphClustCoeff)

f = open('clustering_' + str(sys.argv[1]) + '.txt', 'w')
f.write(str(strValue))

