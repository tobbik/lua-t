FROM       alpine:latest AS luabuilder
MAINTAINER Tobias Kieslich <tobias.kieslich@gmail.com>

ENV LVER=5.4 LREL=1 LURL=http://www.lua.org/ftp/

# Build dependencies.
RUN apk update && \
    apk --no-cache add \
    build-base linux-headers wget readline readline-dev

# Build a reasonably vanilla lua version. This models the archlinux package
COPY build/${LVER}/liblua.so.patch /tmp/
COPY build/${LVER}/lua.pc /tmp/
RUN mkdir -p /tmp/lua && cd /tmp/lua && \
    wget ${LURL}/lua-${LVER}.${LREL}.tar.gz  -O - | tar xz --strip-components 1 && \
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
    ln -sf /usr/lib/pkgconfig/lua53.pc /usr/lib/pkgconfig/lua.pc

COPY src      /lua-t/src
COPY lua      /lua-t/lua
COPY test     /lua-t/test
COPY example  /lua-t/example
COPY Makefile /lua-t/Makefile
COPY dev.mk   /lua-t/dev.mk
RUN cd /lua-t && \
    make BUILD_EXAMPLE=1 pristine && \
    make \
      LVER=${LVER} \
      CC=gcc LD=gcc \
      MYCFLAGS="-fbuiltin -g -O0" \
      INCDIR=/usr/include \
      BUILD_EXAMPLE=1 \
      DEBUG=1 \
      PREFIX=/usr install



# second stage: install the build products only and clean out the trash
FROM       alpine:latest
ENV LVER=5.4 LREL=1 LURL=http://www.lua.org/ftp/
RUN apk update && \
    apk --no-cache add bash readline
COPY --from=luabuilder  /usr/bin/lua         /usr/bin/lua
COPY --from=luabuilder  /usr/bin/luac        /usr/bin/luac
COPY --from=luabuilder  /usr/lib/lua         /usr/lib/lua
COPY --from=luabuilder  /usr/lib/liblua.so   /usr/lib/liblua.so
COPY --from=luabuilder  /usr/share/lua       /usr/share/lua
COPY --from=luabuilder  /lua-t/test          /lua-t/test
COPY --from=luabuilder  /lua-t/example       /lua-t/example
RUN mv     /usr/lib/liblua.so                 /usr/lib/liblua.so.${LVER}.${LREL} && \
    ln -sf /usr/lib/liblua.so.${LVER}.${LREL} /usr/lib/liblua${LVER}.so && \
    ln -sf /usr/lib/liblua.so.${LVER}.${LREL} /usr/lib/liblua.so.${LVER} && \
    ln -sf /usr/lib/liblua.so.${LVER}.${LREL} /usr/lib/liblua.so
