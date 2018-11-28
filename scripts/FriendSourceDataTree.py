
# This script submit jobs to create files with selected data and MC variables to be used in MC-based fits
import ROOT
import settings_2017_Phase1_v2 as settings
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
#infoRuns.SetMetadata('DataTreeFileName',settings.DataTreeFileName)
infoRuns.SetMetadata('DataMultiplicityWildCard',settings.DataMultiplicityWildcard)
infoRuns.SetMetadata('DataId',settings.DataId)
infoRuns.SetMetadata('SelectionTreeFileName',settings.SelectionTreeFileName)
infoRuns.SetMetadata('FriendTreeFileName',settings.FriendTreeFileName)

prepDir = '/nfs/slac/g/exo_data4/users/wcree/phase_1_new_analysis/prep'
prepName = 'prep_none_bb0n_blinded_6500.wsp.root'
weightsDir = '/nfs/slac/g/exo_data4/users/wcree/phase_1_new_analysis/train/weights'
factoryName = 'BDT_none_bb0n_blinded_6500'

# Specify the path to the setup file
#setupScript = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/setup.sh'#_root5.28.sh'
setupScript = '/nfs/slac/g/exo_data4/users/Fitting/software/Stable/setup.sh'#_root5.28.sh'
pythonName = 'python'#'wrap_python.sh'

# Specify the path to the EXOFitting scripts
#scriptDir = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOFitting/EXO_Fitting/scripts/'
scriptDir = '/nfs/slac/g/exo_data4/users/Fitting/software/Stable/EXO_Fitting/EXO_Fitting/scripts/'
submitCommand = 'bsub -R rhel60 -W 59 [SCRIPT_NAME]'#'qsub [SCRIPT_NAME]'
submitCommandScriptWildcard = '[SCRIPT_NAME]'

# Build options
isData = True
submitJob = True

# Build the selection trees
#ROOT.EXOEnergyUtils.CutSourceDataTree(infoRuns,setupScript,pythonName,isRandomTrigger,settings.diagonalCut,settings.fiducial1,settings.fiducial2,settings.fiducial3,scriptDir,submitCommand,submitCommandScriptWildcard,not isData,True)
ROOT.EXOEnergyUtils.FriendSourceDataTree(infoRuns,setupScript,pythonName,prepDir,prepName,weightsDir,factoryName,scriptDir,submitCommand,submitCommandScriptWildcard,not isData,submitJob)
