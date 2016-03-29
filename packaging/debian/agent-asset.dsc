Format:         1.0
Source:         agent-asset
Version:        0.1.0-1
Binary:         libagent-asset0, agent-asset-dev
Architecture:   any all
Maintainer:     John Doe <John.Doe@example.com>
Standards-Version: 3.9.5
Build-Depends: bison, debhelper (>= 8),
    pkg-config,
    automake,
    autoconf,
    libtool,
    libzmq4-dev,
    uuid-dev,
    libczmq-dev,
    libmlm-dev,
    core-dev,
    libbiosproto-dev,
    libcxxtools-dev,
    libtntdb-dev,
    dh-autoreconf,
    systemd,
    dh-systemd

Package-List:
 libagent-asset0 deb net optional arch=any
 agent-asset-dev deb libdevel optional arch=any

