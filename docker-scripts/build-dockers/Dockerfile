###############################################################################
###  NDS Builder                                                            ###
###############################################################################
FROM skylyrac/blocksds:slim-latest as ds-build
RUN mkdir -p /home/builder/slot1launch
RUN mkdir -p /home/builder/building
RUN chown -R ubuntu:ubuntu /home/builder
WORKDIR /home/builder/building
RUN apt update ; apt -y install python3
USER ubuntu
CMD /home/builder/slot1launch/docker-scripts/docker_build_nds
