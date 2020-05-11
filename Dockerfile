FROM debian:stable-slim

RUN apt-get -y update && \
	apt-get -y upgrade && \
	apt-get install --no-install-recommends -y \
	wget \
	gnupg \
	ca-certificates \
	autoconf \
	automake \
	build-essential \
	ccache \
	device-tree-compiler \
	dfu-util \
	file \
	g++ \
	gcc \
	gcc-multilib \
	gcovr \
	git \
	git-core \
	iproute2 \
	libpcap-dev \
	libtool \
	locales \
	make \
	net-tools \
	ninja-build \
	python3-dev \
	python3-pip \
	python3-ply \
	python3-setuptools \
	python-xdg \
	xz-utils && \
	rm -rf /var/lib/apt/lists/*

ARG ZSDK_VERSION=0.11.2
RUN wget -q "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZSDK_VERSION}/zephyr-sdk-${ZSDK_VERSION}-setup.run" && \
	sh "zephyr-sdk-${ZSDK_VERSION}-setup.run" --quiet -- -d /opt/toolchains/zephyr-sdk-${ZSDK_VERSION} && \
	rm "zephyr-sdk-${ZSDK_VERSION}-setup.run"

ARG CMAKE_VERSION=3.16.2
RUN wget -q https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh && \
	chmod +x cmake-${CMAKE_VERSION}-Linux-x86_64.sh && \
	./cmake-${CMAKE_VERSION}-Linux-x86_64.sh --skip-license --prefix=/usr/local && \
	rm -f ./cmake-${CMAKE_VERSION}-Linux-x86_64.sh

ARG UID=1000
ARG GID=1000

ENV DEBIAN_FRONTEND noninteractive

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN wget -q https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/master/scripts/requirements.txt && \
	wget -q https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/master/scripts/requirements-base.txt && \
	wget -q https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/master/scripts/requirements-build-test.txt && \
	wget -q https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/master/scripts/requirements-doc.txt && \
	wget -q https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/master/scripts/requirements-run-test.txt && \
	wget -q https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/master/scripts/requirements-extras.txt && \
	pip3 install wheel &&\
	pip3 install -r requirements.txt && \
	pip3 install west &&\
	pip3 install sh

RUN groupadd -g $GID -o user

# Set the locale
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/toolchains/zephyr-sdk-${ZSDK_VERSION}
ENV ZEPHYR_BASE=/workdir

RUN chown -R user:user /home/user

CMD ["/bin/bash"]
USER user
WORKDIR /workdir
