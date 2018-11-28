
import ROOT, settings
import os, time

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

settings.init()

infoRuns = ROOT.EXOSourceRunsPolishedInfo(settings.SourceRunsFileName)

#infoRuns.CutDoubleComparison('WeekLivetime',1e-15,True)

infoRuns.CutDoubleComparison('WeekIndex',settings.minWeek,True)
infoRuns.CutDoubleComparison('WeekIndex',settings.maxWeek,False)

allWeeks = infoRuns.GetListOf('WeekIndex')

channel = settings.calibrationChannel #'Scintillation'#'Ionization' #'Rotated'
if not channel:
	channel = 'Rotated'
weeks = set([])
for i in range(allWeeks.size()):
    #if int(allWeeks.at(i)) > 7 or int(allWeeks.at(i)) < 5:
    #    continue
    weeks.add(allWeeks.at(i))
print channel, weeks

#body = """
#source /nfs/slac/g/exo_data4/users/maweber/software/Devel/setup.sh;
#cd /nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOEnergy/scripts/;
#python CalibrateEnergy.py --week %s --channel %s > %s 2> %s
#"""

body = """
source /nfs/slac/g/exo_data4/users/maweber/software/Devel/setup.sh;
cd /nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOEnergy/scripts/;
python CalibrateEnergy.py --week %s --channel %s > %s 2> %s
"""


for week in sorted(weeks):
    outResDir = settings.CalibrationOutput
    filename = '%s/fit_week_%s.sh'%(outResDir,week)
    file = open(filename,'w')
    file.write(body % (week,channel,filename.replace('.sh','.out'),filename.replace('.sh','.err')))
    file.close()
    cmd = 'chmod 755 %s' % (filename)
    os.system(cmd)
    cmd = 'bsub -R rhel60 -W 59 %s' % (filename)
    print cmd
    os.system(cmd)
    #time.sleep(1)
    #ocmd = 'source %s' % (filename)
    #print ocmd
    #os.system(ocmd)
