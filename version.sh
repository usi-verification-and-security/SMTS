#!/usr/bin/env sh
# -*- coding: utf-8 -*-

cd "$(dirname "$0")"
if [ $# -lt 1 ]
then
  git config --global --add safe.directory /SMTS
fi
git rev-list --count origin/cube-and-conquer | tr -d '\n'