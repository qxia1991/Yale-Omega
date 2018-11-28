
import ROOT

ROOT.gSystem.Load('libEXOCalibUtilities')

startTime = ROOT.TTimeStamp(2013,05,25,0,0,0,True)#2011, 9, 16, 11, 0, 0, 0, True)
#db = ROOT.EXOCalibManager.GetCalibManager().getCalib(ROOT.EXOEnergyMCBasedFitHandler.GetHandlerName(),"2014-v1-weekly-h",startTime.GetSec())
#db = ROOT.EXOCalibManager.GetCalibManager().getCalib(ROOT.EXOEnergyZCorrectionHandler.GetHandlerName(),"vanilla",startTime.GetSec())
#print db

flavor = "linear-2014-v1"
#zCorr = ROOT.EXOEnergyZCorrection()
print ROOT.EXOEnergyZCorrection.GetZcorrection(0,flavor,startTime.GetSec(),0), ROOT.EXOEnergyZCorrection.GetZcorrection(-0.0000001,flavor,startTime.GetSec(),0)

#print db.GetParByName('Angle',1), db.GetParByName('Multiplicity',1), db.GetParByName('WeekIndex',1), db.GetParByName('Rotated_Calibration_p1',2)

#userVals = ROOT.std.map('string','double')(
#userVals['Angle'] = 1.
#ROOT.EXOEnergyMCBasedFit.SetUserValues(userVals,2)
#ROOT.EXOEnergyMCBasedFit.UseDatabase(False)
#print db.GetParByName('Angle',2)

#ROOT.EXOEnergyMCBasedFit.UseDatabase(True)
#gausFit = ROOT.EXOEnergyCalib.GetInstanceForFlavor("2014-v1","2013-0nu-denoised","2014-v1")
#mcbasedFit = ROOT.EXOEnergyCalib.GetInstanceForFlavor("2014-v1-weekly","2014-v1-average")

#print gausFit.RawRotatedEnergy(1000,1000,1,startTime.GetSec(),0), gausFit.RawRotatedEnergy(1000,1000,2,startTime.GetSec(),0)
#print mcbasedFit.RawRotatedEnergy(1000,1000,1,startTime.GetSec(),0), mcbasedFit.RawRotatedEnergy(1000,1000,2,startTime.GetSec(),0)

#print gausFit.CalibratedRotatedEnergy(1000,1000,1,startTime.GetSec(),0), gausFit.CalibratedRotatedEnergy(1000,1000,2,startTime.GetSec(),0)
#print mcbasedFit.CalibratedRotatedEnergy(1000,1000,1,startTime.GetSec(),0), mcbasedFit.CalibratedRotatedEnergy(1000,1000,2,startTime.GetSec(),0)
