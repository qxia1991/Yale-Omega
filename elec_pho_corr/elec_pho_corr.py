import ROOT, os, sys, glob,pdb
ROOT.gSystem.Load("libEXOCalibUtilities")
import numpy as np
import pdb
import matplotlib
#matplotlib.use("pdf") 
import matplotlib.pyplot as plt
from matplotlib import image
import scipy.ndimage
from scipy.special import erf
from scipy.optimize import curve_fit
from scipy.interpolate import griddata
import cPickle as pickle
from numpy.fft import rfft
import glob
badChannelList = [163,178,191,205]
#dic=open("charge_inject_6449.pickle")
#e_per_bit=pickle.load(dic)
run_num = 6453
flist=glob.glob('run0000%d*.root'%(run_num))
fproclist=glob.glob('proc0000%d*.root'%(run_num))
u_gain = [0]*114
infile = open("uwire_gain.txt").readlines()
infile2 = open("APDGainCalibration_6473_TablesForDB.txt").readlines()[1:]
badChannelList = [163,178,191,200,205]
apd_gain = [0]*226
for line in infile2:
   line = line.split()
   ch=int(line[0])
   apd_gain[ch]=float(line[1])
for line in infile:
   line = line.split()
   ch=int(line[0])
   u_gain[ch]=float(line[1])
flist = [""]
fproclist = []
tc = ROOT.TChain("tree")
tcproc=ROOT.TChain("tree")
for f in flist:
    tc.Add(f)
for f in fproclist:
    tcproc.Add(f)
#print flist,fproclist

def data_from_hist( h ):
    nbins = h.GetNbinsX()+1

    out_arr = np.zeros( (nbins, 4) )

    for i in range(nbins):
        bcent = h.GetBinCenter(i)
        bval = h.GetBinContent(i)
        berrl = h.GetBinErrorLow(i)
        berrh = h.GetBinErrorUp(i)
        out_arr[i,:] = [bcent, bval, berrl, berrh]
    return out_arr

fUWireScalingFactor=5.61
eperbit=350

def electron_counts(edproc):
    tot=0
    ncc=edproc.GetNumChargeClusters()
    for i in range(ncc):
    ## fid vol cut
       meangain_numerator = 0
       meangain_denominator = 0
       ccl = edproc.GetChargeCluster(i)
       nUch = ccl.GetNumUWireSignals()
#        if nUch !=1: return 0
       for j in range(nUch):
          Uch = ccl.GetUWireSignalChannelAt(j)
          Usig = ccl.GetUWireSignalAt(j)
          meangain_numerator += u_gain[Uch]*Usig.fRawEnergy
          meangain_denominator += Usig.fRawEnergy
          print Usig.fRawEnergy
          x,y,z = ccl.fX, ccl.fY, ccl.fZ
          u,v = ccl.fU, ccl.fV
        #print x,u,v,z
          if( np.abs(x)>162. or np.abs(u)> 162. or np.abs(v)>162 or np.abs(z)<10. or np.abs(z)>182):
             return 0
        #tot+=ccl.fCorrectedEnergy/fUWireScalingFactor*eperbit
       tot+=ccl.fRawEnergy/fUWireScalingFactor*meangain_numerator/meangain_denominator#*int_u_gain2["Channel_%d_Gain"%(Uch)][0]/int_u_gain2["Channel_%d_Gain"%(Uch)][0]
        #tot+=ccl.fRawEnergy
    return tot

def photon_counts(edproc):
    tot=0
    energy=0
    nsc=edproc.GetNumScintillationClusters()
    for i in range(nsc):
        sc=edproc.GetScintillationCluster(i)
        ncc=sc.GetNumChargeClusters()
    ## fid vol cut
        for j in range(ncc):
            ccl = sc.GetChargeClusterAt(j)
            x,y,z = ccl.fX, ccl.fY, ccl.fZ
            u,v = ccl.fU, ccl.fV
            if( np.abs(x)>162. or np.abs(u)> 162. or np.abs(v)>162 or np.abs(z)<10. or np.abs(z)>182): return 0
        for ch in range(152, 226):
            if( ch in badChannelList ): continue
            ects_n = sc.GetAPDSignal(ROOT.EXOAPDSignal.kGangFit,ch).fRawCounts/apd_gain[ch]*ROOT.APD_GAIN
            if ects_n<0: continue
            tot+=ects_n
        energy+=sc.fRawEnergy/4.25*4.5
    return energy

def twoD_Gaussian((x, y), amplitude, xo, yo, sigma_x, sigma_y, theta, offset):
    xo = float(xo)
    yo = float(yo)    
    a = (np.cos(theta)**2)/(2*sigma_x**2) + (np.sin(theta)**2)/(2*sigma_y**2)
    b = -(np.sin(2*theta))/(4*sigma_x**2) + (np.sin(2*theta))/(4*sigma_y**2)
    c = (np.sin(theta)**2)/(2*sigma_x**2) + (np.cos(theta)**2)/(2*sigma_y**2)
    g = offset + amplitude*np.exp( - (a*((x-xo)**2) + 2*b*(x-xo)*(y-yo) 
                            + c*((y-yo)**2)))
    return g.ravel()
def log_twoD_Gaussian(x,y):
    def log_sum_Gaussian(p):
        amplitude=p[0]
        xo = p[1]
        yo = p[2]
        sigma_x = p[3]
        sigma_y = p[4]
        theta = p[5]
        offset = p[6]
        xo = float(xo)
        yo = float(yo)
        a = (np.cos(theta)**2)/(2*sigma_x**2) + (np.sin(theta)**2)/(2*sigma_y**2)
        b = -(np.sin(2*theta))/(4*sigma_x**2) + (np.sin(2*theta))/(4*sigma_y**2)
        c = (np.sin(theta)**2)/(2*sigma_x**2) + (np.cos(theta)**2)/(2*sigma_y**2)
        g = offset + amplitude*np.exp( - (a*((x-xo)**2) + 2*b*(x-xo)*(y-yo)+ c*((y-yo)**2)))
        log_sum = np.sum(g)
        return log_sum
    return log_sum_Gaussian


def flinear(x,A,B):
   return A*x+B

def ffn(x, A1, A2, mu, sig):                                                                                                       
    return A1*(1-erf((x-mu)/(np.sqrt(2*sig**2)))) + A2*np.exp(-(x-mu)**2/(2*sig**2))                                             
                                                                                                                                   
def step_plot(be,h,p0,lab,fit,col='k'):                                                                                                
    x = be[:-1] + np.diff(be)/2.0                                            
    fmin,fmax=x[np.argmax(h)]-1000,x[np.argmax(h)]+1000 
    s = np.sqrt(h) 
    s[s==0] = 1.0     
    gpts = np.logical_and(fmin<x, fmax>x)
#    plt.errorbar(x, h, yerr=np.sqrt(h), fmt='.', color=col, markeredgecolor=col)                                                   
    if fit==True:
        p0, pcov = curve_fit(ffn, x[gpts], h[gpts], p0=p0, sigma=s[gpts])   
        xx = np.linspace( x[gpts][0], x[gpts][-1], 1e3)                  
 #       plt.plot(xx,ffn(xx,*p0), label="%.3f"%(p0[2]) +lab+"\n error=%.3f"%(np.sqrt(np.diag(pcov))[2]), color=col, lw=1.5)
    else:
        p0[2]=p0[3]=0
    return p0[2],p0[3]

nt = tc.GetEntries()
cele = []
spec=[]
cpho=[]
num=0
tot_num=0
angle=0.23
flag = 0
if flag == 0: 
    for n in range(nt):
       e_tot=0
       s_tot=0
       tot=0
       if(n % 10000==0): print n
       tc.GetEntry(n)
       tcproc.GetEntry(n)
       ed = tc.EventBranch
       edproc=tcproc.EventBranch
       ncc=edproc.GetNumChargeClusters()
       nsc=edproc.GetNumScintillationClusters()
       for i in range(ncc):
         ecc=edproc.GetChargeCluster(i).fPurityCorrectedEnergy
         e_tot+=ecc
       for i in range(nsc):
          esc=edproc.GetScintillationCluster(i).fRawEnergy
          s_tot+=esc
       enum=electron_counts(edproc)
       snum = photon_counts(edproc)
       spec.append(np.cos(angle)*e_tot+np.sin(angle)*s_tot)
       cele.append(enum)
       cpho.append(snum)
    cele = np.array(cele)
    cpho = np.array(cpho)
    spec = np.array(spec)
    np.savez(corr_file,cele=cele,cpho=cpho,spec=spec)
else:
   popt = [[]for i in range(5)] 
   pcov = [[]for i in range (5)]
   xerr = []
   yerr = []
   for i,run_num in enumerate([6460,6453,6454,6456,6481]): 
#   for i,run_num in enumerate([6456]):
      corr_file = "elec_pho_corr_%d.npz"%(run_num)
      corr_file = np.load(corr_file)
      plt.figure()
      spec = corr_file['spec']
      cele = corr_file['cele']
      cpho = corr_file['cpho']
      h2,b2=np.histogram(spec,bins=20,range=(2000,6000))
      mean,sigma = step_plot(b2, h2,[0.1,100,4000,500], fit=True,col='r',lab="run %d rotated energy spectrum"%(run_num))          
      gpts = np.logical_and(mean-3*sigma<spec, mean+3*sigma>spec)
      plt.plot(cpho[gpts],cele[gpts],'go',label="electron vs photon number")
      plt.ion()
      plt.show()
      raw_input()
      plt.close()
      xbin = x[1]-x[0]
      ybin = y[1]-y[0]
      x = cpho[gpts]
      y = celec[gpts]
      a0 = 10
      x0 = 5000
      y0 = 120000
      sigmax0 = 100
      sigmay0 = 8000
      theta0 = np.deg2rad(84)
      offset0 = 10
      initial_guess = (a0,x0,y0,sigmax0,sigmay0,theta0,offset0)
      log_sum = log_twoD_Gaussian(x,y)
      res = minimize(log_sum,initial_guess,method="CG")
      print (res.x)

      if run_num == 6460:
         field = "1kV"
         x = np.linspace(int(plt.xlim()[0]), int(plt.xlim()[-1]), plt.xlim()[-1]/1000)
         y = np.linspace(int(plt.ylim()[0]), int(plt.ylim()[-1]), plt.ylim()[-1]/5500)
      elif run_num == 6453:
         field = "1746V"
         x = np.linspace(int(plt.xlim()[0]), int(plt.xlim()[-1]), plt.xlim()[-1]/900)
         y = np.linspace(int(plt.ylim()[0]), int(plt.ylim()[-1]), plt.ylim()[-1]/5500)
      elif run_num ==6454:
         field = "4070V"
         x = np.linspace(int(plt.xlim()[0]), int(plt.xlim()[-1]), plt.xlim()[-1]/700)
         y = np.linspace(int(plt.ylim()[0]), int(plt.ylim()[-1]), plt.ylim()[-1]/4500)
      elif run_num == 6456:
         field = "8kV"
         x = np.linspace(int(plt.xlim()[0]), int(plt.xlim()[-1]), plt.xlim()[-1]/300)
         y = np.linspace(int(plt.ylim()[0]), int(plt.ylim()[-1]), plt.ylim()[-1]/4500)
      elif run_num == 6481:
         field = "13kV"
         x = np.linspace(int(plt.xlim()[0]), int(plt.xlim()[-1]), plt.xlim()[-1]/300)
         y = np.linspace(int(plt.ylim()[0]), int(plt.ylim()[-1]), plt.ylim()[-1]/6500)

      H,xedges,yedges = np.histogram2d(cpho[gpts],cele[gpts],bins=(x,y))
      H = H.T
      x, y = np.meshgrid(xedges[:-1], yedges[:-1])
      if run_num == 6460:
         initial_guess = (10,13000,90000,2000,9000,1.5,0)
      elif run_num == 6453:
         initial_guess = (10,13000,90000,2000,9000,1.5,0)
      elif run_num == 6454:
         initial_guess = (10,9000,120000,900,10000,1.5,0)
      elif run_num == 6456: 
         initial_guess = (10,8500,130000,800,10000,1.5,0)
      elif run_num == 6481:
         initial_guess = (10,7500,140000,800,12000,1.5,0)

#      popt[i],pcov[i] = curve_fit(twoD_Gaussian,(x,y),np.ravel(H),p0=initial_guess)
 #     xerr.append(xbin/2)
  #    yerr.append(ybin/2)
   #   print popt[i],pcov[i]
      data_fitted = twoD_Gaussian((x,y),res.x)
      rotation = res.x[-2]
      if run_num in [6460,6453,6454]:
         slope = -abs(1/np.tan(rotation))*2/3
      elif run_num == 6456:
         slope = -abs(1/np.tan(rotation))*1.05/2
      else:
         slope = -abs(1/np.tan(rotation))*0.95/2
      np.put(popt[i],-2,abs(np.arctan(slope)))
      fig = plt.figure()
#      ax = fig.add_subplot()
      #plt.setp(ax, xticks=[65000,75000,85000,95000,105000,115000], xticklabels=['65000','75000','85000','95000','105000','115000'])
      im = plt.imshow(H, cmap=plt.cm.jet,aspect=(x.max()-x.min())/(y.max()-y.min()), origin='bottom',extent=(x.min(), x.max(), y.min(), y.max()),vmin=H.min(),vmax=H.max())
      max_pho = popt[i][1]
      max_ele = popt[i][2]
      yintercept = max_ele-slope*max_pho
      xintercept = -yintercept/slope
      x_vals = np.array(plt.xlim())
      y_vals = slope*x_vals+yintercept
#      centerx_err = np.sqrt(np.abs(np.diag(pcov[i])))[1]/popt[i][2] #relative err
#      centery_err = np.sqrt(np.abs(np.diag(pcov[i])))[2]/popt[i][2] #relative err
      centerx_err = xbin/2/max_pho
      centery_err = ybin/2/max_ele
      rotation_err = abs((np.arctan(slope)-np.arctan(1/np.tan(rotation)))/np.arctan(slope))
      yintercept_err = np.sqrt((max_ele*centery_err)**2+(slope*max_pho*np.sqrt(centerx_err**2+(1/np.cos(np.arctan(slope))**2*np.arctan(slope)*rotation_err/slope)**2))**2) # abs err
      xintercept_err = np.sqrt((max_pho*centerx_err)**2+(max_ele/slope*np.sqrt(centery_err**2+(1/np.cos(np.arctan(slope))**2*np.arctan(slope)*rotation_err/slope)**2))**2) # abs err
#      xintercept_err = xintercept*np.sqrt((yintercept_err/yintercept)**2+(1/np.cos(np.arctan(slope))**2*np.arctan(slope)*rotation_err/slope)**2) # abs err
      plt.title("Electron photon anticorrelation run %d %s"%(run_num,field))
      plt.plot(x_vals,y_vals,'w-',label="rotation angle:%.1f$^{\circ}\pm$%.1f$^{\circ}$\npeak photon number:%d$\pm$%d\n peak electron number:%d$\pm$%d\n photon intercept:%d$\pm$%d\n electron intercept:%d$\pm$%d"%(-np.rad2deg(np.arctan(slope)),-np.rad2deg(np.arctan(slope)*rotation_err),max_pho,max_pho*centerx_err,max_ele,max_ele*centery_err,xintercept,xintercept_err,yintercept,yintercept_err))
      plt.legend(loc="upper left",prop={"size":10})
      plt.colorbar()
      x = scipy.ndimage.zoom(x, 3)
      y = scipy.ndimage.zoom(y, 3)
      data_fitted = scipy.ndimage.zoom(data_fitted.reshape(H.shape), 3)
      plt.contour(x,y,data_fitted,8,colors='w')
      plt.ylim(yedges[0],yedges[-2])
      plt.xlim(xedges[0],xedges[-2])
      plt.savefig("measured_elec_pho_corr_%d.png"%(run_num))
      plt.show()
      raw_input()
      plt.close()
      fig2 = plt.figure()
      ax2 = fig2.add_subplot(121,title='')
      X, Y = np.meshgrid(xedges, yedges)
      ax2.pcolormesh(X, Y, H)
      ax2 = fig2.add_subplot(122,title='')
      im2 = image.NonUniformImage(ax2, interpolation='bilinear')
      xcenters = xedges[:-1]# + xedges[1:]) / 2
      ycenters = yedges[:-1]# + yedges[1:]) / 2
      im2.set_data(xcenters, ycenters, H)
      ax2.images.append(im2)
#      im2 = ax2.imshow(data_fitted, cmap=plt.cm.jet, origin='bottom',extent=(x.min(), x.max(),y.min(), y.max()))
      ax2.contour(x,y,data_fitted, 8, colors='w')
      plt.plot(x_vals,y_vals,'w-',label="rotation angle:%.1f$^{\circ}\pm$%.1f$^{\circ}$\npeak photon number:%d$\pm$%d\n peak electron number:%d$\pm$%d\n photon intercept:%d$\pm$%d\n electron intercept:%d$\pm$%d"%(-np.rad2deg(np.arctan(slope)),-np.rad2deg(np.arctan(slope)*rotation_err),max_pho,max_pho*centerx_err,max_ele,max_ele*centery_err,xintercept,xintercept_err,yintercept,yintercept_err))
      plt.show()
      raw_input()
      plt.close()
#   elec_tune =2615000/13.6/220000 
   elec_tune = 1
   def errorfunction(eff,theta,B):
      global popt
      popt = np.array(popt)
      return (np.tan(popt[:,-2])*abs(eff)-abs(np.tan(theta)))**2/abs(np.tan(theta))+(-abs(np.tan(theta))*popt[:,1]/abs(eff)+B-popt[:,2]*elec_tune)**2/(popt[:,2]*elec_tune)
   errfunction = lambda p: errorfunction(*p) 
   params = (0.1,0.755,2615000/13.6)
   p,cov_x,infodict,errmsg,success = scipy.optimize.leastsq(errfunction, params,full_output=1,epsfcn=0.0001)
   print p,np.sqrt(np.diag(cov_x))
   print errorfunction(*p)
   plt.figure()
   plt.errorbar(popt[:,1]/abs(p[0]),popt[:,2]*elec_tune,xerr=xerr/abs(p[0]),yerr=yerr,fmt='o')
#   plt.plot(popt[:,1]/0.099,popt[:,2],'o')
   p0=(-1,100000)
   p0, pcov = curve_fit(flinear, popt[:,1]/0.099,popt[:,2], p0=p0)   
   print p0,np.sqrt(np.diag(pcov))
   x_vals = np.array(plt.xlim())
   y_vals = -abs(np.tan(p[1]))*x_vals+p[2]
#   y_vals = p0[0]*x_vals+p0[1]
   plt.plot(x_vals,y_vals,'-',label='%f*x+%f'%(-abs(np.tan(p[1])),p[2]))
   plt.title("Electron vs. photon under various E fields(efficiency=%.3f corr_angle=%.1f$^{\circ}$)"%(p[0],abs(np.rad2deg(p[1]))))
   plt.xlabel("photon number")
   plt.ylabel("electron number")
   plt.legend(loc="upper left",prop={"size":9})
   plt.savefig("elec_pho_corr.png")
   plt.show()
   raw_input()

