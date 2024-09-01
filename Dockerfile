# OS
FROM ubuntu:22.04

# Set the working directory
WORKDIR /unilog

# Update OS
RUN apt-get update -y
RUN apt-get upgrade -y

# Basic CLI Reqs
RUN apt-get install -y curl
RUN apt-get install -y build-essential
RUN apt-get install -y cmake
RUN apt-get install -y ninja-build
RUN apt-get install -y zlib1g-dev
RUN apt-get install -y libarchive-dev
RUN apt-get install -y unixodbc-dev
RUN apt-get install -y libdb-dev
RUN apt-get install -y libpcre3-dev
RUN apt-get install -y libyaml-dev
RUN apt-get install -y default-jdk
RUN apt-get install -y python3-dev
RUN apt-get install -y libssl-dev
RUN apt-get install -y qtbase5-dev
RUN apt-get install -y uuid-dev
RUN apt-get install -y patchelf

# SWI-Prolog specific apt repository and packages
#RUN apt-get install -y software-properties-common
#RUN apt-add-repository -y ppa:swi-prolog/stable
#RUN apt-get -y update
#RUN apt-get -y install swi-prolog

CMD ["/bin/bash"]
