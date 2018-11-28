
import ROOT
import sys

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

fv_wires,fv_min_z,fv_max_z = 162, 10, 182
cutName = 'fv_%i_%i_%i' % (fv_wires,fv_min_z,fv_max_z)

def GetInfoRuns(filename,isLJ = False):
    infoRuns = ROOT.EXOSourceRunsPolishedInfo(filename)

    if not isLJ:
        #Cs at S2, no simulation...
        infoRuns.CutExact('RunNumber','2410',False)
        infoRuns.CutExact('RunNumber','2450',False)
        #Processing problem, should be fixed
        infoRuns.CutExact('RunNumber','5822',False)

        infoRuns.CutExact('SourcePositionS','0',False)
        infoRuns.CutExact('SourcePositionS','17',False)
        infoRuns.CutExact('ManualFlag','NONE')
        infoRuns.CutExact('Quality','BAD',False)
        infoRuns.AddExactCondition('SourceName','Cs-137',True)
        infoRuns.AddExactCondition('SourceName','Ra-226',True)
        infoRuns.CutDoubleComparison('Purity',2.0,True)
        infoRuns.AddExactCondition('SourceName','Co-60',True)
        infoRuns.CutDoubleComparison('Purity',9.9,False)
        infoRuns.ClearExceptRuns()
        infoRuns.CutDoubleComparison('RunNumber',5900,False)

    return infoRuns

infoRuns = GetInfoRuns('../data/UpdatedLivetimeSourceRunsInfo.txt',False)
ljInfoRuns = GetInfoRuns('../data/UpdatedLivetimeLJSourceRunsInfo.txt',True)

runList = infoRuns.GetListOf('RunNumber')
ljRunList = ljInfoRuns.GetListOf('RunNumber')
print runList.size(), ljRunList.size()

for run in runList:
    if not run in ljRunList:
        print run
print 'other way'
for run in ljRunList:
    if not run in runList:
        print run

ljLT = ljInfoRuns.GetSetOf('CampaignLivetime')
lt = infoRuns.GetSetOf('CampaignLivetime')
for l in lt:
    print l
for l in ljLT:
    print l
