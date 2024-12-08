# Clone OBS if needed
if [ ! -d obs-studio ]; then
    git clone --recursive https://github.com/obsproject/obs-studio
fi

# Build OBS if needed, and deploy the framework into the Electron app
BUILD_TYPE=Debug
if [ ! -d obs-studio/.build/libobs/$BUILD_TYPE/libobs.framework ]; then
    pushd obs-studio
    export CMAKE_C_COMPILER=clang
    export CMAKE_CXX_COMPILER=clang
    rm -rf .build || true
    mkdir -p .build
    pushd .build
    cmake --preset macos \
        -B `pwd` \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DENABLE_RTMPS=ON \
        -DENABLE_PLUGINS=ON \
        ..
    R=$?
    if [ $R -eq 0 ]; then
        cmake --build `pwd`
        R=$?
    fi

    popd
    popd

    if [ $R != 0 ]; then
        exit $R
    fi

fi

# Install node-gyp if needed
if [ -z `which node-gyp` ]; then
    npm install node-gyp
fi

# pushd obs-plugin
# node-gyp configure build
# if [ $? != 0 ]; then
#     echo "Error building node plugin"
#     exit -1
# fi
# popd

#  mkdir -p electron-quick-start/

APP=app
rm -rf $APP/node_modules
rm -rf $APP/obs-plugin/build
rm -rf $APP/obs-plugin/node_modules
rm -rf $APP/Frameworks

# Copy dependencies
mkdir -p $APP/Frameworks/obs-plugins
cp -r ./obs-studio/.build/libobs/$BUILD_TYPE/libobs.framework $APP/Frameworks
cp -r ./obs-studio/.build/UI/$BUILD_TYPE/OBS.app/Contents/Frameworks/*.dylib $APP/Frameworks
cp -r ./obs-studio/.build/plugins/obs-outputs/$BUILD_TYPE/obs-outputs.plugin $APP/Frameworks/obs-plugins

# pushd $APP/obs-plugin
# npm install
# npm test
# popd


# # Go into the repository
cd app
# # Install dependencies
npm install
# # Run the app
npm start