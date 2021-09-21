PLAT=$(uname -s)

if [ "x${PLAT}" = "xLinux" ]; then
  echo "-I$(pkgconf --variable=includedir lua)"
elif [ "x${PLAT}" = "xFreeBSD" ] || \
     [ "x${PLAT}" = "xOpenBSD" ] || \
     [ "x${PLAT}" = "xNetBSD" ]; then
  echo $(pkgconf --cflags lua)
else
  echo "-I/usr/local/include"
fi

