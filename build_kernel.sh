# Android kernel 3.4.5 Build Script for TF4 PVT and DVP board 
# You make sure that the PATH of CROSS COMPILER.
# Android JB_MR1 source code package included ARM Cross Complier v4.6 under "{platform}/prebuilts/gcc/linux-x86/arm/" directory.

KERNEL_CROSS_COMPILE_PATH="../prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-"
export CROSS_COMPILE=$KERNEL_CROSS_COMPILE_PATH

# CPU_JOB_NUM=$(grep processor /proc/cpuinfo | awk '{field=$NF};END{print int((field+1)/2)}')
CPU_JOB_NUM="4"
if [ $# -lt 1 ]
then
		echo "========================================"
		echo "Usage: ./build_kernel.sh <PRODUCT_BOARD>"
		echo "ex) ./build_kernel.sh yamo5410 | tf4_dvt"
		echo "========================================"
		echo "Forcely set yamo5410"
		PRODUCT_BOARD="yamo5410"
#		exit 0
else
		PRODUCT_BOARD=$1
fi

echo
echo '[[[[[[[ Build android kernel 3.4.5 for MR1 ]]]]]]]'
echo 
				
START_TIME=`date +%s`

echo "set defconfig for $PRODUCT_BOARD"
echo
export ARCH=arm
make $PRODUCT_BOARD"_android_defconfig"
echo "make"
echo
make -j$CPU_JOB_NUM

END_TIME=`date +%s`

let "ELAPSED_TIME=$END_TIME-$START_TIME"
echo
echo "Total compile time is $ELAPSED_TIME seconds"
