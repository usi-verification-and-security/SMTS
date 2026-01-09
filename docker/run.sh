#!/bin/bash

source $(dirname "$0")/lib

exec docker run -it $IMAGE
