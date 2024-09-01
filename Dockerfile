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

# SWI-Prolog specific apt repository and packages
RUN apt-get install -y software-properties-common
RUN apt-add-repository -y ppa:swi-prolog/stable
RUN apt-get -y update
RUN apt-get -y install swi-prolog

CMD ["/bin/bash"]
