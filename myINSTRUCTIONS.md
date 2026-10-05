# Instructions to complie and run the analysis
To run the simulation and the analysis
```bash
cmake -S scintbar -B scintbar-build      # once, so the new macros get copied
cmake --build scintbar-build -j$(sysctl -n hw.ncpu)
cd scintbar-build

./scintbar scan.mac                       # 9 runs -> scintbar_d1cm.root ... scintbar_d9cm.root
hadd -f scan_all.root scintbar_d*cm.root  # merge (hadd comes with ROOT)
root -l 'plotScan.C("scan_all.root", 48.)'
```

For compiling
```bash
cmake -S scintbar_scan -B scintbar_scan_build \          
     -DGeant4_DIR=/Users/solangel/sw/geant/geant4-v11.4.2-install/lib/cmake/Geant4
```