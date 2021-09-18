PLAT=$(uname -m)   # arm, amd64, ...

if [ "Xamd64" = "X${PLAT}" ]; then
   FLAGS="-march=native"
elif [[ ${PLAT} =~ 86 ]]; then
   FLAGS="-march=native"
elif [[ ${PLAT} =~ armv8l ]]; then
   FLAGS="-m32 -mthumb -march=armv8-a -mcpu=cortex-a72 -mtune=cortex-a72.cortex-a53 -mfloat-abi=softfp"
elif [[ ${PLAT} =~ armv ]]; then
   FLAGS="-fbuiltin -march=native -pipe -fstack-protector-strong -fno-plt"
elif [[ ${PLAT} =~ aarch ]]; then
   FLAGS="-fbuiltin -march=armv8-a -pipe -fstack-protector-strong -fno-plt"
else
   FLAGS=""
fi

echo "$FLAGS"
