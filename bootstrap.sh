#!/bin/bash
mkdir -p VSPClientUI/xcode   && cd VSPClientUI/xcode   && qmake -spec macx-xcode ../VSPClientUI.pro    && cd ../../
mkdir -p VSPController/xcode && cd VSPController/xcode && qmake -spec macx-xcode ../VSPController.pro  && cd ../../
mkdir -p VSPSetup/xcode      && cd VSPSetup/xcode      && qmake -spec macx-xcode ../VSPSetup.pro       && cd ../../

