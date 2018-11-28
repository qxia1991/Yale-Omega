import glob
import os
import re
import sys
import getopt

def normalize_directory(afile, build_base):
    path_name, class_name = os.path.split(afile)
    class_name = os.path.splitext(class_name)[0]
    path_name = path_name.replace(build_base+'/','')
    return path_name, class_name, afile.replace(build_base+'/','')
 
def do_all_files(output_name, build_base, all_files, input_linkdef = ""): 
    
    ignore_list = [ "EXOTestCalib" ]
    
    warning_header = """
// Please do not edit this file, it was auto-generated
// by makeEXOROOT.py and changes will be lost the next
// time the script is run.  -M. Marino
#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs; // from an autogen
    """
    # open output files
    exo_root_linkdef = open(output_name, "w+")
    
    linkdef_string = ""
    
    for afile in all_files:
        _, class_name, path_name = normalize_directory(afile, build_base) 
        if class_name in ignore_list: continue
        linkdef_string += """
#pragma link C++ defined_in "%s";""" % path_name
    if input_linkdef != "":
        linkdef_string += """
%s\n""" % open(input_linkdef).read()
    linkdef_string += """
#endif /* __CINT__ */
"""

    # write everything out
    exo_root_linkdef.write(warning_header + linkdef_string)

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "o:b:i", ["output=", "basebuild=", "input_linkdef="])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        sys.exit(2)
    output_name = ""
    build_base  = ""
    input_linkdef = ""
    for o, a in opts:
        if o in ("-o", "--output"):
            output_name = a
        elif o in ("-b", "--basebuild"):
            build_base = a
        elif o in ("-i", "--input_linkdef"):
            input_linkdef = a
        else:
            assert False, "unhandled option"
    # ...
    do_all_files(output_name, build_base, args, input_linkdef) 

if __name__ == '__main__':
    main()