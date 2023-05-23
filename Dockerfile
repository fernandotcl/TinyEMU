FROM ubuntu:22.04 AS build
ARG USER_ID
ARG GROUP_ID
ARG USER_NAME
ARG GROUP_NAME

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
&&  apt-get install -y \
        build-essential \
        git \
        libssl-dev \
        libcurl4-openssl-dev \
        libsdl2-dev

RUN groupadd -g ${GROUP_ID} ${GROUP_NAME} || true \
;   useradd -u ${USER_ID} -g ${GROUP_ID} -d ${HOME} -m -s /bin/bash ${USER_NAME} \
;   echo "${USER_NAME} ALL = (ALL) NOPASSWD: ALL" > /etc/sudoers.d/${USER_NAME} \
;   install -m 700 -o ${USER_NAME} -g ${GROUP_NAME} -d /run/user/${USER_NAME}
