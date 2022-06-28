#!/usr/bin/env sh
# -*- coding: utf-8 -*-

cd "$(dirname "$0")"
git config --global --add safe.directory /SMTS
git rev-list --count origin/cube-and-conquer | tr -d '\n'