PLAT=$(uname -s)

if [ "x${PLAT}" = "xLinux" ]; then
  echo $(pkgconf --variable=prefix lua)
elif [ "x${PLAT}" = "xFreeBSD" ] || \
     [ "x${PLAT}" = "xOpenBSD" ] || \
     [ "x${PLAT}" = "xNetBSD" ]; then
  echo "/usr/local"
else
  echo "/usr/local"
fi

