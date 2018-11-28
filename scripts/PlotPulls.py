
import ROOT

ROOT.gStyle.SetOptFit()
rin = ROOT.TFile('pulls_thcora_true3pars_calib.root','read')
nPar = 3#2

canvas = ROOT.TCanvas()
canvas.Divide(2,nPar)
for m,mult in enumerate(['ss','ms']):
    for p in range(nPar):
        h = rin.Get('%s_p%i'%(mult,p))
        h.Rebin(40)
        c = (2*(p+1),2*(p+1)-1)[mult == 'ss']
        canvas.cd(c)
        h.Fit('gaus','LQ')
        h.Draw('')
raw_input('pulls')

canvas2 = ROOT.TCanvas()
canvas2.Divide(2,nPar)
for m,mult in enumerate(['ss','ms']):
    for p in range(nPar):
        h = rin.Get('%s_dp%i'%(mult,p))
        h.Rebin(2)
        h.GetXaxis().SetRangeUser(-1,1)
        c = (2*(p+1),2*(p+1)-1)[mult == 'ss']
        canvas2.cd(c)
        #h.Fit('gaus','LQ')
        h.Draw('')
raw_input('rel diffs')

        
rin.Close()
