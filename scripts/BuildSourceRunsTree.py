
import ROOT, settings

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

# Initialize the settings file
settings.init()

# Define the source runs info file
infoRuns = ROOT.EXOSourceRunsPolishedInfo(settings.SourceRunsFileName)

# Apply cuts on the list of runs
infoRuns.CutDoubleComparison('WeekIndex',settings.minWeek,True)
infoRuns.CutDoubleComparison('WeekIndex',settings.maxWeek,False)

# Define input (reconstructed) and output (preprocessed) files
infoRuns.SetMetadata('RecoFileName',settings.RecoFileName)
infoRuns.SetMetadata('DataTreeFileName',settings.DataTreeFileName)

# Specify the path to the setup file
setupScript = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/setup.sh'
pythonName = 'python'#'wrap_python.sh'

# Specify path to the EXOFitting library
libEXOFittingScriptDir = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOFitting/EXO_Fitting/scripts/'

submitCommand = 'bsub -R rhel60 -q long [SCRIPT_NAME]'#'qsub [SCRIPT_NAME]'
submitCommandScriptWildcard = '[SCRIPT_NAME]'
submitJobs = True

# Build prepsocessed trees
ROOT.EXOEnergyUtils.CreateSourceDataTree(infoRuns,setupScript,pythonName,settings.applyZCorrection,settings.isDenoised,settings.ZCorrectionFlavor,settings.calibFlavor1,settings.calibFlavor2,settings.calibFlavor3,libEXOFittingScriptDir,submitCommand,submitCommandScriptWildcard,submitJobs)
