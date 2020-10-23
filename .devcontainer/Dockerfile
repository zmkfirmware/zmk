FROM zmkfirmware/zephyr-west-action-arm

RUN apt-get -y update && \
    apt-get -y upgrade && \
    apt-get install --no-install-recommends -y \
    ssh \
    gpg && \
    rm -rf /var/lib/apt/lists/*

COPY .bashrc tmp
RUN mv /tmp/.bashrc ~/.bashrc
