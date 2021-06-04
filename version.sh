#!/usr/bin/env sh
# -*- coding: utf-8 -*-

cd "$(dirname "$0")"
git rev-list --count cube-and-conquer | tr -d '\n'