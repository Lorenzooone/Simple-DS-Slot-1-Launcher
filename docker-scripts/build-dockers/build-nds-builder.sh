#!/bin/bash
docker image rm lorenzooone/slot1launch:nds_builder
docker build --target ds-build . -t lorenzooone/slot1launch:nds_builder
