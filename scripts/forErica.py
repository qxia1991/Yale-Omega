
import ROOT
ROOT.gSystem.Load('libEXOCalibUtilities')

someTime = ROOT.TTimeStamp(2011, 10, 9, 18, 0, 0, 0, True)
ericaTime = ROOT.TTimeStamp(1346773897)
print someTime.GetSec(), ericaTime.GetSec()
ericaTime.Print()
apdGains = ROOT.EXOCalibManager.GetCalibManager().getCalib(ROOT.EXOAPDGainsHandler.GetHandlerName(),"vanilla",someTime.GetSec())

print apdGains.gain(152), apdGains.gain_error(152)
print apdGains.validSince().getClibTime(), apdGains.validTill().getClibTime(), 
#,apdGains.gain(153),apdGains.gain(154),apdGains.gain(155),apdGains.gain(156)


