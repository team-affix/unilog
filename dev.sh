# Build the docker image using the dockerfile
docker build -t unilog-dev .
# Rebuild:
# docker build --no-cache -t unilog-dev .

# Create and run a docker container, such that it is removed after exiting.
#     Also, mount local directory into workdir of container
docker run -v .:/unilog --rm -it unilog-dev /bin/bash
