
import ROOT
import copy
#ROOT.gInterpreter.GenerateDictionary("std::set<std::string>","set")
ROOT.gSystem.Load('../lib/libEXOEnergy.so')

infoRuns = ROOT.EXOSourceRunsPolishedInfo('../data/UpdatedLivetimeSourceRunsInfo.txt')
#infoRuns.CutExact('SourceStrength','weak')
sources = infoRuns.GetSetOf('SourceName')
sourceWeeks = {}

for source in sources:
    sourceRuns = copy.copy(infoRuns)# ROOT.EXOSourceRunsPolishedInfo('../data/UpdatedLivetimeLJSourceRunsInfo.txt')
    sourceRuns.CutExact('SourceName',source)
    weeks = sourceRuns.GetSetOf('WeekIndex')
    sourceWeeks[source] = []
    for week in weeks:
        sourceWeeks[source].append(int(week))

for source in sourceWeeks:
    print source, sorted(sourceWeeks[source])
