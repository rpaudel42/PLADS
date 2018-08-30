# Calculate Sole & Valverde, 2004 graph entropy
# Uses Equations 1 and 4
# First we need the denominator of q(k)
# To get it we need the probability of each degree
# First get the number of nodes with each degree

library(sna)

args <- commandArgs(TRUE)

edgesFile <- paste("edges_",args[1],".csv",sep="")

e1 <- read.csv(edgesFile, header=FALSE,sep=",")

adj <- table(e1)

existingDegrees = degree(adj)/2

maxDegree = nrow(adj) - 1

allDegrees = 0:maxDegree
degreeDist = matrix(0, 3, length(allDegrees)+1) # Need an extra zero prob degree for later calculations
degreeDist[1,] = 0:(maxDegree+1)
for(aDegree in allDegrees) 
{
   degreeDist[2,aDegree+1] = sum(existingDegrees == aDegree)
}

# Calculate probability of each degree
for(aDegree in allDegrees) 
{
   degreeDist[3,aDegree+1] = degreeDist[2,aDegree+1]/sum(degreeDist[2,])
}

# Sum of all degrees mult by their probability
sumkPk = 0
for(aDegree in allDegrees) 
{
   sumkPk = sumkPk + degreeDist[2,aDegree+1] * degreeDist[3,aDegree+1]
}

# Equivalent is sum(degreeDist[2,] * degreeDist[3,])
# Now we have all the pieces we need to calculate graph entropy
graphEntropy = 0
for(aDegree in 1:maxDegree) 
{
   q.of.k = ((aDegree + 1)*degreeDist[3,aDegree+2])/sumkPk
   # 0 log2(0) is defined as zero
   if (q.of.k != 0) 
   {
      graphEntropy = graphEntropy + -1 * q.of.k * log2(q.of.k)
   }
}

outputFile <- paste("entropy_",args[1],".txt",sep="")

#sprintf("graph entropy:  %f", graphEntropy)
write(graphEntropy,outputFile)

