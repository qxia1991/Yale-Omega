
import ROOT, settings
import os, time

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

settings.init()

infoRuns = ROOT.EXOSourceRunsPolishedInfo(settings.SourceRunsFileName)

#infoRuns.CutDoubleComparison('WeekLivetime',1e-15,True)

infoRuns.CutDoubleComparison("WeekIndex",settings.minWeek,True)
infoRuns.CutDoubleComparison("WeekIndex",settings.maxWeek,False)

allWeeks = infoRuns.GetListOf('WeekIndex')
weeks = set([])
for i in range(allWeeks.size()):
    #if int(allWeeks.at(i)) > 7 or int(allWeeks.at(i)) < 5:
    #    continue
    weeks.add(allWeeks.at(i))

multiplicity = ['ss','ms']

energy = '2458.0'

body = """
source /nfs/slac/g/exo_data4/users/maweber/software/Devel/setup.sh;
cd /nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOEnergy/scripts/;
python FitWeeklyAngle.py --week %s --mult %s --energy %s %s > %s 2> %s
"""

nonDenoised = ''
if not settings.isDenoised:
	nonDenoised = '--non-denoised'

for week in sorted(weeks):
    outDir = settings.AngleFitterOutput
    
    for mult in multiplicity:
        filename = '%s/AngleFit_week_%s_%s.sh'%(outDir,week,mult)
        file = open(filename,'w')
        file.write(body % (week,mult,energy,nonDenoised,filename.replace('.sh','.out'),filename.replace('.sh','.err')))
        file.close()
        cmd = 'chmod 755 %s' % (filename)
        os.system(cmd)
        cmd = 'bsub -R rhel60 -q long %s' % (filename)
        print cmd
        os.system(cmd)
