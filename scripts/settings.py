# This file contains variables that are common to all scripts
#
# Calibration:	Phase 2 data. Only 12kV (run number > 7104).
#		The data includes:
#			- changed V-wire threshold
#			- new light map
#			- new wire gains
#			- new fiducial volume to cut the edges of the hex volume
#		Phase 2 MC.
#		This is the final calibration of the Phase2 data including data up to June 1. 2017
#
# Created:	06-12-17

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
  global fiducial3			# Max Z of the fiducial volume
  global fiducial4			# Max R of the fiducial volume
  global cutName			# Cut name for fiducial volume
  global minWeek			# Weeks greater than this number are selected
  global maxWeek			# Weeks greater than this number are cut
  global calibFlavor1			# Weekly calibration flavor
  global calibFlavor2			# Average calibration flavor
  global calibFlavor3			# Dummy value
  global diagonalCut			# Diagonal cut flavor
  global calibrationChannel		# Calibration channel
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
  fiducial4 = 173.
  cutName = 'fv_%i_%i_%i_%i'%(fiducial1,fiducial2,fiducial3,fiducial4)
  SourceRunsFileName = '../data/SourceRunsInfo_Phase2PreRC_20170611.txt'
  RecoFileName = '/nfs/slac/g/exo_data3/exo_data/data/WIPP/processed/[RunNumber]/proc0000[RunNumber]-*.root'
  DataTreeFileName = '/nfs/slac/g/exo_data6/groups/Energy/data/WIPP/preprocessed/2017_Phase2_061217/AfterCalibration/run_[RunNumber]_tree.root'
  SelectionTreeFileName = '/nfs/slac/g/exo_data6/groups/Energy/data/WIPP/selection/2017_Phase2_061217/AfterCalibration/%s/run_[RunNumber]_tree.root'%(cutName)
  DataMultiplicityWildcard = '[MULTIPLICITY]'
  DataId = 'run_[RunNumber]_[MULTIPLICITY]'
  MCTreeFileName = '/nfs/slac/g/exo_data6/groups/Fitting/data/MC/preprocessed/2017_Phase2_v3/all/'
  MCSelectionFileName = '/nfs/slac/g/exo_data6/groups/Fitting/data/MC/selection/2017_Phase2_v3/all/fv_162_10_182_173/'
  minWeek = 244
  maxWeek = 298
  calibFlavor1 = '2017_Phase2_v4_0nu_weekly'
  calibFlavor2 = '2017-v1-average'
  calibFlavor3 = 'dummy'
  diagonalCut = 'phase2_v1'
  calibrationChannel = 'Rotated'
  applyZCorrection = False
  ZCorrectionFlavor = 'vanilla'
  CalibrationOutput = '/nfs/slac/g/exo_data4/users/Energy/data/results/calibration/2017_Phase2_061217/FullRange/rotated/%s/'%(cutName)
  AngleFitterOutput = '/nfs/slac/g/exo_data4/users/Energy/data/results/angle/2017_Phase2_061217/'
  isDenoised = False
  useAngleFile = True
  angleFile = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOEnergy/scripts/WeeklyAngle_Phase2_061217.txt'
  FitRange = [1200,6000,1200,6000] #1200,6000,1200,6000
  IgnoreLimit = None # [3700,4000]
  ShapeAgreeCalibFlavor = '2016-v3-weekly'
  CalibType = 'energy-msbased-fit'
  ShapeAgreeRange = [800,4000,300,4000]
  ShapeAgreeOutput = '/nfs/slac/g/exo_data4/users/maweber/software/Devel/EXOEnergy/scripts/srcAgreePlots_061616.root'

