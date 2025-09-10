FROM debian:trixie-slim

# Environment

ENV TOOLKIT_VER=7.2
ENV PLATFORM=epyc7002

# Set up runtime dependencies

# hadolint ignore=DL3008
RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        cifs-utils \
        libcap2-bin \
        python3 \
        python3-pip \
        xz-utils \
    ; \
    rm -rf /var/lib/apt/lists/*

# hadolint ignore=DL3003,DL3008
RUN set -eux; \
    \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        git \
        wget \
    ; \
    \
    mkdir -p /toolkit && cd /toolkit; \
    git clone https://github.com/SynologyOpenSource/pkgscripts-ng; \
    cd /toolkit/pkgscripts-ng; \
    git checkout "DSM${TOOLKIT_VER}"; \
    mkdir -p /toolkit/source; \
    mkdir -p /toolkit/sources; \
    mkdir -p /toolkit/packages; \
    mkdir -p /toolkit/toolkit_tarballs && cd /toolkit/toolkit_tarballs; \
    wget --progress=dot:giga "https://global.synologydownload.com/download/ToolChain/toolkit/${TOOLKIT_VER}/base/base_env-${TOOLKIT_VER}.txz"; \
    wget --progress=dot:giga "https://global.synologydownload.com/download/ToolChain/toolkit/${TOOLKIT_VER}/${PLATFORM}/ds.${PLATFORM}-${TOOLKIT_VER}.env.txz"; \
    wget --progress=dot:giga "https://global.synologydownload.com/download/ToolChain/toolkit/${TOOLKIT_VER}/${PLATFORM}/ds.${PLATFORM}-${TOOLKIT_VER}.dev.txz"; \
    apt-get purge -y --auto-remove \
        git \
        wget \
    ; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/*;

COPY docker-entrypoint.sh /usr/local/bin/

# Entrypoint config

WORKDIR /toolkit/pkgscripts-ng
ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]

CMD [""]
