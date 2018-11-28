
import ROOT

filename = 'srcAgreePlots_calibMC_mcReal_Weekly_allEne.root'

rin = ROOT.TFile(filename,'read')
rout = ROOT.TFile('addedPos_'+filename,'recreate')
for sourceName in ['Th-228','Co-60','Ra-226']:
    for m, mult in enumerate(['ss','ms']):
        for sourcePosS in ['5','11','2','8']:
            if sourcePosS == '5':
                hData = rin.Get('%s_%s_%s_data' % (sourceName,sourcePosS,mult))
                hMC = rin.Get('%s_%s_%s_mc' % (sourceName,sourcePosS,mult))
            else:
                hData.Add(rin.Get('%s_%s_%s_data' % (sourceName,sourcePosS,mult)))
                hMC.Add(rin.Get('%s_%s_%s_mc' % (sourceName,sourcePosS,mult)))
        rout.cd()
        hData.Write('%s_%s_data' % (sourceName,mult))
        hMC.Write('%s_%s_mc' % (sourceName,mult))
        hRes = hData.Clone(hData.GetName()+'res')
        hRes.Add(hData,hMC,1,-1)
        hRes.Divide(hMC)
        hRes.Write('%s_%s_res' % (sourceName,mult))
        hData.Divide(hMC)
        hData.Write('%s_%s_ratio' % (sourceName,mult))
rout.Close()
rin.Close()

