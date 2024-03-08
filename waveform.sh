#!/bin/sh

IN=$1
OUT=$2

[ $# -lt 2 ] && exit 1

set -xe

ffmpeg -y -i ${IN} -f lavfi -i color=c=black:s=1440x720 -filter_complex "[0:a]showwavespic=s=1440x720:colors=white[fg];[1:v][fg]overlay=format=auto" -frames:v 1 ${OUT}
