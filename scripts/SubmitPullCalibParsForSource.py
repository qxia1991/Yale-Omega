
import os, time

scriptBody = """
cd /u/xo/licciard/my_exodir/exo_data4_users/Energy/software/rhel6-64/Devel/;
source setup.sh;
cd EXOEnergy/scripts;
python PullCalibParsForSource.py %i %i %s >& %s
"""

outDir = '/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/results/pulls/'

for seedP in range(10):
    for seedD in range(50):
        outName = '%s/pulls_thcora_true3pars_calib_seed_p%i_d%i' % (outDir,seedP,seedD)
        scriptFile = open(outName+'.sh','w')
        scriptFile.write(scriptBody % (seedP, seedD, outName+'.root', outName+'.out'))
        scriptFile.close()

        cmd = 'chmod 755 %s' % (outName+'.sh')
        os.system(cmd)
        cmd = 'bsub -R rhel60 -q medium %s' % (outName+'.sh')
        print cmd
        os.system(cmd)
        time.sleep(1)
