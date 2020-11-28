FROM zmkfirmware/zephyr-west-action-arm

ENV LC_ALL=C

RUN apt-get -y update && \
    apt-get -y upgrade && \
    apt-get install --no-install-recommends -y \
    ssh \
    nano \
    locales \
    gpg && \
    rm -rf /var/lib/apt/lists/*

COPY .bashrc tmp
RUN mv /tmp/.bashrc ~/.bashrc
