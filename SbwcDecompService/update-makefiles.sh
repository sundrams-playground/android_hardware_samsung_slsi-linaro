#!/bin/bash

options="-r vendor.samsung_slsi.hardware:vendor/samsung_slsi/hardware \
         -r android.hidl:system/libhidl/transport \
         -r android.hardware:hardware/interfaces"

outputs="vendor/samsung_slsi/hardware/SbwcDecompService/1.0/default"

#./out/host/linux-x86/bin/hidl-gen -L c++-headers -o $outputs $options vendor.samsung_slsi.hardware.SbwcDecompService@1.0;
#./out/host/linux-x86/bin/hidl-gen -Lmakefile $options vendor.samsung_slsi.hardware.SbwcDecompService@1.0;
./out/host/linux-x86/bin/hidl-gen -Landroidbp $options -o . vendor.samsung_slsi.hardware.SbwcDecompService@1.0;

#./out/host/linux-x86/bin/hidl-gen -L androidbp-impl -o $outputs $options vendor.samsung_slsi.hardware.SbwcDecompService@1.0;

#./out/host/linux-x86/bin/hidl-gen -L hash $options vendor.samsung_slsi.hardware.SbwcDecompService@1.0
#./out/host/linux-x86/bin/hidl-gen -L c++-impl -o $outputs $options vendor.samsung_slsi.hardware.SbwcDecompService@1.0;
