FROM zmkfirmware/zephyr-west-action-arm

ENV LC_ALL=C

RUN apt-get -y update \
    && apt-get -y upgrade \
    && apt-get install --no-install-recommends -y \
    ssh \
    nano \
    locales \
    ssh \
    gpg \
    gpg-agent \
    curl \
    && curl -sL https://deb.nodesource.com/setup_14.x | bash - \
    && apt-get -y update \
    && apt-get install --no-install-recommends -y nodejs \
    && node --version \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY .bashrc tmp
RUN mv /tmp/.bashrc ~/.bashrc
