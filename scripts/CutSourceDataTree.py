
# This script submit jobs to create files with selected data and MC variables to be used in MC-based fits
import ROOT, settings
import sys

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

# Initialize the settings file
settings.init()

# Define the source runs info file
infoRuns = ROOT.EXOSourceRunsPolishedInfo(settings.SourceRunsFileName)

# Apply cuts on the list of runs
infoRuns.CutDoubleComparison('WeekIndex',settings.minWeek,True)
infoRuns.CutDoubleComparison('WeekIndex',settings.maxWeek,False)

# Define input (preprocessed) and output (selection) files
infoRuns.SetMetadata('DataTreeFileName',settings.DataTreeFileName)
infoRuns.SetMetadata('DataMultiplicityWildCard',settings.DataMultiplicityWildcard)
infoRuns.SetMetadata('DataId',settings.DataId)
infoRuns.SetMetadata('SelectionTreeFileName',settings.SelectionTreeFileName)

infoRuns.SetMCTreeFileNames(settings.MCTreeFileName)
infoRuns.SetMCSelectionFileNames(settings.cutName,settings.MCSelectionFileName,settings.DataMultiplicityWildcard)

# Specify the path to the setup file
setupScript = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/setup.sh'#_root5.28.sh'
pythonName = 'python'#'wrap_python.sh'

# Specify the path to the EXOFitting scripts
scriptDir = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOFitting/EXO_Fitting/scripts/'
submitCommand = 'bsub -R rhel60 -q long [SCRIPT_NAME]'#'qsub [SCRIPT_NAME]'
submitCommandScriptWildcard = '[SCRIPT_NAME]'

# Build options
isData = True
isRandomTrigger = True

fiducial4 = -1
if hasattr(settings, 'fiducial4'):
	fiducial4 = settings.fiducial4
	
# Build the selection trees
ROOT.EXOEnergyUtils.CutSourceDataTree(infoRuns,setupScript,pythonName,isRandomTrigger,settings.diagonalCut,settings.fiducial1,settings.fiducial2,settings.fiducial3,fiducial4,scriptDir,submitCommand,submitCommandScriptWildcard,not isData,True)
