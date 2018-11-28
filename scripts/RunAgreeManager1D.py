import ROOT
import os,sys,code
from optparse import OptionParser
scrdir =  os.path.dirname(os.path.realpath(__file__)) # find this file. then go to its directory
edir = os.path.dirname(scrdir)

if __name__ == "__main__":

    usage = "usage: python RunAgreemanager1D.py [options]"
    parser = OptionParser(usage)
    
    parser.add_option("-d","--software-dir", nargs=1, 
                      default=edir)
    parser.add_option("-w","--workspace", nargs=2)
    parser.add_option("-s","--source-run-list", nargs=1,
                      default='data/UpdatedLivetimeSourceRunsInfo.txt')
    parser.add_option("-o","--output-dir", nargs=1,
                      help = 'directory is forced to exist along with any parent directories before starting')
    parser.add_option("-f","--production-file", nargs=1, default='')
    parser.add_option("-p","--production", nargs=1, 
                      default='2016_Xe134_Average_fv_162_10_182_with3dcut')
    parser.add_option("-b","--batch-mode", action='store_true')
    parser.add_option("-l","--legends", nargs=1, default='15')
    parser.add_option("-v","--varlist",nargs=1,type ='string',action='append')
    parser.add_option("-i","--interact", default=False, action = 'store_true')
    parser.add_option("--cutexact",type="string", action='append',
                      help = 'cutting run list for equality')
    parser.add_option("--cutdouble",type="string", action='append',
                      help = 'cutting run list for comparisons')
    parser.add_option("--userlimits",type = "string", default = "", nargs = 1,
                      help= "for setting new limits on observables that aren't energy --  to check sub range shape agreements, energy in string crashes") 
    parser.add_option("--userenergycut",type = "string", default = "", nargs = 1,
                      help= "for limiting energy -- format l1,h1:l2,h2:... -- in lb subtraction widens to full edge bins -- in source histos needs energy_smear precomputed and doesn't quite do the right thing as has leaky edges from resmearing on weekly basis") 
    parser.add_option("-n", "--oldperiods", default=False, action='store_true')
    parser.add_option("-c", "--extracalib", nargs=4, default='')
    parser.add_option("-r", "--runlist", nargs=1, default=None)

    options,args = parser.parse_args()   
    print 'Using options:', options
    print type(options)
    print "Options:", options.production_file

    if not options.output_dir:
        print 'Missing output directory...'
        parser.print_help()
        sys.exit()

    if options.batch_mode:
        ROOT.gROOT.SetBatch(ROOT.kTRUE)

    ROOT.gSystem.Load('{software_dir}/lib/libEXOEnergy.so'.format(**options.__dict__))
    ROOT.RooMsgService.instance().setGlobalKillBelow(ROOT.RooFit.WARNING)

    infoRuns = ROOT.EXOSourceRunsPolishedInfo('{software_dir}/{source_run_list}'.format(**options.__dict__))
    infoRuns.SetWiresNoiseLevelPeriods() #on the fly metadata for noise periods
    if options.runlist != None:
        print "Using Explicit Run List"
        runs = ROOT.std.vector('string')()
        for run in  options.runlist.split(","):
            runs.push_back(run)
        infoRuns.CutExactList("RunNumber", runs)

    if options.workspace:
        wspFileName = options.workspace[0]
        wspName = options.workspace[1]

    cmd = 'mkdir -p {output_dir}'.format(**options.__dict__)
    print cmd
    os.system(cmd)
    outDir = options.output_dir

    print infoRuns.GetSourceRunsInfoFileName()
    print "WSP FILE", wspFileName
    print "WSP NAME", wspName
    print "Check Prod file:", options.production_file

    sam = ROOT.EXOSourceAgreementManager1D(
        infoRuns, wspFileName, wspName, options.production_file)
    sam.SetProduction(options.production)
    sam.SetUserLimitsCut(options.userlimits)
    sam.SetUserEnergyCut(options.userenergycut)

    if options.oldperiods:
        print "Using old periods"
        sam.SetUseNoisePeriods(False)
    else:
        infoRuns.SetWiresNoiseLevelPeriods()

    if options.varlist != None : 
        c_vec = ROOT.std.vector("string")() ## whohoo lets make a c++ vector out of that list
        [c_vec.push_back(x) for x in options.varlist]
        sam.LimitObs(c_vec)
    sam.SetStats(int(options.legends))
    if options.cutexact != None : 
        for uc in options.cutexact : 
            sam.UsrCutExact(uc)
    if options.cutdouble != None : 
        for uc in options.cutdouble : 
            print "User Cut given", uc
            sam.UsrCutDoubleComparison(uc)
            #raw_input()
    if options.extracalib != '':
        print "Extra Calib:", options.extracalib
        aSS = float(options.extracalib[0])
        bSS = float(options.extracalib[1])
        aMS = float(options.extracalib[2])
        bMS = float(options.extracalib[3])
        sam.SetExtraCalib(aSS, bSS, aMS, bMS)
    if options.interact : 
        sam.InteractivePrep(outDir)
        code.interact(
            banner="Entering interactive session",local=locals())
    else :
        print "Running Agreement"
        sam.RunAgreement(outDir)
