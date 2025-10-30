PLAT=$(uname -s)

if [ "x${PLAT}" = "xLinux" ]; then
  echo $(pkgconf --libs lua)
elif [ "x${PLAT}" = "xFreeBSD" ] || \
     [ "x${PLAT}" = "xOpenBSD" ] || \
     [ "x${PLAT}" = "xNetBSD" ]; then
  echo $(pkgconf --libs lua-5.4)
else
  echo "-llua -lm"
fi

