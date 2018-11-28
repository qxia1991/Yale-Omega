
import ROOT
import sys

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

inputFileName = sys.argv[1]
inputTreeName = sys.argv[2]
outputFileName = sys.argv[3]
cut = sys.argv[4]

if len(sys.argv) > 5:
    ROOT.EXOEventSummary.SetDiagonalCutDBFlavor(sys.argv[5])

print 'Opening ROOT file', inputFileName
inputFile = ROOT.TFile(inputFileName,'read')
inputTree = inputFile.Get(inputTreeName)

if not inputTree:
    print 'TTree', inputTreeName, 'not found, exitting...'
    sys.exit(1)

outputFile = ROOT.TFile(outputFileName,'recreate')
print 'Copying selection TTree to', outputFileName
outputTree = inputTree.CopyTree(cut)

outputFile.cd()
outputTree.Write()
outputFile.Close()

print 'Closing files...'
inputFile.Close()
