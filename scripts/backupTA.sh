#!/bin/bash
cd `dirname $0`
ARCH=$(adb shell getprop ro.product.cpu.abi | tr -d '\r')
DEVICE=$(adb shell getprop ro.product.device | tr -d '\r')
SERIAL=$(adb shell getprop ro.serialno | tr -d '\r')
SUFFIX=32
if [ "$ARCH" == "arm64-v8a" ]; then
SUFFIX=64
fi
adb push files/dirtycow${SUFFIX} /data/local/tmp/dirtycow
adb push files/run-as${SUFFIX} /data/local/tmp/run-as
adb push files/exploitta${SUFFIX} /data/local/tmp/exploitta
adb push files/dumpta${SUFFIX} /sdcard/dumpta
adb push files/backupTADevice.sh /data/local/tmp
adb shell "chmod 755 /data/local/tmp/*"
TAIMG="TA_${DEVICE}_${SERIAL}_$(date -u +%Y%m%d-%H%M).img"
adb shell /data/local/tmp/backupTADevice.sh ${TAIMG}
adb pull /data/local/tmp/${TAIMG} ${TAIMG}
adb shell "rm -f /data/local/tmp/dirtycow /data/local/tmp/run-as /data/local/tmp/exploitta /sdcard/dumpta /data/local/tmp/backupTADevice.sh"
echo "TA Sucessfully pulled to ${TAIMG}"
