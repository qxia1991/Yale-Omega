# This file contains variables that are common to all scripts
#
# Calibration:  Used for the calibraiton of the Xe134 MC.
#               The data is run2abcd
# 
# Created:      02-21-16

def init():
  global SourceRunsFileName		# Filename of the polished source info file
  global RecoFileName			# Path to the processed files
  global DataTreeFileName		# Path to the preprocessed files
  global SelectionTreeFileName		# Path to the selection file names
  global DataMultiplicityWildcard	# Multiplicity wildcard for selection files
  global DataId				# Name scheme for the data files
  global MCTreeFileName			# Path to the preprocessed MC files
  global MCSelectionFileName		# Path to the selection MC files
  global fiducial1			# Apothem of the fiducial volume
  global fiducial2			# Min Z of the fiducial volume
  global fiducial3			# Max Z of the fiducial voluem
  global cutName			# Cut name for fiducial volume
  global minWeek			# Weeks greater than this number are selected
  global maxWeek			# Weeks greater than this number are cut
  global calibFlavor1			# Weekly calibration flavor
  global calibFlavor2			# Average calibration flavor
  global calibFlavor3			# Dummy value
  global diagonalCut			# Diagonal cut flavor
  global applyZCorrection		# Apply Z-correction?
  global ZCorrectionFlavor		# Z-correction flavor
  global CalibrationOutput		# Output path for the calibration results
  global AngleFitterOutput		# Output path for the rotation angle fitter
  global isDenoised			# Flag indicating whether the data is denoised
  global useAngleFile			# Specify whether to read the rotation angle from file
  global angleFile			# Path to the file containing rotation angles
  global FitRange			# Fit range in units of rotated energy [low_ss, high_ss, low_ms, high_ms]
  global IgnoreLimit			# Energy range to be ignored
  global ShapeAgreeCalibFlavor		# Calibration flavor to be used for the source agreement study
  global CalibType			# Calibration type. Usually energy-mcbased-fit
  global ShapeAgreeRange		# Energy range for shape agreement study [low_ss, high_ss, low_ms, high_ms]
  global ShapeAgreeOutput		# Path to the output file of the shape agreement study

  fiducial1 = 162.
  fiducial2 = 10.
  fiducial3 = 182.
  cutName = 'fv_%i_%i_%i'%(fiducial1,fiducial2,fiducial3)
  SourceRunsFileName = '../data/UpdatedLivetimeSourceRunsInfo_20160616.txt'
  RecoFileName = '/nfs/slac/g/exo_data6/exo_data/data/WIPP/DN_Run2abcd_All/[RunNumber]/denoised0000[RunNumber]-*.root'
  DataTreeFileName = '/nfs/slac/g/exo_data6/groups/Energy/data/WIPP/preprocessed/2016_Xe134_NewDN/ForCalibration/Xe134_run_[RunNumber]_tree.root'
  SelectionTreeFileName = '/nfs/slac/g/exo_data4/users/Energy/data/WIPP/selection/2016_Xe134_NewDN/ForCalibration/%s/Xe134_run_[RunNumber]_tree.root'%(cutName)
  DataMultiplicityWildcard = '[MULTIPLICITY]'
  DataId = 'Xe134_run_[RunNumber]_[MULTIPLICITY]'
  MCTreeFileName = '/nfs/slac/g/exo_data6/groups/Fitting/data/MC/preprocessed/2015_Xe134/'
  MCSelectionFileName = '/nfs/slac/g/exo_data6/groups/Fitting/data/MC/selection/2015_Xe134/%s/'%(cutName)
  minWeek = 1
  maxWeek = 125
  calibFlavor1 = '2015-v3-weekly'
  calibFlavor2 = '2015-v3-average'
  calibFlavor3 = 'dummy'
  diagonalCut = '2013-0nu-denoised'
  applyZCorrection = True
  ZCorrectionFlavor = 'linear-2015-v1'
  CalibrationOutput = '/nfs/slac/g/exo_data4/users/Energy/data/results/calibration/2016_Xe134_newDN/rotated/%s/'%(cutName)
  AngleFitterOutput = ''
  isDenoised = True
  useAngleFile = False
  angleFile = ''
  FitRange = [750,5000,400,5000]
  IgnoreLimit = None # [5000,5200]
  ShapeAgreeCalibFlavor = '2016-v1-weekly'
  CalibType = 'energy-msbased-fit'
  ShapeAgreeRange = [800,4000,300,4000]
  ShapeAgreeOutput = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOEnergy/scripts/srcAgreePlots_2016-Xe134_newDN.root'

