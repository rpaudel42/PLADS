#!/usr/bin/env python

#
# Using SNAP tool to generate CONNECTEDNESS for an undirected graph
#

from snap import *
import snap

import sys

#print 'Argument 0:', str(sys.argv[0])
#print 'Argument 1:', str(sys.argv[1])

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

Components = snap.TCnComV()
snap.GetSccs(G1, Components)
#for CnCom in Components:
#    print "Size of component: %d" % CnCom.Len()

total = 0
ComponentDist = snap.TIntPrV()
snap.GetSccSzCnt(G1, ComponentDist)
for comp in ComponentDist:
    #print "Size: %d - Number of Components: %d" % (comp.GetVal1(), comp.GetVal2())
    total = total + comp.GetVal2()

connectedness = total / (numVertices * numVertices * 1.0)

strValue = str.format("{0:.10f}", connectedness)

f = open('connectedness_' + str(sys.argv[1]) + '.txt', 'w')
f.write(str(strValue))
