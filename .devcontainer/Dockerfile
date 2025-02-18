FROM docker.io/zmkfirmware/zmk-dev-arm:3.5

COPY .bashrc tmp
RUN mv /tmp/.bashrc ~/.bashrc
RUN apt-get update && apt-get install -y build-essential
