#!/bin/sh

if [ ! -d "./libenv/" ]; then
    mkdir ./libenv
fi

if [ ! -d "./../bin/classes" ]; then
    mkdir ./../bin/classes
fi

echo "gen native code: com.openml.jni.JEnv.java"
javac -d ./../bin/classes/ ./../src/com/openml/jni/JEnv.java
javah -classpath ./../bin/classes -d ./libenv -jni com.openml.jni.JEnv 

