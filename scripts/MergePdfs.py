
import os, settings

settings.init()

cmd = 'gs -o %s -sDEVICE=pdfwrite -dPDFSETTINGS=/prepress %s'
dir = settings.CalibrationOutput

pdfs = {'merged_fit_weeks_ss_histo.pdf':'fit_week_???_ss_histo.pdf', 
        'merged_fit_weeks_ms_histo.pdf':'fit_week_???_ms_histo.pdf',
        'merged_fit_weeks_ss_data.pdf':'fit_week_???_ss_data.pdf',
        'merged_fit_weeks_ms_data.pdf':'fit_week_???_ms_data.pdf'}

for pdf in pdfs:
    cmdt = cmd % (dir+pdf,dir+pdfs[pdf])
    print cmdt
    os.system(cmdt)

for pdf in pdfs:
    print 'Created', dir+pdf
