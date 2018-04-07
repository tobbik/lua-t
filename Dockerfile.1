FROM       alpine:latest
MAINTAINER Tobias Kieslich <tobias.kieslich@gmail.com>

ENV LVER=5.3 LREL=4 LURL=http://www.lua.org/ftp/

# Build dependencies.
RUN apk update && \
    apk --no-cache add \
    build-base linux-headers git bash unzip curl readline readline-dev

# Build a reasonably vanilla lua version. This models the archlinux package
COPY build/${LVER}/liblua.so.patch /tmp/
COPY build/${LVER}/lua.pc /tmp/
RUN cd /tmp && \
    curl -o lua.tgz ${LURL}/lua-${LVER}.${LREL}.tar.gz && \
    tar xzvf lua.tgz && mv lua-${LVER}.${LREL} lua && cd lua && \
    patch -p1 -i ../liblua.so.patch && \
    sed "s/%VER%/${LVER}/g;s/%REL%/${LVER}.${LREL}/g" ../lua.pc > lua.pc && \
    make MYCFLAGS="-fPIC -fbuiltin -g -O0" linux && \
    make \
      TO_LIB="liblua.a liblua.so liblua.so.${LVER} liblua.so.${LVER}.${LREL}" \
      INSTALL_DATA='cp -d' \
      INSTALL_TOP=/usr \
      INSTALL_MAN=/usr/share/man/man1 \
      install && \
    ln -sf /usr/bin/lua   /usr/bin/lua${LVER} && \
    ln -sf /usr/bin/luac  /usr/bin/luac${LVER} && \
    ln -sf /usr/lib/liblua.so.${LVER}.${LREL} /usr/lib/liblua${LVER}.so && \
    install -Dm644 lua.pc /usr/lib/pkgconfig/lua53.pc && \
    ln -sf /usr/lib/pkgconfig/lua53.pc "$pkgdir"/usr/lib/pkgconfig/lua.pc

COPY src      /lua-t/src
COPY lua      /lua-t/lua
COPY test     /lua-t/test
COPY example  /lua-t/example
COPY Makefile /lua-t/Makefile
RUN cd /lua-t && \
    make BUILD_EXAMPLE=1 pristine && \
    make \
      LVER=${LVER} \
      MYCFLAGS="-fbuiltin -g -O0" \
      INCDIR=/usr/include \
      BUILD_EXAMPLE=1 \
      DEBUG=1 \
      PREFIX=/usr all && \
    make \
      LVER=${LVER} \
      MYCFLAGS="-fbuiltin -g -O0" \
      INCDIR=/usr/include \
      BUILD_EXAMPLE=1 \
      DEBUG=1 \
      PREFIX=/usr install

