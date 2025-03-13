#!/bin/bash
cd VSPClientUI/xcode   && qmake -spec macx-xcode ../VSPClientUI.pro    && cd ../../
cd VSPController/xcode && qmake -spec macx-xcode ../VSPController.pro  && cd ../../
cd VSPSetup/xcode      && qmake -spec macx-xcode ../VSPSetup.pro       && cd ../../

