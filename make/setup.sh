
echo "Enter location of EXO_Fitting headers: (e.g. /nfs/slac/g/exo/software/hudson/exo-build-noric32/workspace/EXOFitting-rhel5/EXO_Fitting/EXOFitting/)" 
read EXOFITINC
export EXOFITINC=${EXOFITINC}
echo "Using EXOFITINC=${EXOFITINC}"

echo "Enter location of EXO_Fitting library: (e.g. $EXOLIB/fitting/lib)" 
read EXOFITLIB
export EXOFITLIB=${EXOFITLIB}
export LD_LIBRARY_PATH=${EXOFITLIB}:${LD_LIBRARY_PATH}
echo "Using EXOFITLIB=${EXOFITLIB}"

